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
      if ( ( currentMode == Mode::off )
         || ( lcdReader->getCurrentOperatingMode() == OperatingMode::alertTempFume )
         || ( lcdReader->getCurrentOperatingMode() == OperatingMode::alertTermDepr ) )
      {
         sleep(1);
         continue;
      }

      if ( lcdReader->getCurrentOperatingMode() == OperatingMode::starting )
      {
        sleep(10);
        continue;
      }

      if ( needCleaning && (  lcdReader->getCurrentOperatingMode() == OperatingMode::off ) && (currentMode != Mode::off ) )
      {
        buttonControl->stop();
        needCleaning = false;
        sleep(20*60); // 20mn
      }

      std::time_t time_temp = std::time(nullptr);
      const std::tm * tm_local = std::localtime(&time_temp);

      shutdownPeriod = ( tm_local->tm_hour < 6 ) // 0 to 6 hours : off dans tous les modes
                    || ( currentMode == Mode::basic && tm_local->tm_hour >= 9 && tm_local->tm_hour < 17 && tm_local->tm_wday !=0 && tm_local->tm_wday !=5 ) // tm_wday (0 à 6) 0 dimanche - 6 samedi
		    // change value 6 to 5: vendredi télétravail
                    || ( currentMode == Mode::absent && tm_local->tm_hour >= 8 ); // work only from 6 to 8
      // custom

//      double currHumidex=dhtReader->getHumidex();
//      double deltaHumidex=currHumidex - humidexConsigne;


      // if the pellet is running...
      if ( lcdReader->getCurrentOperatingMode() == OperatingMode::on )
      {
	// if the temp/hygro has been reached
        if (  ( currentMode == Mode::absent && dhtReader->getTemp() > 14.0 )
           || ( currentMode != Mode::absent && dhtReader->getTemp() > 20.0 )
           || shutdownPeriod )
        {
          buttonControl->stop();
          needCleaning = true;
          sleep(60*60); // 1h
          continue;
        }
        // else
        short deltaPower = 0;

        if (currentMode == Mode::absent)
          deltaPower = 1 - lcdReader->getPower();
        else
          deltaPower = std::min ( 6, (short)(19.0 - dhtReader->getTemp() ) + 1) - lcdReader->getPower(); // getTemp-20

        if (deltaPower != 0)
          buttonControl->incPower(deltaPower);
        sleep(60); // 1mn
        continue;
      }

      // if the pellet is stopped
      if (  ( lcdReader->getCurrentOperatingMode() == OperatingMode::off )
         && ! shutdownPeriod
	 && ( ( dhtReader->getTemp() < ( 19.0 - dhtReader->getHumi() / 30.0 ) )
            && !( openWeatherClient->isClearForcast() && 20.0 - dhtReader->getTemp() <= 2 ) )
         )
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


