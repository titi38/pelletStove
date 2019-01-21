//------------------------------------------------------------------------------
//
// ButtonControl.cc : exec command
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
#include <stdio.h>
#include <string.h>
#include <wiringPi.h>
#include <ctype.h>
#include "libnavajo/libnavajo.hh"
#include "ButtonControl.hh"


  ButtonControl::ButtonControl()
  {
    if ( wiringPiSetup() == -1 )
      exit( 1 );

    initPinMode();
  }

  /***********************************************************************/

  void ButtonControl::initPinMode()
  {
    pinMode(static_cast<int>(ControlButtons::on), OUTPUT);
    pinMode(static_cast<int>(ControlButtons::off), OUTPUT);
    pinMode(static_cast<int>(ControlButtons::up), OUTPUT);
    pinMode(static_cast<int>(ControlButtons::down), OUTPUT);
  }

  /***********************************************************************/

  void ButtonControl::setButton(const ControlButtons b, const int state)
  {
    digitalWrite(static_cast<int>(b), state);
  }

  /***********************************************************************/

  void ButtonControl::pressButton(const ControlButtons b, 
		const ButtonPressionDuration duration)
  {
    setButton(b, HIGH);
    usleep(1000*500);
    if (duration != ButtonPressionDuration::quick) // Add a delay
    {
      usleep(1000*1500);
      if (duration != ButtonPressionDuration::normal) // Add a delay
        usleep(1000*1500);
    }
    setButton(b, LOW);
    usleep(1000*500);
  }

  /***********************************************************************/

  void ButtonControl::press2Buttons(const ControlButtons b1, const ControlButtons b2)
  {
    setButton(b1, HIGH);
    setButton(b2, HIGH);
    delay(2000); // 2seconds
    setButton(b1, LOW);
    setButton(b2, LOW);
    delay(500);
  }

  /***********************************************************************/

  void ButtonControl::goToMainMenu()
  {
    NVJ_LOG->append(NVJ_INFO, "goToMainMenu()");
    // start from main menu
    pressButton(ControlButtons::off);
    pressButton(ControlButtons::off);
    pressButton(ControlButtons::off);
    pressButton(ControlButtons::off);
  }

  /***********************************************************************/

  void ButtonControl::incPower(short step)
  {
    NVJ_LOG->append(NVJ_INFO, "incPower(" + to_string(step) + ")" );
    // start from main menu
    // increase puissance
    press2Buttons(ControlButtons::up, ControlButtons::down);
    pressButton(ControlButtons::on);
    pressButton(ControlButtons::on);

    for (int i=0; i<abs(step); i++)
      if (step > 0)
        pressButton(ControlButtons::up);
      else
        pressButton(ControlButtons::down);

    pressButton(ControlButtons::on);
    pressButton(ControlButtons::on);
    pressButton(ControlButtons::off);
    pressButton(ControlButtons::off);
  }

  /***********************************************************************/

  void ButtonControl::decPower(short step)
  {
    incPower(-step);
  }
  /***********************************************************************/

  void ButtonControl::incTempWater(short stepC)
  {
    NVJ_LOG->append(NVJ_INFO, "incTempWater(" + to_string(stepC) + ") - TO DO...");

    //TODO
  }

  /***********************************************************************/

  void ButtonControl::decTempWater(short stepC)
  {
    incTempWater(-stepC);
  }

  /***********************************************************************/

  void ButtonControl::removeAllActivePrograms()
  {
    NVJ_LOG->append(NVJ_INFO, "removeAllActivePrograms()");
    
  // disable all programs
    press2Buttons(ControlButtons::up,ControlButtons::down);
    pressButton(ControlButtons::down);
    pressButton(ControlButtons::on);
    pressButton(ControlButtons::down);
    pressButton(ControlButtons::on); 
    pressButton(ControlButtons::on);
    for (int i=1; i<=6; i++)
    {
 	pressButton(ControlButtons::down);
	pressButton(ControlButtons::on);
    }
    pressButton(ControlButtons::off);
    pressButton(ControlButtons::off);
    pressButton(ControlButtons::off);
  }

  /***********************************************************************/

  void ButtonControl::start()
  {
    NVJ_LOG->append(NVJ_INFO, "start()");
    pressButton(ControlButtons::on, ButtonPressionDuration::longer);
  }

  /***********************************************************************/

  void ButtonControl::stop()
  {
    NVJ_LOG->append(NVJ_INFO, "stop()");
    pressButton(ControlButtons::off, ButtonPressionDuration::longer);
  }

  /***********************************************************************/

  void ButtonControl::resetError()
  {
    NVJ_LOG->append(NVJ_INFO, "resetError()");
    pressButton(ControlButtons::off, ButtonPressionDuration::longer);
    delay(1000);
    pressButton(ControlButtons::off, ButtonPressionDuration::longer);
  }

  /***********************************************************************/

