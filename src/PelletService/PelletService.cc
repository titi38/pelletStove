//------------------------------------------------------------------------------
//
// PelletService.cc : exec command
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
#include <csignal>
#include <string.h>
#include <deque>
#include <mutex>
#include <ctime>
#include <chrono>
#include <wiringPi.h>

#include "PelletService.hh"
#include "Stats.hh"

#include "rapidjson/document.h"      // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h"    // for stringify JSON

using namespace rapidjson;

WebServer *webServer = NULL;

// Tampon de logs en mémoire : pas de dépendance envers ZNeTS.
// On garde seulement les messages de cette exécution, sans fichier de log.
class PelletLogOutput : public LogOutput {
public:
  struct Entry { std::string date, level, message, details; };
  void initialize() {};
  void append(const NvjLogSeverity &level, const std::string &message, const std::string &details) {
    static const char *unknown = "OTHER";
    const char *name = unknown;
    switch(level) {
      case NVJ_DEBUG: name="DEBUG"; break;
      case NVJ_INFO: name="INFO"; break;
      case NVJ_WARNING: name="WARNING"; break;
      case NVJ_ALERT: name="ALERT"; break;
      case NVJ_ERROR: name="ERROR"; break;
      case NVJ_FATAL: name="FATAL"; break;
      default: break;
    }
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{}; localtime_r(&now, &local);
    char date[32]; std::strftime(date,sizeof(date),"%Y-%m-%d %H:%M:%S",&local);
    std::lock_guard<std::mutex> lock(mutex_);
    if (entries_.size() >= 500) entries_.pop_front();
    entries_.push_back({date,name,message,details});
  }
  std::deque<Entry> snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_;
  }
private:
  mutable std::mutex mutex_;
  std::deque<Entry> entries_;
};

static PelletLogOutput *pelletLogOutput = nullptr;

bool PelletLogs::getPage(HttpRequest *request, HttpResponse *response) {
  GenericStringBuffer<UTF8<>> buffer;
  Writer<GenericStringBuffer<UTF8<>>> writer(buffer);
  writer.StartObject();
  writer.String("logs"); writer.StartArray();
  if (pelletLogOutput) {
    for (const auto &row : pelletLogOutput->snapshot()) {
      writer.StartObject();
      writer.String("date"); writer.String(row.date.c_str());
      writer.String("level"); writer.String(row.level.c_str());
      writer.String("message"); writer.String(row.message.c_str());
      writer.String("details"); writer.String(row.details.c_str());
      writer.EndObject();
    }
  }
  writer.EndArray(); writer.EndObject();
  response->setCORS();
  return fromString(buffer.GetString(),response);
}



/***********************************************************************/

void exitFunction ( int dummy )
{
  if ( webServer != NULL ) webServer->stopService ();
}

/***********************************************************************/

int main ()
{
  // connect signals
  signal (SIGTERM, exitFunction);
  signal (SIGINT, exitFunction);
  signal (SIGQUIT, exitFunction);


  NVJ_LOG->addLogOutput (new LogStdOutput);
  pelletLogOutput = new PelletLogOutput;
  NVJ_LOG->addLogOutput (pelletLogOutput);
  webServer = new WebServer;

  PelletService service;
  webServer->addRepository (&service);

  webServer->startService ();

  webServer->wait ();

  LogRecorder::freeInstance ();

  return 0;
}

/***********************************************************************/

bool PelletInfoMonitor::PelletInfoMonitor::getPage ( HttpRequest *request, HttpResponse *response )
{
  std::string json = "{ \"indoorData\" : " + dhtReader->getInfoJson () + ",";
  json += "\"pelletMonitor\" : " + lcdReader->getInfoJson () + ",";
  json += "\"pelletGauge\" : " + gauge->getInfoJson () + ",";

  json += "\"outdoorData\" : " + openWeatherClient->getInfoJson () + ",";
  json += "\"autoMode\" : " + autoMode->getInfoJson ();
  json += " }";
  response->setCORS (/*true, false, "http://192."*/);
  return fromString (json, response);
}


/***********************************************************************/

bool PelletStats::PelletStats::getPage ( HttpRequest *request, HttpResponse *response )
{
  std::string json = Stats::getInstance()->getStatsJson();
  response->setCORS (/*true, false, "http://192."*/);
  return fromString (json, response);
}


/***********************************************************************/

bool PelletCommand::PelletCommand::getPage ( HttpRequest *request, HttpResponse *response )
{
  lock_guard <std::mutex> lk (mutex_command);

  GenericStringBuffer <UTF8<>> buffer;
  Writer <GenericStringBuffer<UTF8<> >> writer (buffer);
  bool runSuccess = false;
  std::string resultJson, errMessage, resMessage;

  if ( request->hasParameter ("incPower"))
  {
    short step = 1;
    string ss;
    if ( request->getParameter ("step", ss))
      step = getValue<short> (ss);
    buttonControl->incPower (step);
    runSuccess = true;
  }
  else
    if ( request->hasParameter ("decPower"))
    {
      short step = 1;
      string ss;
      if ( request->getParameter ("step", ss))
        step = getValue<short> (ss);
      buttonControl->decPower (step);
      runSuccess = true;
    }
    else
      if ( request->hasParameter ("incTempWater"))
      {
        short step = 1;
        string ss;
        if ( request->getParameter ("step", ss))
          step = getValue<short> (ss);
        buttonControl->incTempWater (step);
        runSuccess = true;
      }
      else
        if ( request->hasParameter ("decTempWater"))
        {
          short step = 1;
          string ss;
          if ( request->getParameter ("step", ss))
            step = getValue<short> (ss);
          buttonControl->decTempWater (step);
          runSuccess = true;
        }
        else
          if ( request->hasParameter ("start"))
          {
            if ( lcdReader->getCurrentOperatingMode () == OperatingMode::off )
            {
              buttonControl->start ();
              runSuccess = true;
            }
            else
              errMessage = "the pelet is not stopped";

          }
          else
            if ( request->hasParameter ("stop"))
            {
              if ( lcdReader->getCurrentOperatingMode () == OperatingMode::on )
              {
                buttonControl->stop ();
                runSuccess = true;
              }
              else
                errMessage = "the pelet is not running";
            }
            else
              if ( request->hasParameter ("resetAlert"))
              {
                buttonControl->resetError ();
                runSuccess = true;
              }
              else
                if ( request->hasParameter ("clean"))
                {
                  if ( lcdReader->getCurrentOperatingMode () == OperatingMode::off )
                  {
                    buttonControl->stop ();
                    runSuccess = true;
                  }
                  else
                    errMessage = "the cleaning process is unavaillable when the fire is started";
                }
                else
                  if ( request->hasParameter ("getLcdMsg"))
                  {
                    resMessage = lcdReader->getLcdMessage ();
                    runSuccess = true;
                  }
                  else
                    if ( request->hasParameter ("pressButton"))
                    {
                      ButtonControl::ButtonPressionDuration duration = ButtonControl::ButtonPressionDuration::normal;
                      ButtonControl::ControlButtons buttons[2];
                      size_t nbButton = 0;
                      string durStr;
                      if ( request->getParameter ("dur", durStr))
                      {
                        if ( durStr == "short" )
                          duration = ButtonControl::ButtonPressionDuration::quick;
                        else
                          if ( durStr == "normal" )
                            duration = ButtonControl::ButtonPressionDuration::normal;
                          else
                            if ( durStr == "long" )
                              duration = ButtonControl::ButtonPressionDuration::longer;
                            else
                              errMessage = "unknown duration, will use normal duration";
                      }

                      if ( request->hasParameter ("up"))
                        buttons[ nbButton++ ] = ButtonControl::ControlButtons::up;

                      if ( request->hasParameter ("down"))
                        buttons[ nbButton++ ] = ButtonControl::ControlButtons::down;

                      if ( request->hasParameter ("on") && nbButton < 2 )
                        buttons[ nbButton++ ] = ButtonControl::ControlButtons::on;

                      if ( request->hasParameter ("off") && nbButton < 2 )
                        buttons[ nbButton++ ] = ButtonControl::ControlButtons::off;

                      if ( nbButton == 1 )
                        buttonControl->pressButton (buttons[ 0 ], duration);
                      else
                        buttonControl->press2Buttons (buttons[ 0 ], buttons[ 1 ] /*,duration */);

                      runSuccess = true;
                    }
                    else
                      if ( request->hasParameter ("auto"))
                      {
                        string modeStr;
                        if ( request->getParameter ("mode", modeStr))
                        {
                          if ( modeStr == "on" )
                          {
                            autoMode->start (AutoMode::Mode::normal);
                            runSuccess = true;
                          }
                          else
                            if ( modeStr == "off" )
                            {
                              autoMode->stop ();
                              runSuccess = true;
                            }
                            else
                              if ( modeStr == "vac" )
                              {
                                autoMode->start (AutoMode::Mode::vacation);
                                runSuccess = true;
                              }
                              else
                                if ( modeStr == "abs" )
                                {
                                  autoMode->start (AutoMode::Mode::absent);
                                  runSuccess = true;
                                }
                                else
                                  if ( modeStr == "custom" )
                                  {
                                    autoMode->start (AutoMode::Mode::custom);
                                    runSuccess = true;
                                  }
                                  else
                                    errMessage = "unknown mode, ignored";

                        }
                      }
                      else
                        if ( request->hasParameter ("shutdown"))
                        {
                          NVJ_LOG->append (NVJ_INFO, "Shutdown command received...");
                          execl (getenv ("SHELL"), "sh", "-c", "sudo shutdown", NULL);
                        }
                        else
                          errMessage = "Unknown command";

  writer.StartObject ();

  writer.String ("success");
  writer.Bool (runSuccess);
  if ( resMessage != "" )
  {
    writer.String ("result");
    writer.String (resMessage.c_str ());
  }

  if ( errMessage != "" )
  {
    writer.String ("error");
    writer.String (errMessage.c_str ());
  }

  writer.EndObject ();
  resultJson = buffer.GetString ();
  buffer.Clear ();

  response->setCORS (/*true, false, "http://192."*/);
  return fromString (resultJson, response);
}

/***********************************************************************/
// Configuration des périodes de présence (JSON RapidJSON, réponse HTTP classique).
bool PelletSchedule::getPage(HttpRequest *request, HttpResponse *response)
{
  response->setCORS();
  std::string action;
  if (!request->getParameter("action", action))
    return fromString(autoMode->getScheduleJson(), response);

  std::string error, value;
  bool success = false;
  if (action == "save") {
    if (request->getParameter("config", value))
      success = autoMode->saveScheduleJson(value, error);
    else error = "Missing config";

  } else if (action == "override") {
    if (request->getParameter("value", value))
      success = autoMode->setPresenceOverride(value, error);
    else error = "Missing override value";
  } else error = "Unknown action";

  GenericStringBuffer<UTF8<>> buffer;
  Writer<GenericStringBuffer<UTF8<>>> writer(buffer);
  writer.StartObject();
  writer.Key("success"); writer.Bool(success);
  if (!error.empty()) { writer.Key("error"); writer.String(error.c_str()); }
  writer.EndObject();
  return fromString(buffer.GetString(), response);
}

/***********************************************************************/

PelletService::PelletService ()
{
  if ( wiringPiSetup () == -1 )
    exit (1);

  lcdReader = new LcdReader;
  buttonControl = new ButtonControl (lcdReader);
  dhtReader = new DhtReader;
  gauge = new Gauge;

  delay (5000);

//buttonControl->resetError();

  if ( lcdReader->getCurrentOperatingMode () == OperatingMode::unknown )
    buttonControl->goToMainMenu ();

  autoMode = new AutoMode (buttonControl, dhtReader, lcdReader, &openWeatherClient);

  pelletSchedule = new PelletSchedule(autoMode);
  add("schedule.json", pelletSchedule);

  pelletLogs = new PelletLogs ();
  add ("logs.json", pelletLogs);

  pelletStats = new PelletStats ();
  add ("stats.json", pelletStats);

  pelletInfoMonitor = new PelletInfoMonitor (dhtReader, lcdReader, gauge, &openWeatherClient, autoMode);
  add ("info.json", pelletInfoMonitor);

  pelletCommand = new PelletCommand (buttonControl, lcdReader, autoMode);
  add ("command.json", pelletCommand);


}

/***********************************************************************/

PelletService::~PelletService ()
{
  delete pelletLogs;
  delete pelletSchedule;
  delete pelletStats;
  delete pelletCommand;
  delete pelletInfoMonitor;
  delete autoMode;
  delete dhtReader;
  delete lcdReader;
  delete gauge;
  delete buttonControl;
}


