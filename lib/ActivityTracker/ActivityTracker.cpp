#include "ActivityTracker.hpp"

#include <ActivityHistory.hpp>
#include <time.h>

ActivityTracker::ActivityTracker() {}

bool ActivityTracker::begin() {
    trackerPreferences.begin("ev_tracker");

    state = (ActivityState)trackerPreferences.getUChar("state", STATE_IDLE);
    currentActivityId = trackerPreferences.getUInt("crtActivityId", 0);
    startUnixTime = trackerPreferences.getULong64("startUnixTime", 0);
    uint64_t stopUnixTime = trackerPreferences.getULong64("stopUnixTime", 0);
    lastStopUnixTime = stopUnixTime;

    if(state == STATE_ACTIVE) {
        ActivityRecord newRecord;
        newRecord.ActivityId = currentActivityId;
        newRecord.startUnixTime = startUnixTime;
        newRecord.stopUnixTime = stopUnixTime;

        ActivityHistory::appendRecord(newRecord);

        state = STATE_IDLE;
        trackerPreferences.putUChar("state", state);
    }

    return true;
}

void ActivityTracker::stopActivity() {
    if(state != STATE_ACTIVE) return;

    ActivityRecord newRecord;
    newRecord.ActivityId = currentActivityId;
    newRecord.startUnixTime = startUnixTime;
    newRecord.stopUnixTime = getUnixTime();

    ActivityHistory::appendRecord(newRecord);

    state = STATE_IDLE;
    trackerPreferences.putUChar("state", state);
}

void ActivityTracker::updateActivity(uint32_t activityId) {
    if(state == STATE_ACTIVE and activityId == currentActivityId){
        updateStopTime();
        return;
    };

    stopActivity();

    state = STATE_ACTIVE;
    currentActivityId = activityId;
    startUnixTime = getUnixTime();
    lastStopUnixTime = startUnixTime;

    trackerPreferences.putUChar("state", state);
    trackerPreferences.putUInt("crtActivityId", currentActivityId);
    trackerPreferences.putULong64("startUnixTime", startUnixTime);
    trackerPreferences.putULong64("stopUnixTime", startUnixTime);
}

void ActivityTracker::updateStopTime() {
    if(state != STATE_ACTIVE) return;

    if(getUnixTime() - lastStopUnixTime >= STOP_TIME_UPDATE_INTERVAL) {
        lastStopUnixTime = getUnixTime();
        trackerPreferences.putULong64("stopUnixTime", lastStopUnixTime);
    }
}

uint64_t ActivityTracker::getUnixTime() {
    time_t now;
    time(&now);
    return (uint64_t)now;
}

bool ActivityTracker::isActivityActive() const { return state == STATE_ACTIVE; }
uint32_t ActivityTracker::getCurrentActivityId() const {
    return currentActivityId;
}
uint64_t ActivityTracker::getCurrentActivityDuration() {
    if(state == STATE_IDLE) return 0;
    return getUnixTime() - startUnixTime;
}
