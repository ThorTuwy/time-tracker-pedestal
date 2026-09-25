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

const u32_t NFC_READER_DELAY = 300;
const u8_t MAX_READ_RETRY=3;

u32_t lastTickLMainLogic=0;
u8_t readRetry=0;
std::vector<uint8_t> currentUUID;

// NFC reading + Activity tracking +  OLED display
void tickMainLogic(){
    if(millis() - lastTickLMainLogic < NFC_READER_DELAY) return;
    
    std::vector<uint8_t> readUUID = nfcReader.readTagUUID();
    printUUID("Read UUID   ", readUUID);
    printUUID("Current UUID", currentUUID);
    
    if(readUUID.empty() and currentUUID.empty()){
        return;
    }
    if(readUUID.empty() and !currentUUID.empty() and readRetry<MAX_READ_RETRY){
        readRetry++;
        return;
    };
    currentUUID=readUUID;
    
    readRetry=0;

    if(currentUUID.size()!=0){
        uint32_t uuidHash = ~esp_rom_crc32_le(~0UL, currentUUID.data(), currentUUID.size());
        activityTracker.updateActivity(uuidHash);
        displayManager.showActivityTrackTime("Prueba123",activityTracker.getCurrentActivityDuration());
    }
    else{
        activityTracker.stopActivity();
        displayManager.showMessage("No tag :C\n");
    }
}

void loop() {
    tickMainLogic();
}