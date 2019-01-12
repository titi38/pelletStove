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

        std::string json = "{ \"InsideTempSensor\" : {";
        json += "\"temp\": " + to_string( theService->dhtReader.getTemp() ) + ",";
        json += "\"humi\": " + to_string( theService->dhtReader.getHumi() ) + ",";
        json += "\"humidex\": " + to_string( theService->dhtReader.getHumidex() ) + "},";

	json += "\"pelletMonitor\" : {";
        json += "\"power\": " + to_string( theService->lcdReader.getPower() ) + ",";
        json += "\"tempWater\": " + to_string( theService->lcdReader.getTempWater() ) + ",";
        json += "\"tempWaterConsigne\": " + to_string( theService->lcdReader.getTempWaterConsigne() ) + "},";

//        std::string json = "{ \"OusideTempSensor\" : {";
//        json += "\"temp\": " + to_string( .getTemp() );


        json += " }";

        return fromString(json, response);
}

/***********************************************************************/

PelletService::PelletService()
{
    pelletInfoMonitor = new PelletInfoMonitor(this);
    add("info.json",pelletInfoMonitor);
}

  /***********************************************************************/

PelletService::~PelletService()
{
  delete pelletInfoMonitor;
}


