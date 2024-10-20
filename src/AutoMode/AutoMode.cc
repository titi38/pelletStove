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
#include "rapidjson/document.h"      // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h"    // for stringify JSON

using namespace rapidjson;

#include "AutoMode.hh"

#define OUTDOORTEMPCORR ( openWeatherClient->getTempForecast() - (double)(openWeatherClient->getHumiForecast())/75.0 )

#define TEMP_CONSIGNE 19.0
#define TEMP_VERYCOLD 15.0
#define TEMP_CONSIGNE_ABSENCE  13.0

// DIFF_TEMP_STARTUP: .5 si T entre 10 et 1°C, 1.5 si moins de 1°C
#define DIFF_TEMP_STARTUP ((OUTDOORTEMPCORR<1.0)?1:.0  + (OUTDOORTEMPCORR<10.0)?.5:.0)

#define DIFF_TEMP_CLEARCAST 1.0
#define DIFF_TEMP_VERYCOLD 2.5


/***********************************************************************/

AutoMode::AutoMode ( ButtonControl *bc, DhtReader *dht, LcdReader *lcd, OpenWeatherClient *owc ) :
    buttonControl (bc), dhtReader (dht), lcdReader (lcd), openWeatherClient (owc)
{
  exiting = false;

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
    const std::tm *tm_local = std::localtime (&time_temp);

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

    shutdownPeriod = ( tm_local->tm_hour >= 22 || tm_local->tm_hour < 5 ) // 22 to 6 hours : off dans tous les modes
                     || ( currentMode == Mode::normal
                          && tm_local->tm_hour >= 8 && tm_local->tm_hour < 17
                          && tm_local->tm_wday != 0 /*&& tm_local->tm_wday != 5*/
                          && tm_local->tm_wday != 6 )  // tm_wday (0 à 6) 0 dimanche - 6 samedi
                     || ( currentMode == Mode::absent && tm_local->tm_hour >= 8 ); // work only from 6 to 8


    // shutdownPeriod... but it's cold !!!
    if ( !veryColdCondition && shutdownPeriod && operatingMode == OperatingMode::off
         && currentMode != Mode::absent && ( tempCorr < TEMP_VERYCOLD )) // 3°C de moins : rallumage anticipé
    {
      veryColdCondition = true;
    }

    if ( !shutdownPeriod ) // only in shutdownPeriod
      veryColdCondition = false;

    // if the pellet is running...
    if ( operatingMode == OperatingMode::on )
    {
      // if the temp/hygro has been reached
      if ((
              ( currentMode == Mode::absent && tempCorr > TEMP_CONSIGNE_ABSENCE )

	      // Si on est entre 9h (ou 11h) et 17h, qu'il fait beau et pas suffisamment froid. On arrête 
	      || ( tm_local->tm_hour >= ( 9 + ( tm_local->tm_mon >= 10 || tm_local->tm_mon <= 1 ) ? 2 : 0 )
                   && tm_local->tm_hour < 17 && openWeatherClient->isClearForcast ()
                   && tempCorr >= TEMP_CONSIGNE - DIFF_TEMP_CLEARCAST )

              || ( shutdownPeriod && !veryColdCondition )
              || ( veryColdCondition && tempCorr >= TEMP_CONSIGNE - DIFF_TEMP_VERYCOLD ))
          && ( std::chrono::duration_cast<std::chrono::minutes> (std::chrono::system_clock::now () - startTime).count ()
               >= 90 ))
      {
        NVJ_LOG->append (NVJ_INFO,
                         "Stop Cond: shutdownPeriod=" + to_string (shutdownPeriod) + ", temp=" + to_string (temp)
                         + ", humi=" + to_string (humi) + ", forecast="
                         + to_string (openWeatherClient->isClearForcast ()) + ", currentMode="
                         + to_string (static_cast<int>(currentMode)) + ", veryColdCondition="
                         + to_string (veryColdCondition));
        buttonControl->stop ();
        stopTime = std::chrono::system_clock::now ();
        veryColdCondition = false;
        needCleaning = true;
        sleep (60 * 60); // 1h
        continue;
      }
      // else
      short deltaPower = 0;

      if ( currentMode == Mode::absent || ( veryColdCondition && !shutdownPeriod ))
        deltaPower = 1 - lcdReader->getPower ();
      else
        deltaPower = std::min (6, ( short ) ( TEMP_CONSIGNE - tempCorr ) + 1) - lcdReader->getPower ();

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
        && !( shutdownPeriod && !veryColdCondition )

        && ( tempCorr < TEMP_CONSIGNE - DIFF_TEMP_STARTUP )

	// Si on est entre 9h (ou 11h) et 17h, qu'il fait beau et pas suffisamment froid. On ne démarre pas !
        && !( tm_local->tm_hour >= ( 9 + ( tm_local->tm_mon >= 10 || tm_local->tm_mon <= 1 ) ? 2 : 0 )
              && tm_local->tm_hour < 17 && openWeatherClient->isClearForcast ()
              && tempCorr >= TEMP_CONSIGNE - DIFF_TEMP_CLEARCAST - DIFF_TEMP_STARTUP ) 

	// On ne démarre pas si on est à moins de 1h30 du précédent arrêt
        && ( std::chrono::duration_cast<std::chrono::minutes> (std::chrono::system_clock::now () - stopTime).count ()
             >= 90 )

	// On démarre plus après 21h30.
        && !( tm_local->tm_hour >= 21 && tm_local->tm_min >= 30 ))
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


