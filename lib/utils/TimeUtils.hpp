#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <time.h>

namespace TimeUtils {

time_t getStartOfWeekUnixTime(time_t unixTime);
time_t getStartOfMonthUnixTime(time_t unixTime);

}  // namespace TimeUtils

#endif
