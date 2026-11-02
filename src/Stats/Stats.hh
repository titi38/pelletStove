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
#include <mutex>
#include <ctime>
#include "libnavajo/libnavajo.hh"

#define FIRST_TIMESTAT 255

using namespace std;


/**
* Stats - generic class to collect statistics
*/
class Stats
{
    static Stats *theStats;
    static constexpr int kSlots = 96;
    mutable std::mutex mutex_;
    double sumsTemp_ = 0, sumsHumi_ = 0;
    int countHT_ = 0;
    double powerIntegral_ = 0; // power * seconds in current quarter
    double powerSeconds_ = 0;
    int currentPower_ = 0;
    bool hasPower_ = false;
    std::time_t lastPowerTime_ = 0;
    std::time_t quarterStart_ = 0;
    double dailyStats_[kSlots][3] = {};
    int nextSlot_ = 0, countSlots_ = 0;

    static std::time_t quarterStart(std::time_t now);
    void advanceLocked(std::time_t now);
    void accountPowerLocked(std::time_t until);
    void closeQuarterLocked();

  public:
    Stats() = default;
    ~Stats() = default;
    static Stats* getInstance();
    static void freeInstance(); // Do not call while worker threads are active.
    void updateIfEndQuarter();
    void updateHTStats(double temp, double humi);
    void updateRunStats(unsigned char power);
    std::string getStatsJson() const;
};

#endif
