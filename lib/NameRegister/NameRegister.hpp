#ifndef NAME_REGISTER_H
#define NAME_REGISTER_H

#include <Arduino.h>

struct ActivityNameRecord {
    uint32_t id;
    char name[32];
    uint8_t r;
    uint8_t g;
    uint8_t b;
} __attribute__((packed));

namespace NameRegister {
    inline constexpr const char* REGISTER_PATH = "/names.bin";

    bool begin();
    bool addRecord(const ActivityNameRecord& record);
    bool searchRecord(uint32_t id, ActivityNameRecord& outRecord);
    bool replaceRecord(uint32_t id, const ActivityNameRecord& record);
    bool replaceRegister(const uint8_t* data, size_t length);
    bool deleteRecord(uint32_t id);
}

#endif
