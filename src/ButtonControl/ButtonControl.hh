//------------------------------------------------------------------------------
//
// ButtonControl.hh : Press button of pellet stove controller
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

#ifndef BUTTONCONTROL_HH_
#define BUTTONCONTROL_HH_

#include <string>

using namespace std;


  /**
  * ButtonControl - generic class to handle the relay
  */
  class ButtonControl
  {
    enum class ControlButtons : int { on=4, off=2, up=5, down=3 /*Arduino: on=6, off=4, up=7, down=5*/  } ;
    enum class ButtonPressionDuration { quick, normal, longer };

    void initPinMode();
    void setButton(const ControlButtons b, const int state);
    void pressButton(const ControlButtons b, 
		     const ButtonPressionDuration duration=ButtonPressionDuration::quick);
    void press2Buttons(const ControlButtons b1, const ControlButtons b2);


    public:
      ButtonControl();

      void goToMainMenu();
      void incPower(short step);
      void decPower(short step);
      void incTempWater(short stepC);
      void decTempWater(short stepC);

      void removeAllActivePrograms();
      void start();
      void stop();

  };
  

#endif
