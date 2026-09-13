#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <NfcReader.hpp>
#include <DisplayManager.hpp>
#include <ActivityTracker.hpp>

#include <esp_rom_crc.h>

#include "Config.h"

NfcReader nfcReader(Config::CLRC663_I2C_ADDRESS);
DisplayManager displayManager(Config::OLED_I2C_ADDRESS);
ActivityTracker activityTracker;

void setup() {
    delay(2000);
    Serial.begin(Config::BAUD_RATE);
    Wire.begin(Config::SDA,Config::SCL);

    nfcReader.begin(Config::SDA,Config::SCL);
    displayManager.begin();
    activityTracker.begin();

    displayManager.showMessage("No tag :C\n");
}

const u32_t LOOP_DELAY = 300;

const u8_t MAX_READ_RETRY=3;
u8_t readRetry=0;
std::vector<uint8_t> currentUUID;

void printUUID(const char* label, const std::vector<uint8_t>& uuid) {
    Serial.printf("%s: ", label);
    if (uuid.empty()) {
        Serial.println("[empty]");
        return;
    }
    for (size_t i = 0; i < uuid.size(); i++) {
        Serial.printf("%02X", uuid[i]);
        if (i < uuid.size() - 1) Serial.print(":"); // add separator
    }
    Serial.println();
}

void loop() {
    std::vector<uint8_t> readUUID = nfcReader.readTagUUID();
    printUUID("Read UUID   ", readUUID);
    printUUID("Current UUID", currentUUID);

    
    if(readUUID.empty() and currentUUID.empty()){
        delay(LOOP_DELAY);
        return;
    }
    if(readUUID.empty() and !currentUUID.empty() and readRetry<MAX_READ_RETRY){
        readRetry++;
        delay(LOOP_DELAY);
        return;
    };
    currentUUID=readUUID;
    
    readRetry=0;

    if(currentUUID.size()!=0){
        uint32_t uuidHash = ~esp_rom_crc32_le(~0UL, currentUUID.data(), currentUUID.size());
        activityTracker.updateActivity(uuidHash);
        displayManager.showActivityCurrentTime("Prueba123",activityTracker.getCurrentActivityDuration());
    }
    else{
        activityTracker.stopActivity();
        displayManager.showMessage("No tag :C\n");
    }
    
    delay(LOOP_DELAY);
}