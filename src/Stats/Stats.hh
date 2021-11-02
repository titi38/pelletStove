//------------------------------------------------------------------------------
//
// Stats.hh : Collect Temp/Hygro/Run statistics
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

#ifndef STATS_HH_
#define STATS_HH_

#include <string>
#include "libnavajo/libnavajo.hh"


using namespace std;


/**
* Stats - generic class to collect statistics
*/
class Stats
{
    volatile double sumTemp = 0, sumHumi = 0, sumRun = 0;
    volatile int nbHTVal = 0, nbRunVal = 0;
    volatile int nbStats = 0, idxStats = 0;
    volatile double dailyStats[24][4][3]; // 24hours x 4 quartersOfHour x 3 values(temp/hum/run)

    static Stats *theStats;

  protected:
    Stats ();
    virtual ~Stats ();

  public:
    /**
     * getInstance is part of the GeoIpListRecorder Singleton implementation
     * @return the uniq instance of GeoIpListRecorder
     */
    inline static Stats* getInstance()
    {
      if (theStats == NULL)
        theStats = new GeoIpListRecorder;

      return Stats;
    };

    inline static void freeInstance()
    {
      if (theStats != nullptr)
        delete theStats;
    };

    void updateIfEndQuarter();
    void updateHTStats (double temp, double humi);
    void updateRunStats ( unsigned char runMode);

    string getStatsJson () const;

};


#endif
