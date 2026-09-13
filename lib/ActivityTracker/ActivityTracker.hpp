#ifndef Activity_TRACKER_H
#define Activity_TRACKER_H

#include <Arduino.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <time.h>

enum ActivityState : uint8_t {
    STATE_IDLE = 0,
    STATE_ACTIVE = 1
};

struct ActivityRecord {
    uint32_t ActivityId;
    uint64_t startUnixTime;
    uint64_t stopUnixTime;
};

class ActivityTracker {
public:
    ActivityTracker(const char* binaryPath = "/history.bin");

    bool begin();
    void updateActivity(uint32_t ActivityId);
    void stopActivity();
    void updateStopTime();

    bool isActivityActive() const;
    uint32_t getCurrentActivityId() const;
    uint64_t getCurrentActivityDuration();


    size_t getRecordCount();
    bool readRecord(size_t index, ActivityRecord &outRecord);
    void clearHistory();
    
private:
    static constexpr uint8_t STOP_TIME_UPDATE_INTERVAL = 60;

    const char* binaryPath;

    Preferences preferences;
    ActivityState state;
    uint32_t currentActivityId;
    uint64_t startUnixTime;
    uint64_t lastStopUnixTime;


    uint64_t getUnixTime();
    void appendRecord(const ActivityRecord &record);
};

#endif