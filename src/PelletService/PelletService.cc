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



#include <csignal>
#include <string.h>
#include <wiringPi.h>

#include "PelletService.hh"

#include "rapidjson/document.h"      // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h"    // for stringify JSON

using namespace rapidjson;

WebServer *webServer = NULL;

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
                        if ( durStr == "quick" )
                          duration = ButtonControl::ButtonPressionDuration::quick;
                        else
                          if ( durStr == "normal" )
                            duration = ButtonControl::ButtonPressionDuration::longer;
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

PelletService::PelletService ()
{
  if ( wiringPiSetup () == -1 )
    exit (1);

  lcdReader = new LcdReader;
  buttonControl = new ButtonControl (lcdReader);
  dhtReader = new DhtReader;
  gauge = new Gauge;

  delay (5000);

  if ( lcdReader->getCurrentOperatingMode () == OperatingMode::unknown )
    buttonControl->goToMainMenu ();

  autoMode = new AutoMode (buttonControl, dhtReader, lcdReader, &openWeatherClient);

  pelletInfoMonitor = new PelletInfoMonitor (dhtReader, lcdReader, gauge, &openWeatherClient, autoMode);
  add ("info.json", pelletInfoMonitor);

  pelletCommand = new PelletCommand (buttonControl, lcdReader, autoMode);
  add ("command.json", pelletCommand);
}

/***********************************************************************/

PelletService::~PelletService ()
{
  delete pelletCommand;
  delete pelletInfoMonitor;
  delete autoMode;
  delete dhtReader;
  delete lcdReader;
  delete gauge;
  delete buttonControl;
}


