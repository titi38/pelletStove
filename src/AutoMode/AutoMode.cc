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

#include "rapidjson/document.h"    	// rapidjson's DOM-style API
#include "rapidjson/prettywriter.h"  	// for stringify JSON
using namespace rapidjson;

#include "AutoMode.hh"


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
    while ( !exiting )
    {
    }
  }

  /***********************************************************************/

  void AutoMode::start(AutoMode::Mode m)
  {

  }

  /***********************************************************************/

  void AutoMode::stop()
  {

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


