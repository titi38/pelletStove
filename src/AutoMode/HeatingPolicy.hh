#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>
#include <utility>

namespace pellet_policy {
enum class Mode { Manual, Automatic, Presence, Absence };
enum class Override { None, Arrive, Leave };
enum class Reason { Manual, Scheduled, Anticipation, AutomaticFrost, PresencePeriod, AbsenceFrost, OverrideArrive, OverrideLeave, Off };

struct Period {
  int startMinute = 0;
  int endMinute = 0;
  Period() = default;
  Period(int start, int end) : startMinute(start), endMinute(end) {}
};
struct Frost { bool enabled = false; double startBelow = 15.0; double stopAt = 17.0; };
struct Automatic {
  double comfort = 20.0;
  bool anticipationEnabled = false;
  double anticipationBelow = 17.0;
  int anticipationMinutes = 60;
  Frost frost;
  // Monday = 0 ... Sunday = 6; period endpoints in local minutes [0,1440].
  std::array<std::vector<Period>, 7> days;
};
struct Presence { int startMinute = 7*60; int endMinute = 23*60; double comfort = 19.5; };
struct Config { Automatic automatic; Presence presence; Frost absenceFrost; };
struct Input {
  Mode mode = Mode::Manual;
  Override override = Override::None;
  int weekday = 0; // Monday=0
  int minuteOfDay = 0;
  double indoorTemperature = 20.0; // raw indoor temperature, not weather-adjusted
  bool indoorSensorValid = true;
  Input() = default;
  Input(Mode m, Override o, int day, int minute, double temperature, bool valid)
    : mode(m), override(o), weekday(day), minuteOfDay(minute),
      indoorTemperature(temperature), indoorSensorValid(valid) {}
};
struct Decision {
  bool allowed = false;
  Reason reason = Reason::Off;
  double targetTemperature = 0.0;

  Decision() = default;

  Decision(bool allowed_, Reason reason_, double target_)
    : allowed(allowed_),
      reason(reason_),
      targetTemperature(target_) {}
};

inline void validate(const Config& c) {
  const auto finite = [](double t){ return std::isfinite(t) && t >= 5 && t <= 30; };
  const auto frost = [&](const Frost& f){
    if(!finite(f.startBelow) || !finite(f.stopAt) || f.startBelow >= f.stopAt)
      throw std::invalid_argument("Invalid frost thresholds");
  };
  if(!finite(c.automatic.comfort) || !finite(c.presence.comfort) || !finite(c.automatic.anticipationBelow))
    throw std::invalid_argument("Invalid temperatures");
  frost(c.automatic.frost); frost(c.absenceFrost);
  if(c.automatic.anticipationMinutes < 0 || c.automatic.anticipationMinutes > 180)
    throw std::invalid_argument("Invalid anticipation minutes");
  if(c.presence.startMinute < 0 || c.presence.startMinute > 1439 ||
     c.presence.endMinute < 0 || c.presence.endMinute > 1440 ||
     c.presence.startMinute == c.presence.endMinute)
    throw std::invalid_argument("Invalid presence period");
  for(const auto& day: c.automatic.days)
    for(const auto& p: day)
      if(p.startMinute < 0 || p.endMinute > 1440 || p.startMinute >= p.endMinute)
        throw std::invalid_argument("Invalid automatic period");
}

// Stateless period membership, allowing the daily Presence interval to cross midnight.
inline bool inPresencePeriod(const Presence& p, int minute) {
  if(p.startMinute < p.endMinute) return minute >= p.startMinute && minute < p.endMinute;
  return minute >= p.startMinute || minute < p.endMinute;
}
inline bool scheduled(const Automatic& a, int day, int minute) {
  for(const auto& p : a.days[day]) if(minute >= p.startMinute && minute < p.endMinute) return true;
  return false;
}
inline bool anticipating(const Automatic& a, int day, int minute, double temp) {
  if(!a.anticipationEnabled || temp >= a.anticipationBelow) return false;
  // Look ahead from now up to max anticipation across midnight and the week boundary.
  for(int offset = 1; offset <= a.anticipationMinutes; ++offset) {
    const int absolute = minute + offset;
    const int newDay = (day + absolute/1440) % 7;
    const int newMinute = absolute%1440;
    for(const auto& p: a.days[newDay]) if(p.startMinute == newMinute) return true;
  }
  return false;
}

// One latch per independent frost-protection mode: no chatter between thresholds.
class Policy {
public:
  explicit Policy(Config cfg) : cfg_(std::move(cfg)) { validate(cfg_); }
  void update(Config cfg) { validate(cfg); cfg_=std::move(cfg); autoFrost_=false; absenceFrost_=false; }
  void reset() { autoFrost_=false; absenceFrost_=false; }
  Decision decide(const Input& in) {
    if(in.weekday < 0 || in.weekday > 6 || in.minuteOfDay < 0 || in.minuteOfDay >= 1440)
      return {};
    if(in.mode == Mode::Manual) { reset(); return {false,Reason::Manual,0}; }
    if(!in.indoorSensorValid || !std::isfinite(in.indoorTemperature) ||
       in.indoorTemperature < -30 || in.indoorTemperature > 60) {
      // A failed sensor must not create a new automatic heating authorization.
      reset(); return {};
    }
    const auto hysteresis = [&](bool& latched, const Frost& f) {
      if(!f.enabled) {latched=false;return false;}
      if(latched) {if(in.indoorTemperature >= f.stopAt) latched=false;}
      else if(in.indoorTemperature < f.startBelow) latched=true;
      return latched;
    };
    const bool af = hysteresis(autoFrost_,cfg_.automatic.frost);
    const bool vf = hysteresis(absenceFrost_,cfg_.absenceFrost);
    if(in.mode == Mode::Presence) {
      if(inPresencePeriod(cfg_.presence,in.minuteOfDay)) return {true,Reason::PresencePeriod,cfg_.presence.comfort};
      return {};
    }
    if(in.mode == Mode::Absence) {
      if(vf) return {true,Reason::AbsenceFrost,cfg_.absenceFrost.stopAt};
      return {};
    }
    if(in.mode == Mode::Automatic) {
      // Frost protection remains available even during a temporary 'leave' override.
      if(af) return {true,Reason::AutomaticFrost,cfg_.automatic.frost.stopAt};
      if(in.override == Override::Leave) return {false,Reason::OverrideLeave,0};
      if(in.override == Override::Arrive) return {true,Reason::OverrideArrive,cfg_.automatic.comfort};
      if(scheduled(cfg_.automatic,in.weekday,in.minuteOfDay)) return {true,Reason::Scheduled,cfg_.automatic.comfort};
      if(anticipating(cfg_.automatic,in.weekday,in.minuteOfDay,in.indoorTemperature))
        return {true,Reason::Anticipation,cfg_.automatic.comfort};
    }
    return {};
  }
private:
  Config cfg_;
  bool autoFrost_ = false;
  bool absenceFrost_ = false;
};
} // namespace pellet_policy
