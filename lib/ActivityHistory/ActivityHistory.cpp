#include "ActivityHistory.hpp"

ActivityHistory::ActivityHistory(){}

size_t ActivityHistory::getRecordCount() {
    File file = LittleFS.open(HISTORY_PATH, FILE_READ);
    if (!file) return 0;
    size_t count = file.size() / sizeof(ActivityRecord);
    file.close();
    return count;
}



bool ActivityHistory::readRecord(size_t index, ActivityRecord &outRecord) {
    File file = LittleFS.open(HISTORY_PATH, FILE_READ);
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

void ActivityHistory::clearHistory() {
    LittleFS.remove(HISTORY_PATH);
}