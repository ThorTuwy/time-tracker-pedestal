#include "ActivityTracker.hpp"

ActivityTracker::ActivityTracker(const char* binaryPath)
    : binaryPath(binaryPath) {}

bool ActivityTracker::begin() {
    if (!LittleFS.begin(true)) {
        return false;
    }

    preferences.begin("ev_tracker");

    state = (ActivityState) preferences.getUChar("state", STATE_IDLE);
    currentActivityId = preferences.getUInt("crtActivityId", 0);
    startUnixTime = preferences.getULong64("startUnixTime", 0);
    uint64_t stopUnixTime = preferences.getULong64("stopUnixTime", 0);

    if(state == STATE_ACTIVE){
        ActivityRecord newRecord;
        newRecord.ActivityId = currentActivityId;
        newRecord.startUnixTime = startUnixTime;
        newRecord.stopUnixTime = stopUnixTime;

        appendRecord(newRecord);

        state = STATE_IDLE;
        preferences.putUChar("state", state);
    }

    return true;
}

void ActivityTracker::stopActivity() {
    if(state != STATE_ACTIVE) return;

    ActivityRecord newRecord;
    newRecord.ActivityId = currentActivityId;
    newRecord.startUnixTime = startUnixTime;
    newRecord.stopUnixTime = getUnixTime();

    appendRecord(newRecord);

    state = STATE_IDLE;
    preferences.putUChar("state", state);
}

void ActivityTracker::updateActivity(uint32_t activityId) {
    if(state==STATE_ACTIVE and activityId==currentActivityId) return;

    stopActivity();

    state = STATE_ACTIVE;
    currentActivityId = activityId;
    startUnixTime = getUnixTime();

    preferences.putUChar("state", state);
    preferences.putUInt("crtActivityId", currentActivityId);
    preferences.putULong64("startUnixTime", startUnixTime);
    preferences.putULong64("stopUnixTime", startUnixTime);
}


void ActivityTracker::updateStopTime() {
    if (getUnixTime()-lastStopUnixTime>=STOP_TIME_UPDATE_INTERVAL){
        lastStopUnixTime=getUnixTime();
        preferences.putULong64("stopUnixTime", lastStopUnixTime);
    }

}

void ActivityTracker::appendRecord(const ActivityRecord &record) {
    File file = LittleFS.open(binaryPath, FILE_APPEND);
    if (!file) return;

    file.write((const uint8_t*)&record, sizeof(ActivityRecord));
    file.close();
}

size_t ActivityTracker::getRecordCount() {
    File file = LittleFS.open(binaryPath, FILE_READ);
    if (!file) return 0;
    size_t count = file.size() / sizeof(ActivityRecord);
    file.close();
    return count;
}



bool ActivityTracker::readRecord(size_t index, ActivityRecord &outRecord) {
    File file = LittleFS.open(binaryPath, FILE_READ);
    if (!file) return false;

    size_t offset = index * sizeof(ActivityRecord);
    if (offset + sizeof(ActivityRecord) > file.size()) {
        file.close();
        return false;
    }

    file.seek(offset);
    size_t bytesRead = file.read((uint8_t*)&outRecord, sizeof(ActivityRecord));
    file.close();

    return bytesRead == sizeof(ActivityRecord);
}

void ActivityTracker::clearHistory() {
    LittleFS.remove(binaryPath);
}

uint64_t ActivityTracker::getUnixTime() {
    time_t now;
    time(&now);
    return (uint64_t)now;
}

bool ActivityTracker::isActivityActive() const { return state == STATE_ACTIVE; }
uint32_t ActivityTracker::getCurrentActivityId() const { return currentActivityId; }
uint64_t ActivityTracker::getCurrentActivityDuration() {
    if(state==STATE_IDLE) return 0;
    return getUnixTime()-startUnixTime;
}