#ifndef ACTIVITY_TRACKER_H
#define ACTIVITY_TRACKER_H

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

inline constexpr const char* HISTORY_PATH = "/history.bin";

class ActivityTracker {
public:
    ActivityTracker();

    bool begin();
    void updateActivity(uint32_t ActivityId);
    void stopActivity();
    void updateStopTime();

    bool isActivityActive() const;
    uint32_t getCurrentActivityId() const;
    uint64_t getCurrentActivityDuration();
private:
    static constexpr uint8_t STOP_TIME_UPDATE_INTERVAL = 60;

    Preferences preferences;
    ActivityState state;
    uint32_t currentActivityId;
    uint64_t startUnixTime;
    uint64_t lastStopUnixTime;

    uint64_t getUnixTime();
    void appendRecord(const ActivityRecord &record);
};

#endif