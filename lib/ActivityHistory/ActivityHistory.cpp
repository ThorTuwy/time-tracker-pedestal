#include "ActivityHistory.hpp"

namespace ActivityHistory {

size_t getRecordCount() {
    File file = LittleFS.open(HISTORY_PATH, FILE_READ);
    if(!file) return 0;
    size_t count = file.size() / sizeof(ActivityRecord);
    file.close();
    return count;
}

bool readRecord(size_t index, ActivityRecord& outRecord) {
    File file = LittleFS.open(HISTORY_PATH, FILE_READ);
    if(!file) return false;

    size_t offset = index * sizeof(ActivityRecord);
    if(offset + sizeof(ActivityRecord) > file.size()) {
        file.close();
        return false;
    }

    file.seek(offset);
    size_t bytesRead = file.read((uint8_t*)&outRecord, sizeof(ActivityRecord));
    file.close();

    return bytesRead == sizeof(ActivityRecord);
}

uint64_t getFromToActivityDuration(uint32_t activityId,
                                   uint64_t fromUnixTime,
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

void clearHistory() { LittleFS.remove(HISTORY_PATH); }
}
