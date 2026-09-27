#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_rom_crc.h>

#include <ActivityHistory.hpp>
#include <ActivityTracker.hpp>
#include <DisplayManager.hpp>
#include <NfcReader.hpp>
#include <SerialCommandHandler.hpp>
#include <TimeUtils.hpp>

#include "Config.h"

NfcReader nfcReader(Config::CLRC663_I2C_ADDRESS);
DisplayManager displayManager(Config::OLED_I2C_ADDRESS);

ActivityTracker activityTracker;
ActivityHistory activityHistory;
SerialCommandHandler serialCommandHandler(activityHistory);

// TODO: Make something on device to configure this.
const char* ssid = "*";
const char* password = "*";

void setupWIFI() {
    Serial.printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" CONNECTED!");
}

// TODO: Make something on device to configure this.
const char* ntpServer = "pool.ntp.org";
const char* timezone = "CET-1CEST,M3.5.0,M10.5.0/3";

void setupNTP() {
    configTzTime(timezone, ntpServer);

    Serial.println("Synchronizing with NTP server in Spain...");
    struct tm timeinfo;
    while(!getLocalTime(&timeinfo)) {
        Serial.print(".");
        delay(200);
    }

    Serial.println("\nTime synchronized successfully!");
}

void setup() {
    delay(2000);
    Serial.begin(Config::BAUD_RATE);
    pinMode(Config::DISPLAY_BUTTON_PIN, INPUT_PULLUP);
    Wire.begin(Config::SDA, Config::SCL);

    nfcReader.begin(Config::SDA, Config::SCL);
    displayManager.begin();

    activityTracker.begin();

    setupWIFI();
    setupNTP();

    displayManager.showMessage("No tag :C\n");
}
void printUUID(const char* label, const std::vector<uint8_t>& uuid) {
    Serial.printf("%s: ", label);
    if(uuid.empty()) {
        Serial.println("[empty]");
        return;
    }
    for(size_t i = 0; i < uuid.size(); i++) {
        Serial.printf("%02X", uuid[i]);
        if(i < uuid.size() - 1) Serial.print(":");  // add separator
    }
    Serial.println();
}

const u32_t NFC_READER_DELAY = 300;
const u8_t MAX_READ_RETRY = 3;
const u32_t DISPLAY_BUTTON_DEBOUNCE = 50;

u32_t lastTickLMainLogic = 0;
u8_t readRetry = 0;
std::vector<uint8_t> currentUUID;

DisplayState currentDisplayState = DisplayState::SHOWING_ACTIVITY_TRACK_SUMMARY;
int lastButtonReading = HIGH;
int stableButtonState = HIGH;
u32_t lastButtonChangeTime = 0;

void handleDisplayButton() {
    int reading = digitalRead(Config::DISPLAY_BUTTON_PIN);
    if(reading != lastButtonReading) {
        lastButtonChangeTime = millis();
        lastButtonReading = reading;
    }

    if(millis() - lastButtonChangeTime < DISPLAY_BUTTON_DEBOUNCE) return;
    if(reading == stableButtonState) return;

    stableButtonState = reading;
    if(stableButtonState == LOW) {
        if(currentDisplayState == DisplayState::SHOWING_ACTIVITY_TRACK_TIME) {
            currentDisplayState = DisplayState::SHOWING_ACTIVITY_TRACK_SUMMARY;
        } else {
            currentDisplayState = DisplayState::SHOWING_ACTIVITY_TRACK_TIME;
        }
    }
}

// NFC reading + Activity tracking +  OLED display
void tickMainLogic() {
    if(millis() - lastTickLMainLogic < NFC_READER_DELAY) return;
    lastTickLMainLogic = millis();

    std::vector<uint8_t> readUUID = nfcReader.readTagUUID();

    printUUID("Read UUID   ", readUUID);
    printUUID("Current UUID", currentUUID);

    if(readUUID.empty() and currentUUID.empty()) {
        return;
    }
    if(readUUID.empty() and !currentUUID.empty() and
       readRetry < MAX_READ_RETRY) {
        readRetry++;
        return;
    };
    currentUUID = readUUID;

    readRetry = 0;

    if(currentUUID.size() != 0) {
        uint32_t uuidHash =
            ~esp_rom_crc32_le(~0UL, currentUUID.data(), currentUUID.size());
        activityTracker.updateActivity(uuidHash);
        activityTracker.updateStopTime();

        switch(currentDisplayState) {
            case DisplayState::SHOWING_ACTIVITY_TRACK_TIME: {
                displayManager.showActivityTrackTime(
                    "Prueba123", activityTracker.getCurrentActivityDuration());
                break;
            }
            case DisplayState::SHOWING_ACTIVITY_TRACK_SUMMARY: {
                time_t currentUnixTime;
                time(&currentUnixTime);

                time_t startWeekUnixTime =
                    TimeUtils::getStartOfWeekUnixTime(currentUnixTime);
                time_t startMonthUnixTime =
                    TimeUtils::getStartOfMonthUnixTime(currentUnixTime);

                uint64_t weekActivityDuration =
                    activityTracker.getCurrentActivityDuration() +
                    activityHistory.getFromToActivityDuration(
                        uuidHash, startWeekUnixTime, currentUnixTime);

                uint64_t monthActivityDuration =
                    activityTracker.getCurrentActivityDuration() +
                    activityHistory.getFromToActivityDuration(
                        uuidHash, startMonthUnixTime, currentUnixTime);

                displayManager.showActivityTrackSummary(
                    "Prueba123", weekActivityDuration, monthActivityDuration);
                break;
            }
            default:
                break;
        }
    } else {
        activityTracker.stopActivity();
        struct tm timeinfo;
        if(!getLocalTime(&timeinfo)) {
            Serial.println("Failed to obtain time");
            return;
        }

        char currentTime[64];
        strftime(currentTime, sizeof(currentTime), "%d/%m/%Y %H:%M:%S",
                 &timeinfo);
        displayManager.showMessage(currentTime);
    }
}

void loop() {
    handleDisplayButton();
    serialCommandHandler.handle();
    tickMainLogic();
}
