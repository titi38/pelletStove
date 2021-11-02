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
#include <unistd.h>
#include "libnavajo/LogRecorder.hh"

#include "rapidjson/document.h"      // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h"    // for stringify JSON

#include "Gauge.hh"


using namespace rapidjson;

/***********************************************************************/

Gauge::Gauge ()
{
  if ( wiringPiSetup () == -1 )
    exit (1);

  exiting = false;
  thread_loop = new thread (&Gauge::loop, this);
  NVJ_LOG->append (NVJ_INFO, "Gauge is starting");

  pinMode (static_cast<int>(Sensors::trigger), OUTPUT);
  pinMode (static_cast<int>(Sensors::echo), INPUT);

  digitalWrite (static_cast<int>(Sensors::trigger), LOW);

}

/***********************************************************************/

Gauge::~Gauge ()
{
  exiting = true;
  thread_loop->join ();
  NVJ_LOG->append (NVJ_INFO, "Gauge is finished");
}

/***********************************************************************/

string Gauge::getInfoJson () const
{
  string resultat = "";
  GenericStringBuffer <UTF8<>> buffer;
  Writer <GenericStringBuffer<UTF8<> >> writer (buffer);
  writer.StartObject ();
  writer.String ("remaining");
  writer.Double (getLevel ());
  writer.EndObject ();
  resultat = buffer.GetString ();
  buffer.Clear ();
  return resultat;
}

/***********************************************************************/

void Gauge::readMesure ()
{
  chrono::steady_clock::time_point startEcho, endEcho;
  double distance = .0;

  digitalWrite (static_cast<int>(Sensors::trigger), HIGH);
  usleep (10); // wait 10us
  digitalWrite (static_cast<int>(Sensors::trigger), LOW);

  bool timeout = false;
  size_t nbLow = 0, nbHigh = 0;


  while ( digitalRead (static_cast<int>(Sensors::echo)) == LOW && !timeout )
  { timeout = nbLow++ > 50000; }; // avg:5000

  startEcho = chrono::steady_clock::now ();

  while ( digitalRead (static_cast<int>(Sensors::echo)) == HIGH && !timeout ) // echo received
  { timeout = nbHigh++ > 500000; }; // avg:50000

  endEcho = chrono::steady_clock::now ();

  if ( timeout )
  {
    NVJ_LOG->append (NVJ_INFO, "Gauge: TIMEOUT ! nbLow=" + to_string (nbLow) + ", nbHigh=" + to_string (nbHigh));
    return;
  }

  histDistance[ nbDistance % GAUGE_NBVAL ] =
      chrono::duration_cast<chrono::nanoseconds> (endEcho - startEcho).count () * 1e-9 * 17150;

  NVJ_LOG->append (NVJ_DEBUG, "Gauge last mesure : dist=" + to_string (getAvgDistance ()) + "cm, remaining="
                              + to_string (getLevel ()) + "Kg" + ", lastReadDist="
                              + to_string (histDistance[ nbDistance % GAUGE_NBVAL ]) + ", nbLow=" + to_string (nbLow)
                              + ", nbHigh=" + to_string (nbHigh));
  nbDistance++;

}

/***********************************************************************/

double Gauge::getAvgDistance () const
{
  double sum = .0;
  size_t i = 0;
  if ( !nbDistance ) return 0;
  size_t nbVal = ( nbDistance > GAUGE_NBVAL ) ? GAUGE_NBVAL : nbDistance;
  for ( size_t i = 0; i < nbVal; i++ )
    sum += histDistance[ i ];
  return sum / nbVal;
}

/***********************************************************************/

double Gauge::getLevel () const
{
  const double topLevel = 15.0; // cm
  const double capacity = 60;  // Kg
  const double canisterHeight = 66; //cm

  double avgDist = getAvgDistance ();

  double res = capacity * ( 1 - ( avgDist - topLevel ) / canisterHeight ); // in Kg

  if ( res > 60 ) return 60;
  if ( res < 0 ) return 0;
  return res;
}

/***********************************************************************/

void Gauge::loop ()
{
  while ( !exiting )
  {
    sleep (5);
    readMesure ();
  }
}

/***********************************************************************/
/*
  int main()
  {
     Gauge gauge;
     sleep(10000); 

     return 0;
  }
*/
