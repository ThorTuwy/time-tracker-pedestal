#include "ActivityHistory.hpp"

#include <LittleFS.h>

namespace ActivityHistory {

Preferences historyPreferences;
u32_t nextRecordPosition = 0;

bool begin() {
    bool filesystemInitialized = LittleFS.begin(true);
    if(!filesystemInitialized) return false;

    bool preferencesInitialized = historyPreferences.begin("activityHistory");
    if(!preferencesInitialized) return false;

    nextRecordPosition = historyPreferences.getUInt("nextPosition", 0) %
                         CIRCULAR_BUFFER_RECORD_SIZE;

    if(!LittleFS.exists(HISTORY_PATH)) {
        nextRecordPosition = 0;
        historyPreferences.putUInt("nextPosition", nextRecordPosition);
    }

    return true;
}

size_t getRecordCount() {
    File file = LittleFS.open(HISTORY_PATH, FILE_READ);
    if(!file) return 0;
    size_t count = file.size() / sizeof(ActivityRecord);
    file.close();
    return count;
}

void appendRecord(const ActivityRecord& record) {
    if(!LittleFS.exists(HISTORY_PATH)) {
        File newFile = LittleFS.open(HISTORY_PATH, FILE_WRITE);
        if(!newFile) return;
        newFile.close();
    }

    File file = LittleFS.open(HISTORY_PATH, "r+");
    if(!file) return;

    size_t offset = nextRecordPosition * sizeof(ActivityRecord);
    if(!file.seek(offset)) {
        file.close();
        return;
    }

    size_t bytesWritten =
        file.write((const uint8_t*)&record, sizeof(ActivityRecord));
    file.close();
    if(bytesWritten != sizeof(ActivityRecord)) return;

    nextRecordPosition = (nextRecordPosition + 1) % CIRCULAR_BUFFER_RECORD_SIZE;
    historyPreferences.putUInt("nextPosition", nextRecordPosition);
}

bool readRecord(size_t index, ActivityRecord& outRecord) {
    if(index >= CIRCULAR_BUFFER_RECORD_SIZE) return false;

    File file = LittleFS.open(HISTORY_PATH, FILE_READ);
    if(!file) return false;

    size_t offset = index * sizeof(ActivityRecord);
    if(offset + sizeof(ActivityRecord) > file.size()) {
        // We are transparent in the implementation the circular buffer
        // Soo positions that do not relly exist, but could exist return
        // correctly but empty
        file.close();
        outRecord.ActivityId = 0;
        outRecord.startUnixTime = 0;
        outRecord.stopUnixTime = 0;
        return true;
    }

    file.seek(offset);
    size_t bytesRead = file.read((uint8_t*)&outRecord, sizeof(ActivityRecord));
    file.close();

    return bytesRead == sizeof(ActivityRecord);
}

uint64_t getFromToActivityDuration(uint32_t activityId, uint64_t fromUnixTime,
                                   uint64_t toUnixTime) {
    uint64_t activityDuration = 0;

    File file = LittleFS.open(HISTORY_PATH, FILE_READ);
    if(!file) return 0;
    size_t count = file.size() / sizeof(ActivityRecord);

    ActivityRecord currentRecord;
    for(size_t i = 0; i < count; i++) {
        size_t bytesRead =
            file.read((uint8_t*)&currentRecord, sizeof(ActivityRecord));
        if(bytesRead != sizeof(ActivityRecord)) break;

        if(currentRecord.ActivityId != activityId) continue;

        if(currentRecord.startUnixTime < fromUnixTime) {
            if(currentRecord.stopUnixTime <= fromUnixTime) continue;
            activityDuration += currentRecord.stopUnixTime - fromUnixTime;
        };

        if(currentRecord.stopUnixTime > toUnixTime) {
            if(currentRecord.startUnixTime >= toUnixTime) continue;
            activityDuration += toUnixTime - currentRecord.startUnixTime;
        };

        activityDuration +=
            currentRecord.stopUnixTime - currentRecord.startUnixTime;
    }

    return activityDuration;
}

void clearHistory() {
    LittleFS.remove(HISTORY_PATH);
    nextRecordPosition = 0;
    historyPreferences.putUInt("nextPosition", nextRecordPosition);
}
}  // namespace ActivityHistory
