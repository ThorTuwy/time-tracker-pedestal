#include "TimeUtils.hpp"

time_t TimeUtils::getStartOfWeekUnixTime(time_t unixTime) {
    struct tm localTime;
    if(localtime_r(&unixTime, &localTime) == nullptr) return (time_t)-1;

    localTime.tm_mday -= (localTime.tm_wday + 6) % 7;
    localTime.tm_hour = 0;
    localTime.tm_min = 0;
    localTime.tm_sec = 0;
    localTime.tm_isdst = -1;

    return mktime(&localTime);
}

time_t TimeUtils::getStartOfMonthUnixTime(time_t unixTime) {
    struct tm localTime;
    if(localtime_r(&unixTime, &localTime) == nullptr) return (time_t)-1;

    localTime.tm_mday = 1;
    localTime.tm_hour = 0;
    localTime.tm_min = 0;
    localTime.tm_sec = 0;
    localTime.tm_isdst = -1;

    return mktime(&localTime);
}
