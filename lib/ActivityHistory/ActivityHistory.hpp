#ifndef ACTIVITY_HISTORY_H
#define ACTIVITY_HISTORY_H

#include <Arduino.h>

struct ActivityRecord {
    uint32_t ActivityId;
    uint64_t startUnixTime;
    uint64_t stopUnixTime;
};

namespace ActivityHistory {
    inline constexpr const char* HISTORY_PATH = "/history.bin";

    size_t getRecordCount();
    void appendRecord(const ActivityRecord& record);
    bool readRecord(size_t index, ActivityRecord& outRecord);
    void clearHistory();

    uint64_t getFromToActivityDuration(uint32_t activityId,
                                       uint64_t fromUnixTime,
                                       uint64_t toUnixTime);
}

#endif
