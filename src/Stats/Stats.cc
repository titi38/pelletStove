//------------------------------------------------------------------------------
//
// Stats.cc : Collect Statistics
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
// 1        | 1   | 01/10/21 | T.DESCOMBES | source creation
//----------+-----+----------+-------------+------------------------------------
//
//           Detailed modification operations on this source
//------------------------------------------------------------------------------
//  Date  :                            Author :
//  REL   :
//  Description :
//
//------------------------------------------------------------------------------

#include "rapidjson/document.h"      // rapidjson's DOM-style API
#include "rapidjson/prettywriter.h"    // for stringify JSON

#include "Stats.hh"
#include <ctime>
#include <chrono>

using namespace rapidjson;

Stats *Stats::theStats = nullptr;

/***********************************************************************/

Stats::Stats ()
{
  nbStats = 0;
  idxStats = 0;
  sumTemp = 0;
  sumHumi = 0;
  nbHTVal = 0;
  sumRun = 0;
  nbRunVal = 0;
}

/***********************************************************************/

Stats::~Stats ()
{
}

/***********************************************************************/

void Stats::updateIfEndQuarter ()
{
  std::time_t tt = std::chrono::system_clock::to_time_t (std::chrono::system_clock::now ());
  std::tm *tm = std::localtime (&tt);

  if (( currentTimeStat != tm->tm_min / 15 ) && (currentTimeStat != FIRST_TIMESTAT ) ) // update prev Quarter
  {
    dailyStats[ idxStats ][ 0 ] = (nbHTVal>0) ? sumTemp / nbHTVal : 0;
    dailyStats[ idxStats ][ 1 ] = (nbHTVal>0) ? sumHumi / nbHTVal : 0;
    dailyStats[ idxStats ][ 2 ] = (nbRunVal>0) ? sumRun / nbRunVal : 0;

    sumTemp = 0;
    sumHumi = 0;
    nbHTVal = 0;

    sumRun = 0;
    nbRunVal = 0;

    idxStats = ( idxStats + 1 ) % ( 24 * 4 ); // point to the next stat quarter
    if ( nbStats < 24 * 4 )
      nbStats++;

    currentTimeStat = tm->tm_min / 15;
  }

  if ( (currentTimeStat == FIRST_TIMESTAT ) )
    currentTimeStat = tm->tm_min / 15;
 
}

/***********************************************************************/

void Stats::updateHTStats ( double temp, double humi )
{
  updateIfEndQuarter ();

  sumTemp += temp ;
  sumHumi += humi ;
  nbHTVal++;
}

/***********************************************************************/

void Stats::updateRunStats ( unsigned char runMode )
{
  updateIfEndQuarter ();

  sumRun += runMode;
  nbRunVal++;
}


/***********************************************************************/

string Stats::getStatsJson () const
{
  std::time_t tt = std::chrono::system_clock::to_time_t (std::chrono::system_clock::now ());
  std::tm *tm = std::localtime (&tt);

  string resultat = "";
  GenericStringBuffer <UTF8<>> buffer;
  Writer <GenericStringBuffer<UTF8<> >> writer (buffer);
  writer.SetMaxDecimalPlaces(2);
  writer.StartObject ();

  writer.String ("hour");
  writer.Int (tm->tm_hour);
  writer.String ("quarter");
  writer.Int (currentTimeStat);

  writer.String ("stats");
  writer.StartArray ();
  if ( nbStats )
  {
    for ( int idx = ( idxStats - nbStats ) % ( 24 * 4 ); idx != idxStats; idx++ )
    {
      writer.StartArray ();
      writer.Double (dailyStats[ idx ][ 0 ]); // Temp
      writer.Double (dailyStats[ idx ][ 1 ]); // Humidity
      writer.Double (dailyStats[ idx ][ 2 ]); // Run
      writer.EndArray ();
    }
  }
  writer.EndArray ();
  writer.EndObject ();
  resultat = buffer.GetString ();
  buffer.Clear ();
  return resultat;
}
