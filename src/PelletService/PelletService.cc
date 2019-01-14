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



#include <signal.h> 
#include <string.h> 
#include <wiringPi.h>

#include "PelletService.hh"


WebServer *webServer = NULL;

/***********************************************************************/

void exitFunction( int dummy )
{
   if (webServer != NULL) webServer->stopService();
}

/***********************************************************************/

int main()
{
  // connect signals
  signal( SIGTERM, exitFunction );
  signal( SIGINT, exitFunction );
  
  NVJ_LOG->addLogOutput(new LogStdOutput);
  webServer = new WebServer;

  PelletService service;
  webServer->addRepository(&service);

  webServer->startService();

  webServer->wait();
  
  LogRecorder::freeInstance();

  return 0;
}

/***********************************************************************/

bool PelletInfoMonitor::PelletInfoMonitor::getPage(HttpRequest* request, HttpResponse *response)
{

        std::string json = "{ \"indoorData\" : " + dhtReader->getInfoJson() + ",";
	json += "\"pelletMonitor\" : " + lcdReader->getInfoJson() + ",";
        json += "\"outdoorData\" : " + openWeatherClient->getInfoJson();
        json += " }";

        return fromString(json, response);
}

/***********************************************************************/

bool PelletCommand::PelletCommand::getPage(HttpRequest* request, HttpResponse *response)
{

  if ( request->hasParameter( "incPower" ) )
  {
    short step = 1;
    string ss;
    if ( request->getParameter( "step", ss ) )
        step = getValue<short>( ss );
    buttonControl->incPower(step);
  }

  if ( request->hasParameter( "decPower" ) )
  {
    short step = 1;
    string ss;
    if ( request->getParameter( "step", ss ) )
        step = getValue<short>( ss );
    buttonControl->decPower(step);
  }

  if ( request->hasParameter( "incTempWater" ) )
  {
    short step = 1;
    string ss;
    if ( request->getParameter( "step", ss ) )
        step = getValue<short>( ss );
    buttonControl->incTempWater(step);
  }

  if ( request->hasParameter( "decTempWater" ) )
  {
    short step = 1;
    string ss;
    if ( request->getParameter( "step", ss ) )
        step = getValue<short>( ss );
    buttonControl->decTempWater(step);
  }

  if ( request->hasParameter( "start" ) )
    buttonControl->start();

  if ( request->hasParameter( "stop" ) )
    buttonControl->stop();

  if ( request->hasParameter( "resetAlert" ) )
    buttonControl->resetError();

  return true;
}

/***********************************************************************/

PelletService::PelletService()
{
    delay(2000);
    if (lcdReader.getCurrentOperatingMode() == OperatingMode::unknown)
      buttonControl.goToMainMenu();

    pelletInfoMonitor = new PelletInfoMonitor(&dhtReader, &lcdReader, &openWeatherClient);
    add("info.json",pelletInfoMonitor);

    pelletCommand = new PelletCommand(&buttonControl);
    add("command.json", pelletCommand);
}

  /***********************************************************************/

PelletService::~PelletService()
{
  delete pelletInfoMonitor;
}


