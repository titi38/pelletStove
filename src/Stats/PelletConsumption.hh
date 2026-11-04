#ifndef PELLET_CONSUMPTION_HH_
#define PELLET_CONSUMPTION_HH_

// Estimated feed rate in kg/h associated with each LCD power level.
// Shared by the state endpoint and the historical statistics.
// Index 0 corresponds to a stopped stove (no pellet feed).
static inline double pelletKgPerHour(int power)
{
    static const double kgPerHour[7] = {
        0.0,
        3.0 * 3.1 / 14.1,
        3.0 * 3.1 / 8.4,
        3.0 * 3.1 / 5.9,
        3.0 * 3.1 / 4.5,
        3.0 * 3.1 / 3.9,
        3.0 * 3.1 / 3.5
    };
    return power >= 0 && power <= 6 ? kgPerHour[power] : 0.0;
}

#endif
