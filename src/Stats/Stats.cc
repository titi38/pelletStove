#include "Stats.hh"
#include "rapidjson/prettywriter.h"
#include <chrono>
#include <algorithm>
#include <cmath>

using namespace rapidjson;

Stats* Stats::theStats = nullptr;

Stats* Stats::getInstance()
{
    // Construct on first use, thread-safe since C++11. Lifetime is process-wide.
    static Stats instance;
    return &instance;
}

void Stats::freeInstance()
{
    // Compatibility method. Singleton is process-owned and must not be deleted
    // while DHT/LCD/HTTP worker threads are running.
}

std::time_t Stats::quarterStart(std::time_t now)
{
    // A 15-minute interval is always 900 seconds (including DST transitions).
    return now - ((now % 900 + 900) % 900);
}

void Stats::closeQuarterLocked()
{
    dailyStats_[nextSlot_][0] = countHT_ ? sumsTemp_ / countHT_ : 0;
    dailyStats_[nextSlot_][1] = countHT_ ? sumsHumi_ / countHT_ : 0;
    // If power was not yet observed, return zero; otherwise average over
    // observed seconds, not LCD message counts.
    dailyStats_[nextSlot_][2] = powerSeconds_ > 0 ? powerIntegral_ / powerSeconds_ : 0;
    nextSlot_ = (nextSlot_ + 1) % kSlots;
    countSlots_ = (countSlots_ < kSlots ? countSlots_ + 1 : kSlots);
    sumsTemp_ = sumsHumi_ = 0;
    countHT_ = 0;
    powerIntegral_ = powerSeconds_ = 0;
}

void Stats::accountPowerLocked(std::time_t until)
{
    if (!hasPower_ || until <= lastPowerTime_) return;
    // The known power remains in effect until the next LCD update.
    double seconds = static_cast<double>(until - lastPowerTime_);
    powerIntegral_ += seconds * currentPower_;
    powerSeconds_ += seconds;
    lastPowerTime_ = until;
}

void Stats::advanceLocked(std::time_t now)
{
    if (!quarterStart_) {
        quarterStart_ = quarterStart(now);
        if (hasPower_) lastPowerTime_ = now;
        return;
    }
    if (now < quarterStart_) { // clock corrected backwards
        quarterStart_ = quarterStart(now);
        sumsTemp_ = sumsHumi_ = powerIntegral_ = powerSeconds_ = 0;
        countHT_ = 0;
        lastPowerTime_ = now;
        return;
    }
    // Avoid 1,000s of iterations after long downtime / clock jumps.
    if (now - quarterStart_ > kSlots * 900) {
        quarterStart_ = quarterStart(now);
        countSlots_ = nextSlot_ = 0;
        sumsTemp_ = sumsHumi_ = powerIntegral_ = powerSeconds_ = 0;
        countHT_ = 0;
        lastPowerTime_ = now;
        return;
    }
    while (now >= quarterStart_ + 900) {
        accountPowerLocked(quarterStart_ + 900);
        closeQuarterLocked();
        quarterStart_ += 900;
    }
    accountPowerLocked(now);
}

void Stats::updateIfEndQuarter()
{
    std::lock_guard<std::mutex> lock(mutex_);
    advanceLocked(std::time(nullptr));
}

void Stats::updateHTStats(double temp, double humi)
{
    std::lock_guard<std::mutex> lock(mutex_);
    advanceLocked(std::time(nullptr));
    if (!std::isfinite(temp) || !std::isfinite(humi)) return;
    sumsTemp_ += temp;
    sumsHumi_ += humi;
    ++countHT_;
}

void Stats::updateRunStats(unsigned char power)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto now = std::time(nullptr);
    advanceLocked(now); // account previous power through current time
    currentPower_ = std::min(static_cast<int>(power), 6);
    hasPower_ = true;
    lastPowerTime_ = now;
}

std::string Stats::getStatsJson() const
{
    // This method advances the quarter even if no sensors have reported lately.
    auto* self = const_cast<Stats*>(this);
    std::lock_guard<std::mutex> lock(mutex_);
    const auto now = std::time(nullptr);
    self->advanceLocked(now);
    std::tm tm{};
    localtime_r(&now, &tm);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    writer.SetMaxDecimalPlaces(2);
    writer.StartObject();
    writer.Key("hour"); writer.Int(tm.tm_hour);
    writer.Key("quarter"); writer.Int(tm.tm_min / 15);
    writer.Key("stats"); writer.StartArray();
    for (int n = 0; n < countSlots_; ++n) {
        const int idx = (nextSlot_ + kSlots - countSlots_ + n) % kSlots;
        writer.StartArray();
        for (int j = 0; j < 3; ++j) writer.Double(dailyStats_[idx][j]);
        writer.EndArray();
    }
    writer.EndArray();
    writer.EndObject();
    return buffer.GetString();
}
