#ifndef ACTIVITY_HISTORY_H
#define ACTIVITY_HISTORY_H

#include <Arduino.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <time.h>

#include <ActivityTracker.hpp>

class ActivityHistory {
public:
    ActivityHistory();

    size_t getRecordCount();
    bool readRecord(size_t index, ActivityRecord &outRecord);
    void clearHistory();

private:

};

#endif