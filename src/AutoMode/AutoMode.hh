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
#include <mutex>
#include <array>
#include <vector>
#include <atomic>

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
    enum class Mode : int
    {
        off, normal, vacation, absent, custom
    };
    void start ( Mode m );
    void stop ();
    std::string getInfoJson () const;
    // Programmation hebdomadaire - lundi=0, dimanche=6
    struct PresencePeriod { int start; int end; }; // minutes depuis 00:00
    std::string getScheduleJson () const;
    bool saveScheduleJson (const std::string &json, std::string &error);
    bool setPresenceOverride (const std::string &value, std::string &error);

    AutoMode ( ButtonControl *bc, DhtReader *dht, LcdReader *lcd, OpenWeatherClient *owc );
    ~AutoMode ();

  private:
    void loop ();

    mutable std::mutex scheduleMutex;
    std::array<std::vector<PresencePeriod>, 7> presenceDays;
    double comfortTemperature = 20.0;
    enum class PresenceOverride { none, arrive, leave };
    PresenceOverride presenceOverride = PresenceOverride::none;
    bool overrideOriginalPresence = false;
    std::string schedulePath;
    bool isScheduledPresence(const std::tm &now) const;
    bool saveScheduleToDisk(const std::array<std::vector<PresencePeriod>, 7> &days,
        double comfort, std::string &error) const;
    void loadScheduleFromDisk();
    bool applyScheduleJson(const std::string &json, std::string &error, bool persist);
    static std::string timeString(int minutes);
    static bool parseTime(const std::string &value, int &minutes);

    volatile Mode currentMode = Mode::off;
    ButtonControl *buttonControl = nullptr;
    DhtReader *dhtReader = nullptr;
    LcdReader *lcdReader = nullptr;
    OpenWeatherClient *openWeatherClient = nullptr;

    thread *thread_loop = nullptr;
    volatile bool exiting = false;
};


#endif
