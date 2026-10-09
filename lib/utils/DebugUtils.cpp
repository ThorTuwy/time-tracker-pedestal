#include "DebugUtils.hpp"

#include <Arduino.h>

void DebugUtils::printUUID(const char* label,
                           const std::vector<uint8_t>& uuid) {
    Serial.printf("%s: ", label);
    if(uuid.empty()) {
        Serial.println("[empty]");
        return;
    }
    for(size_t i = 0; i < uuid.size(); i++) {
        Serial.printf("%02X", uuid[i]);
        if(i < uuid.size() - 1) Serial.print(":");
    }
    Serial.println();
}
