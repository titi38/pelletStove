//------------------------------------------------------------------------------
//
// AutoMode.cc : exec command
//
//------------------------------------------------------------------------------
//
// AUTHOR      : T.DESCOMBES (thierry.descombes@gmail.com)
// PROJECT     : pelletStove
//
//------------------------------------------------------------------------------
//           Versions and Editions  historic
//------------------------------------------------------------------------------
// Ver      | Ed  |   date   | resp.       | comment
//----------+-----+----------+-------------+------------------------------------
// 1        | 1   | 01/01/19 | T.DESCOMBES | source creation
//----------+-----+----------+-------------+------------------------------------
//
//           Detailed modification operations on this source
//------------------------------------------------------------------------------
//  Date  :                            Author :
//  REL   :
//  Description :
//
//------------------------------------------------------------------------------

#include <unistd.h>
#include <ctime>
#include <cmath>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "rapidjson/document.h"      // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h"    // for stringify JSON

using namespace rapidjson;

#include "AutoMode.hh"

#define OUTDOORTEMPCORR ( openWeatherClient->getTempForecast() - (double)(openWeatherClient->getHumiForecast())/75.0 )

#define TEMP_CONSIGNE 20
#define TEMP_VERYCOLD 15.0
#define TEMP_CONSIGNE_ABSENCE  13.0

// DIFF_TEMP_STARTUP: .5 si T entre 10 et 1°C, 1.5 si moins de 1°C
#define DIFF_TEMP_STARTUP ((OUTDOORTEMPCORR < 1.0) ? 1.0 : 0.5) + ((OUTDOORTEMPCORR < 10.0) ? 0.5 : 0.0)
#define DIFF_TEMP_SHUTDOWN ((OUTDOORTEMPCORR < 1.0) ? 1.0 : 0.5)

#define DIFF_TEMP_CLEARCAST 1.0
#define DIFF_TEMP_VERYCOLD 2.5


/***********************************************************************/

AutoMode::AutoMode ( ButtonControl *bc, DhtReader *dht, LcdReader *lcd, OpenWeatherClient *owc ) :
    buttonControl (bc), dhtReader (dht), lcdReader (lcd), openWeatherClient (owc)
{
  exiting = false;
  const char *configuredPath = std::getenv("PELLET_SCHEDULE_FILE");
  schedulePath = (configuredPath && *configuredPath) ? configuredPath : "presence-schedule.json";
  // Aucun horaire cache dans le code : sans configuration valide, aucun creneau
  // de presence n'est programme. Les protections antigel restent independantes.
  const char *configuredDefault = std::getenv("PELLET_DEFAULT_SCHEDULE_FILE");
  const std::string defaultPath = (configuredDefault && *configuredDefault)
      ? configuredDefault : "config/presence-schedule.default.json";

  // Une configuration utilisateur existante a toujours priorite sur le modele.
  // Si elle est invalide, on ne la remplace PAS par le modele silencieusement.
  std::ifstream savedFile(schedulePath);
  if (savedFile.is_open()) {
    savedFile.close();
    loadScheduleFromDisk();
  } else {
    std::ifstream defaultFile(defaultPath);
    if (defaultFile.is_open()) {
      std::ostringstream contents;
      contents << defaultFile.rdbuf();
      std::string error;
      if (!applyScheduleJson(contents.str(), error, false))
        NVJ_LOG->append(NVJ_WARNING,
            "Invalid default presence schedule: " + error);
      else
        NVJ_LOG->append(NVJ_INFO,
            "Loaded default presence schedule from " + defaultPath);
    } else {
      NVJ_LOG->append(NVJ_WARNING,
          "No saved schedule or default schedule; no presence periods enabled");
    }
  }

  thread_loop = new thread (&AutoMode::loop, this);
  NVJ_LOG->append (NVJ_INFO, "AutoMode is starting");
}

AutoMode::~AutoMode ()
{
  exiting = true;
  thread_loop->join ();
}

/***********************************************************************/

void AutoMode::loop ()
{
  bool shutdownPeriod = false;
  bool needCleaning = false;
  bool veryColdCondition = false;

  double tempCorr = .0;

  unsigned char nbStrangeTemperature=0;

  std::chrono::time_point <std::chrono::system_clock> startTime =
      std::chrono::system_clock::now () - std::chrono::minutes (90);
  std::chrono::time_point <std::chrono::system_clock> stopTime (startTime);

  sleep (5);

  while ( !exiting )
  {
    if (( currentMode == Mode::off )
        || ( lcdReader->getCurrentOperatingMode () == OperatingMode::alertTempFume )
        || ( lcdReader->getCurrentOperatingMode () == OperatingMode::alertTermDepr ))
    {
      sleep (1);
      continue;
    }

    if ( lcdReader->getCurrentOperatingMode () == OperatingMode::starting )
    {
      sleep (10);
      continue;
    }

    if ( needCleaning && ( lcdReader->getCurrentOperatingMode () == OperatingMode::off )
         && ( currentMode != Mode::off ))
    {
      buttonControl->stop ();
      needCleaning = false;
      sleep (20 * 60); // 20mn
    }

    std::time_t time_temp = std::time (nullptr);
    std::tm localTime;
    const std::tm *tm_local = localtime_r(&time_temp, &localTime);

    OperatingMode operatingMode = lcdReader->getCurrentOperatingMode ();

    double temp = dhtReader->getTemp ();
    double humi = dhtReader->getHumi ();
    if (( temp < -100.0 ) || ( temp > 100.0 ) || ( humi < .0 ) || ( humi > 100.0 ))
    {
      NVJ_LOG->append (NVJ_INFO,
                       "Bad values of temperature/hygro:  temp=" + to_string (temp) + ", humi=" + to_string (humi)
                       + ", re-read");
      sleep (10);
      continue;
    }

    double tempCorrOld = tempCorr;
    tempCorr = temp - humi / 75.0;
// Cas 1 : 1 lecture avec résultat à ignorer
// Cas 2 : 2 lectures qui se confirment l'écart
    if ( tempCorrOld != .0 && std::abs (tempCorrOld - tempCorr) > 1.0 && nbStrangeTemperature<2)
    {

      NVJ_LOG->append (NVJ_INFO,
                       "Big difference in temperature/hygro:  temp=" + to_string (temp) + ", humi=" + to_string (humi)
                       + ", tempCorrOld=" + to_string (tempCorrOld) + ", tempCorr=" + to_string (tempCorr)
                       + ", re-read");
tempCorr = tempCorrOld;
nbStrangeTemperature++;
      sleep (10);
      continue;
    }
    else
      nbStrangeTemperature=0;

    // Snapshot de la programmation, sans garder le mutex pendant les commandes physiques.
    bool scheduledPresence = false;
    bool effectivePresence = false;
    double targetTemperature = TEMP_CONSIGNE;
    {
      std::lock_guard<std::mutex> lock(scheduleMutex);
      scheduledPresence = isScheduledPresence(*tm_local);
      if (presenceOverride != PresenceOverride::none &&
          scheduledPresence != overrideOriginalPresence)
        presenceOverride = PresenceOverride::none; // prochaine transition hebdomadaire
      effectivePresence = presenceOverride == PresenceOverride::arrive ? true :
                          presenceOverride == PresenceOverride::leave ? false : scheduledPresence;
      targetTemperature = currentMode == Mode::normal ?
          comfortTemperature : TEMP_CONSIGNE;
    }

    // En mode Auto, les périodes de présence configurées remplacent les horaires figés.
    // Les protections du poêle et l'antigel restent traités plus bas.
    shutdownPeriod = (currentMode == Mode::normal) ?
                     !effectivePresence :
                     ((tm_local->tm_hour >= 22 || tm_local->tm_hour < 5)
                     || (currentMode == Mode::absent && tm_local->tm_hour >= 6 && tm_local->tm_hour < 8));


    // shutdownPeriod... but it's cold !!!
    /*if (!veryColdCondition && shutdownPeriod && operatingMode == OperatingMode::off
         && currentMode != Mode::absent && ( tempCorr < TEMP_VERYCOLD ) ) // 3°C de moins : rallumage anticipé
    {
      veryColdCondition = true;
    }

    if ( !shutdownPeriod ) // only in shutdownPeriod
      veryColdCondition = false;*/

    veryColdCondition = tempCorr < TEMP_VERYCOLD;

    // if the pellet is running...
    if ( operatingMode == OperatingMode::on )
    {
      // if the temp/hygro has been reached
      if ((
              ( currentMode == Mode::absent && tempCorr > TEMP_CONSIGNE_ABSENCE )

	      || ( tempCorr >= targetTemperature + DIFF_TEMP_SHUTDOWN)

	      // Si on est entre 9h (ou 11h) et 17h, qu'il fait beau et pas suffisamment froid. On arrête 
	      || ( tm_local->tm_hour >= (9 + ((tm_local->tm_mon >= 10 || tm_local->tm_mon <= 1) ? 2 : 0))
                   && tm_local->tm_hour < 17 && openWeatherClient->isClearForcast ()
                   && tempCorr >= targetTemperature + DIFF_TEMP_SHUTDOWN - DIFF_TEMP_CLEARCAST )

              || ( shutdownPeriod && !veryColdCondition ))
             // || ( veryColdCondition && tempCorr >= targetTemperature - DIFF_TEMP_VERYCOLD ))
          && ( std::chrono::duration_cast<std::chrono::minutes> (std::chrono::system_clock::now () - startTime).count ()
               >= 90 ))
      {

	bool expr1 = (currentMode == Mode::absent && tempCorr > TEMP_CONSIGNE_ABSENCE);
	bool expr2 = (tempCorr >= targetTemperature + DIFF_TEMP_SHUTDOWN);
	bool expr3 = (tm_local->tm_hour >= (9 + ((tm_local->tm_mon >= 10 || tm_local->tm_mon <= 1) ? 2 : 0)) &&
		                  tm_local->tm_hour < 17 && openWeatherClient->isClearForcast() &&
	                    tempCorr >= targetTemperature + DIFF_TEMP_SHUTDOWN - DIFF_TEMP_CLEARCAST);
	bool expr4 = (shutdownPeriod && !veryColdCondition);
	bool expr5 = (veryColdCondition && tempCorr >= targetTemperature - DIFF_TEMP_VERYCOLD);

      	std::ostringstream debugOutput;
	debugOutput << std::boolalpha; // Afficher les booléens comme "true" ou "false"

	// Ajouter les valeurs des expressions
	debugOutput << "(currentMode == Mode::absent && tempCorr > TEMP_CONSIGNE_ABSENCE): " << expr1 << "\n";
	debugOutput << "(tempCorr >= targetTemperature + DIFF_TEMP_SHUTDOWN): " << expr2 << "\n";
	debugOutput << "(tm_local->tm_hour >= (9 + (tm_local->tm_mon >= 10 || tm_local->tm_mon <= 1) ? 2 : 0) "
	       << "&& tm_local->tm_hour < 17 && openWeatherClient->isClearForcast() "
	       << "&& tempCorr >= targetTemperature + DIFF_TEMP_SHUTDOWN - DIFF_TEMP_CLEARCAST): " << expr3 << "\n";
	debugOutput << "(shutdownPeriod && !veryColdCondition): " << expr4 << "\n";
	debugOutput << "(veryColdCondition && tempCorr >= targetTemperature - DIFF_TEMP_VERYCOLD): " << expr5 << "\n";
	// Ajouter la date et l'heure
	debugOutput << "Date: " << std::put_time(tm_local, "%Y-%m-%d %H:%M:%S") << "\n";
	
        NVJ_LOG->append (NVJ_INFO,
                         "Stop Cond: shutdownPeriod=" + to_string (shutdownPeriod) + ", tempCorr=" + to_string (tempCorr) + ", temp=" + to_string (temp)
                         + ", humi=" + to_string (humi) + ", forecast="
                         + to_string (openWeatherClient->isClearForcast ()) + ", currentMode="
                         + to_string (static_cast<int>(currentMode)) + ", veryColdCondition="
                         + to_string (veryColdCondition) + "DEBUG: " + debugOutput.str());
        buttonControl->stop ();
        stopTime = std::chrono::system_clock::now ();
//        veryColdCondition = false;
        needCleaning = true;
        sleep (60 * 60); // 1h
        continue;
      }
      // else
      short deltaPower = 0;

      //if ( currentMode == Mode::absent || ( veryColdCondition && !shutdownPeriod ))
      if ( currentMode == Mode::absent )
        deltaPower = (veryColdCondition ? 2:1) - lcdReader->getPower ();
      else
        deltaPower = std::min (6, ( short ) ( targetTemperature - tempCorr ) + 1) - lcdReader->getPower ();

      if ( deltaPower != 0 )
      {
        buttonControl->incPower (deltaPower);
        NVJ_LOG->append (NVJ_INFO,
                         "incPower(" + to_string (deltaPower) + "), tempCorr=" + to_string (tempCorr) + ", temp="
                         + to_string (temp) + ", humi=" + to_string (humi));
      }
      sleep (60); // 1mn
      continue;
    }

    // if the pellet is stopped: startCond ?
    if (( operatingMode == OperatingMode::off )

	&& 
	(( !shutdownPeriod

          && (   ( currentMode == Mode::absent && tempCorr < TEMP_CONSIGNE_ABSENCE - DIFF_TEMP_STARTUP )
          || ( currentMode != Mode::absent && tempCorr < targetTemperature - DIFF_TEMP_STARTUP ) )

	  // Si on est entre 9h (ou 11h) et 17h, qu'il fait beau et pas suffisamment froid. On ne démarre pas !
          && !( tm_local->tm_hour >= (9 + ((tm_local->tm_mon >= 10 || tm_local->tm_mon <= 1) ? 2 : 0))
              && tm_local->tm_hour < 17 && openWeatherClient->isClearForcast ()
              && tempCorr >= targetTemperature - DIFF_TEMP_CLEARCAST - DIFF_TEMP_STARTUP )

	  // On ne démarre pas si on est à moins de 1h30 du précédent arrêt
          && ( std::chrono::duration_cast<std::chrono::minutes> (std::chrono::system_clock::now () - stopTime).count ()
             >= 90 )

	  // On démarre plus après 21h30.
          && (currentMode == Mode::normal || !(tm_local->tm_hour == 21 && tm_local->tm_min >= 30)))
		
	||  veryColdCondition ))
    {
      NVJ_LOG->append (NVJ_INFO,
                       "Start Cond: tempCorr=" + to_string (tempCorr) + ", temp=" + to_string (temp) + ", humi="
                       + to_string (humi) + ", forecast=" + to_string (openWeatherClient->isClearForcast ())
                       + ", currentMode=" + to_string (static_cast<int>(currentMode)) + ", veryColdCondition="
                       + to_string (veryColdCondition));
      buttonControl->start ();
      startTime = std::chrono::system_clock::now ();
      sleep (20 * 60); // 20mn
      continue;
    }
    sleep(1); // éviter une boucle CPU intensive quand aucune action n’est nécessaire
  }
}

/***********************************************************************/

void AutoMode::start ( AutoMode::Mode m )
{
  currentMode = m;
}

/***********************************************************************/

void AutoMode::stop ()
{
  currentMode = Mode::off;
}

/***********************************************************************/

std::string AutoMode::getInfoJson () const
{
  std::string resultat = "";
  GenericStringBuffer <UTF8<>> buffer;
  Writer <GenericStringBuffer<UTF8<> >> writer (buffer);
  writer.StartObject ();
  writer.String ("mode");

  switch ( currentMode )
  {
    case Mode::off:
      writer.String ("off");
      break;
    case Mode::normal:
      writer.String ("basic");
      break;
    case Mode::vacation:
      writer.String ("vacation");
      break;
    case Mode::absent:
      writer.String ("absent");
      break;
    case Mode::custom:
      writer.String ("custom");
      break;
  }

  writer.EndObject ();
  resultat = buffer.GetString ();
  buffer.Clear ();
  return resultat;
}



/***********************************************************************/
// Programmation des périodes de présence. Tous les horaires sont locaux.
std::string AutoMode::timeString(int minutes) {
  char text[6];
  std::snprintf(text, sizeof(text), "%02d:%02d", minutes / 60, minutes % 60);
  return text;
}
bool AutoMode::parseTime(const std::string &str, int &minutes) {
  if (str.size() != 5 || str[2] != ':' || str[0] < '0' || str[0] > '9' ||
      str[1] < '0' || str[1] > '9' || str[3] < '0' || str[3] > '9' || str[4] < '0' || str[4] > '9') return false;
  const int hour = (str[0]-'0')*10 + str[1]-'0';
  const int minute = (str[3]-'0')*10 + str[4]-'0';
  if (hour > 23 || minute > 59) return false;
  minutes = hour * 60 + minute;
  return true;
}
bool AutoMode::isScheduledPresence(const std::tm &now) const {
  const int day = (now.tm_wday + 6) % 7;
  const int minute = now.tm_hour * 60 + now.tm_min;
  for (const PresencePeriod &p : presenceDays[day])
    if (minute >= p.start && minute < p.end) return true;
  return false;
}
std::string AutoMode::getScheduleJson() const {
  std::lock_guard<std::mutex> lock(scheduleMutex);
  GenericStringBuffer<UTF8<>> buffer;
  Writer<GenericStringBuffer<UTF8<>>> writer(buffer);
  writer.StartObject();
  writer.Key("days"); writer.StartArray();
  for (const auto &day : presenceDays) {
    writer.StartArray();
    for (const auto &p : day) {
      writer.StartObject();
      writer.Key("start"); writer.String(timeString(p.start).c_str());
      writer.Key("end"); writer.String(timeString(p.end).c_str());
      writer.EndObject();
    }
    writer.EndArray();
  }
  writer.EndArray();
  writer.Key("comfort"); writer.Double(comfortTemperature);
  writer.Key("override"); writer.String(presenceOverride == PresenceOverride::arrive ? "arrive" :
     presenceOverride == PresenceOverride::leave ? "leave" : "none");
  std::time_t now = std::time(nullptr); std::tm local{};
  localtime_r(&now, &local);
  writer.Key("presence"); writer.Bool(isScheduledPresence(local));
  writer.EndObject();
  return buffer.GetString();
}
bool AutoMode::saveScheduleToDisk(const std::array<std::vector<PresencePeriod>, 7> &days,
  double comfort, std::string &error) const {
  GenericStringBuffer<UTF8<>> buffer;
  Writer<GenericStringBuffer<UTF8<>>> writer(buffer);
  writer.StartObject(); writer.Key("days"); writer.StartArray();
  for (const auto &day : days) {
    writer.StartArray();
    for (const auto &period : day) {
      writer.StartObject(); writer.Key("start"); writer.String(timeString(period.start).c_str());
      writer.Key("end"); writer.String(timeString(period.end).c_str()); writer.EndObject();
    }
    writer.EndArray();
  }
  writer.EndArray(); writer.Key("comfort"); writer.Double(comfort);
  writer.EndObject();
  const std::string tmp = schedulePath + ".tmp";
  {
    std::ofstream file(tmp, std::ios::out | std::ios::trunc);
    if (!file) {error = "Cannot open schedule file for writing"; return false;}
    file << buffer.GetString() << "\n";
    file.flush();
    if (!file) {error = "Cannot write schedule file"; std::remove(tmp.c_str()); return false;}
  }
  if (std::rename(tmp.c_str(), schedulePath.c_str()) != 0) {
    error = "Cannot replace schedule file"; std::remove(tmp.c_str()); return false;
  }
  return true;
}
bool AutoMode::saveScheduleJson(const std::string &json, std::string &error) {
  return applyScheduleJson(json, error, true);
}
bool AutoMode::applyScheduleJson(const std::string &json, std::string &error, bool persist) {
  Document document; document.Parse(json.c_str());
  if (document.HasParseError() || !document.IsObject() ||
      !document.HasMember("days") || !document["days"].IsArray() || document["days"].Size() != 7 ||
      !document.HasMember("comfort") || !document["comfort"].IsNumber()) {
    error = "Invalid schedule JSON"; return false;
  }
  double comfort = document["comfort"].GetDouble();
  if (!std::isfinite(comfort) || comfort < 10 || comfort > 25) {
    error = "Invalid temperatures"; return false;
  }
  std::array<std::vector<PresencePeriod>, 7> days;
  for (rapidjson::SizeType d=0; d<7; ++d) {
    const auto &items=document["days"][d];
    if (!items.IsArray() || items.Size()>12) {error="Too many periods or invalid day";return false;}
    for (const auto &item : items.GetArray()) {
      if (!item.IsObject() || !item.HasMember("start") || !item["start"].IsString() ||
          !item.HasMember("end") || !item["end"].IsString()) {error="Invalid period";return false;}
      int from=0, to=0;
      if (!parseTime(item["start"].GetString(),from) || !parseTime(item["end"].GetString(),to) || from>=to) {
        error="Invalid period times";return false;
      }
      days[d].push_back({from,to});
    }
    std::sort(days[d].begin(),days[d].end(),[](const PresencePeriod&a,const PresencePeriod&b){return a.start<b.start;});
    for (size_t i=1;i<days[d].size();++i) if(days[d][i].start<days[d][i-1].end) {
      error="Overlapping presence periods";return false;
    }
  }
  std::lock_guard<std::mutex> lock(scheduleMutex);
  if (persist && !saveScheduleToDisk(days,comfort,error)) return false;
  presenceDays=days; comfortTemperature=comfort;
  presenceOverride=PresenceOverride::none;
  return true;
}
void AutoMode::loadScheduleFromDisk() {
  std::ifstream file(schedulePath);
  if (!file) return;
  std::ostringstream contents; contents << file.rdbuf();
  std::string error;
  if (!applyScheduleJson(contents.str(),error,false))
    NVJ_LOG->append(NVJ_WARNING, "Ignored invalid schedule file: " + error);
}
bool AutoMode::setPresenceOverride(const std::string &value,std::string &error) {
  if (currentMode != Mode::normal) {error="Auto mode is required";return false;}
  std::lock_guard<std::mutex> lock(scheduleMutex);
  if (value == "clear") {presenceOverride=PresenceOverride::none;return true;}
  if (value != "arrive" && value != "leave") {error="Invalid override";return false;}
  std::time_t now=std::time(nullptr); std::tm local{}; localtime_r(&now,&local);
  overrideOriginalPresence=isScheduledPresence(local);
  presenceOverride = value == "arrive" ? PresenceOverride::arrive : PresenceOverride::leave;
  return true;
}
