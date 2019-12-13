//------------------------------------------------------------------------------
//
// AutoMode.hh : Press button of pellet stove controller
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

#ifndef AUTOMODE_HH_
#define AUTOMODE_HH_

#include <string>
#include <thread>

#include "libnavajo/libnavajo.hh"
#include "libnavajo/LogStdOutput.hh"

#include "ButtonControl.hh"
#include "DhtReader.hh"
#include "LcdReader.hh"
#include "OpenWeatherClient.hh"

using namespace std;

  /**
  * AutoMode
  */
  class AutoMode
  {
    public:
      enum class Mode : int { off, normal, vacation, absent, custom };
      void start(Mode m);
      void stop();
      std::string getInfoJson() const;
      AutoMode(ButtonControl *bc, DhtReader *dht, LcdReader *lcd, OpenWeatherClient* owc);
      ~AutoMode();

    private:
      void loop();

      volatile Mode currentMode=Mode::off;
      ButtonControl *buttonControl=nullptr;
      DhtReader *dhtReader=nullptr;
      LcdReader *lcdReader=nullptr;
      OpenWeatherClient* openWeatherClient=nullptr;

      thread *thread_loop=nullptr;
      volatile bool exiting = false;
  };
  

#endif
