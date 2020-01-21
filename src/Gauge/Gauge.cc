//------------------------------------------------------------------------------
//
// Gauge.hh : exec command
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

#include <chrono>
#include <iostream>
#include <wiringPi.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <stdint.h>
#include <unistd.h>
//#include <math.h>

//#include "rapidjson/document.h"    	// rapidjson's DOM-style API
//#include "rapidjson/prettywriter.h"  	// for stringify JSON

#include "Gauge.hh"



  /***********************************************************************/

  Gauge::Gauge()
  {
    if ( wiringPiSetup() == -1 )
      exit( 1 );

    exiting = false;
    thread_loop=new thread(&Gauge::loop, this);
    //NVJ_LOG->append(NVJ_INFO, "Gauge is starting" );

    pinMode(static_cast<int>(Sensors::trigger), OUTPUT);
    pinMode(static_cast<int>(Sensors::echo), INPUT);

    digitalWrite( static_cast<int>(Sensors::trigger), LOW );
  }

  /***********************************************************************/
 
  Gauge::~Gauge()
  {
    exiting = true;
    thread_loop->join();
  }

  /***********************************************************************/
 
  string Gauge::getInfoJson() const
  { 
    string resultat = "";
    return resultat;     
  }

  /***********************************************************************/

  void Gauge::readMesure()
  {
    chrono::steady_clock::time_point end, start;
    double distance = .0;

    digitalWrite( static_cast<int>(Sensors::trigger), HIGH );
    usleep(10); // wait 10us

    digitalWrite( static_cast<int>(Sensors::trigger), LOW );

    while ( digitalRead( static_cast<int>(Sensors::echo) ) == LOW ) // start emission
      start = chrono::steady_clock::now();

    while ( digitalRead( static_cast<int>(Sensors::echo) ) == HIGH ) // echo received
      end = chrono::steady_clock::now();

    histDistance[nbDistance%GAUGE_NBVAL] = chrono::duration_cast<chrono::nanoseconds>(end - start).count() * 1e-9 * (33100 + .6 * temperature) / 2;
    nbDistance++;

    cout << "La distance moyenne est de: " << getAvgDistance() << "cm" << endl;
  } 

  /***********************************************************************/

  double Gauge::getAvgDistance()
  {
    double sum = .0;
    size_t i = 0;
    size_t nbVal=(nbDistance>GAUGE_NBVAL)?GAUGE_NBVAL:nbDistance;
    for (size_t i=0; i < nbVal; i++)
      sum += histDistance[i];
    return sum / nbVal;
  }

  /***********************************************************************/ 

  void Gauge::loop()
  { 
    while ( !exiting )
    {
      sleep(1);
      readMesure();
    }
  }

  /***********************************************************************/

  int main()
  {
     Gauge gauge;
     sleep(100); 

     return 0;
  }
