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

#include "rapidjson/document.h"    	// rapidjson's DOM-style API
#include "rapidjson/prettywriter.h"  	// for stringify JSON
using namespace rapidjson;

#include "AutoMode.hh"

#define humidexSwitchOff 19
#define humidexConsigne  18

  /***********************************************************************/

  AutoMode::AutoMode(ButtonControl *bc, DhtReader *dht, LcdReader *lcd, OpenWeatherClient* owc): 
          buttonControl(bc), dhtReader(dht), lcdReader (lcd), openWeatherClient(owc)
  {
    exiting = false;

    thread_loop=new thread(&AutoMode::loop, this);
    NVJ_LOG->append(NVJ_INFO, "AutoMode is starting" );
  }

  AutoMode::~AutoMode()
  {
    exiting = true;
    thread_loop->join();
  }

  /***********************************************************************/

  void AutoMode::loop()
  {
   bool shutdownPeriod = false;
   bool needCleaning = false;
   sleep(5);

   while ( !exiting )
    {
      if (needCleaning)
      {
        buttonControl->stop();
        needCleaning = false;
        sleep(20*60); // 20mn
      }

      if ( ( currentMode == Mode::off )
         || ( lcdReader->getCurrentOperatingMode() == OperatingMode::alertTempFume )
         || ( lcdReader->getCurrentOperatingMode() == OperatingMode::alertTermDepr ) )
      {
         sleep(1);
         continue;
      }

      std::time_t time_temp = std::time(nullptr);
      const std::tm * tm_local = std::localtime(&time_temp);

      // time_out->tm_wday (0 à 6) 0 dimanche - 6 samedi
      shutdownPeriod = ( tm_local->tm_hour < 6 ) // 0 to 6 hours : off dans tous les modes
                    || ( currentMode == Mode::basic && tm_local->tm_hour >= 9 && tm_local->tm_hour < 17 && tm_local->tm_wday !=0 && tm_local->tm_wday !=6 )
                    || ( currentMode == Mode::absent && tm_local->tm_hour >= 7 && dhtReader->getHumidex() < 15 );
      // custom

      double currHumidex=dhtReader->getHumidex();

      double deltaHumidex=currHumidex - humidexConsigne;

      if ( lcdReader->getCurrentOperatingMode() == OperatingMode::starting )
      {
        sleep(10);
        continue;
      }

      if ( lcdReader->getCurrentOperatingMode() == OperatingMode::on )
      {
        if ( ( currHumidex - humidexSwitchOff > 0 )
           || shutdownPeriod )
        {
          buttonControl->stop();
          needCleaning = true;
          sleep(60*60); // 1h
          continue;
        }
        // else
        short deltaPower = std::min (6, (short)(-deltaHumidex) + 1) - lcdReader->getPower();
        if (deltaPower != 0)
          buttonControl->incPower(deltaPower);
        sleep(60); // 1mn
        continue;
      }

      if (  ( lcdReader->getCurrentOperatingMode() == OperatingMode::off )
         && ! shutdownPeriod
         && ( deltaHumidex < -1.0 ) ) // && heure de presence !
      {
        buttonControl->start();
        sleep(20*60); // 20mn
        continue;
      }
    }
  }

  /***********************************************************************/

  void AutoMode::start(AutoMode::Mode m)
  {
    currentMode=m;
  }

  /***********************************************************************/

  void AutoMode::stop()
  {
    currentMode=Mode::off;
  }

  /***********************************************************************/

  std::string AutoMode::getInfoJson() const
  { 
    std::string resultat = "";
    GenericStringBuffer<UTF8<> > buffer;
    Writer<GenericStringBuffer<UTF8<> > > writer( buffer );
    writer.StartObject();
    writer.String( "mode" );

    switch(currentMode)
    {
      case Mode::off:
        writer.String("off");
        break;
      case Mode::basic:
        writer.String("basic");
        break;
      case Mode::vacation:
        writer.String("vacation");
        break;
      case Mode::absent:
        writer.String("absent");
        break;
      case Mode::custom:
        writer.String("custom");
        break;
    }

    writer.EndObject();
    resultat = buffer.GetString();
    buffer.Clear();
    return resultat;
  }


