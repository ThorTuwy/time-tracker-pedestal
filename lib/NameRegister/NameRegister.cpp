#include "NameRegister.hpp"

#include <LittleFS.h>

namespace NameRegister {

bool begin() {
    if(LittleFS.exists(REGISTER_PATH)) return true;

    File file = LittleFS.open(REGISTER_PATH, FILE_WRITE);
    if(!file) return false;
    file.close();
    return true;
}

bool addRecord(const ActivityNameRecord& record) {
    if(record.id == 0) return false;

    File file = LittleFS.open(REGISTER_PATH, FILE_APPEND);
    if(!file) return false;

    size_t bytesWritten = file.write((const uint8_t*)&record, sizeof(ActivityNameRecord));
    file.close();
    return bytesWritten == sizeof(ActivityNameRecord);
}

bool searchRecord(uint32_t id, ActivityNameRecord& outRecord) {
    if(id == 0) return false;

    File file = LittleFS.open(REGISTER_PATH, FILE_READ);
    if(!file) return false;

    ActivityNameRecord record;
    while(file.read((uint8_t*)&record, sizeof(ActivityNameRecord)) == sizeof(ActivityNameRecord)) {
        if(record.id == id) {
            outRecord = record;
            file.close();
            return true;
        }
    }

    file.close();
    return false;
}

bool replaceRecord(uint32_t id, const ActivityNameRecord& record) {
    if(id == 0 or record.id != id) return false;

    File file = LittleFS.open(REGISTER_PATH, "r+");
    if(!file) return false;

    ActivityNameRecord currentRecord;
    size_t offset = 0;
    while(file.read((uint8_t*)&currentRecord, sizeof(ActivityNameRecord)) ==
          sizeof(ActivityNameRecord)) {
        if(currentRecord.id == id) {
            if(!file.seek(offset)) {
                file.close();
                return false;
            }

            size_t bytesWritten =
                file.write((const uint8_t*)&record, sizeof(ActivityNameRecord));
            file.close();
            return bytesWritten == sizeof(ActivityNameRecord);
        }
        offset += sizeof(ActivityNameRecord);
    }

    file.close();
    return false;
}

bool deleteRecord(uint32_t id) {
    if(id == 0) return false;

    File file = LittleFS.open(REGISTER_PATH, "r+");
    if(!file) return false;

    ActivityNameRecord record;
    size_t offset = 0;
    while(file.read((uint8_t*)&record, sizeof(ActivityNameRecord)) == sizeof(ActivityNameRecord)) {
        if(record.id == id) {
            uint32_t emptyId = 0;
            if(!file.seek(offset)) {
                file.close();
                return false;
            }

            size_t bytesWritten = file.write((const uint8_t*)&emptyId, sizeof(emptyId));
            file.close();
            return bytesWritten == sizeof(emptyId);
        }
        offset += sizeof(ActivityNameRecord);
    }

    file.close();
    return false;
}

}  // namespace NameRegister
