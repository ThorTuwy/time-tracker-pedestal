#ifndef ACTIVITY_HISTORY_H
#define ACTIVITY_HISTORY_H

#include <Arduino.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <time.h>

#include <ActivityTracker.hpp>

class ActivityHistory {
   public:
    ActivityHistory();

    size_t getRecordCount();
    bool readRecord(size_t index, ActivityRecord& outRecord);
    void clearHistory();

    uint64_t getFromToActivityDuration(uint32_t activityId,
                                       uint64_t fromUnixTime,
                                       uint64_t toUnixTime);

   private:
};

#endif