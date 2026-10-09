#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_rom_crc.h>

#include <ActivityHistory.hpp>
#include <ActivityTracker.hpp>
#include <DebugUtils.hpp>
#include <DisplayManager.hpp>
#include <NameRegister.hpp>
#include <NfcReader.hpp>
#include <SerialCommandHandler.hpp>
#include <TimeUtils.hpp>
#include <WebServer.hpp>

#include "Config.h"

NfcReader nfcReader(Config::CLRC663_I2C_ADDRESS);
DisplayManager displayManager(Config::OLED_I2C_ADDRESS);

ActivityTracker activityTracker;
SerialCommandHandler serialCommandHandler;

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

    ActivityHistory::begin();
    NameRegister::begin();
    activityTracker.begin();

    setupWIFI();
    setupNTP();

    WebServer::begin();

    displayManager.showMessage("No tag :C\n");
}
DisplayStateActive currentDisplayState =
    DisplayStateActive::SHOWING_ACTIVITY_TRACK_TIME;
DisplayStateIdle currentIdleDisplayState =
    DisplayStateIdle::SHOWING_CURRENT_TIME;

uint32_t activityId;
bool forceDisplayUpdate = false;

int lastButtonReading = HIGH;
int stableButtonState = HIGH;
u32_t lastButtonChangeTime = 0;
const u32_t DISPLAY_BUTTON_DEBOUNCE = 50;
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
        if(activityId != 0) {
            currentDisplayState = cycleDisplayStateActive(currentDisplayState);
        } else {
            currentIdleDisplayState =
                cycleDisplayStateIdle(currentIdleDisplayState);
        }
    }

    forceDisplayUpdate = true;
}

const u32_t NFC_READER_DELAY = 300;
const u8_t MAX_READ_RETRY = 3;

uint64_t weekActivityDuration;
uint64_t monthActivityDuration;

u32_t lastTickLMainLogic = 0;
u8_t readRetry = 0;
std::vector<uint8_t> activityUUID;
std::string activityName = "";
void tickCardHandler() {
    if(millis() - lastTickLMainLogic < NFC_READER_DELAY) return;
    lastTickLMainLogic = millis();

    std::vector<uint8_t> readUUID = nfcReader.readTagUUID();

    if(readUUID.empty() and activityUUID.empty()) {
        return;
    }
    if(readUUID.empty() and !activityUUID.empty() and
       readRetry < MAX_READ_RETRY) {
        readRetry++;
        return;
    };
    readRetry = 0;

    u32_t newActivityId =
        ~esp_rom_crc32_le(~0UL, readUUID.data(), readUUID.size());
    bool cardChange = newActivityId != activityId;

    activityId = newActivityId;
    activityUUID = readUUID;

    if(activityId != 0) {
        activityTracker.updateActivity(activityId);
        if(cardChange) {
            forceDisplayUpdate = true;
            currentDisplayState =
                DisplayStateActive::SHOWING_ACTIVITY_TRACK_TIME;
            ActivityNameRecord activityNameRecord;
            if(!NameRegister::searchRecord(activityId, activityNameRecord)) {
                ActivityNameRecord newActivityNameRecord = {
                    activityId, "CHANGE_ME_IN_THE_WEB", 0, 0, 0,
                };
                NameRegister::addRecord(newActivityNameRecord);
                activityNameRecord = newActivityNameRecord;
            }
            activityName = activityNameRecord.name;

            time_t currentUnixTime;
            time(&currentUnixTime);
            time_t startWeekUnixTime =
                TimeUtils::getStartOfWeekUnixTime(currentUnixTime);
            uint64_t weekActivityDuration =
                ActivityHistory::getFromToActivityDuration(
                    activityId, startWeekUnixTime, currentUnixTime);
            time_t startMonthUnixTime =
                TimeUtils::getStartOfMonthUnixTime(currentUnixTime);
            uint64_t monthActivityDuration =
                ActivityHistory::getFromToActivityDuration(
                    activityId, startMonthUnixTime, currentUnixTime);
        }
    } else {
        if(cardChange) {
            forceDisplayUpdate = true;
            activityTracker.stopActivity();
            currentIdleDisplayState = DisplayStateIdle::SHOWING_CURRENT_TIME;
            activityName = "";
        }
    }
}

void tickDisplayStateActive() {
    switch(currentDisplayState) {
        case DisplayStateActive::SHOWING_ACTIVITY_TRACK_TIME: {
            displayManager.showActivityTrackTime(
                activityName, activityTracker.getCurrentActivityDuration(),
                forceDisplayUpdate);
            break;
        }
        case DisplayStateActive::SHOWING_ACTIVITY_TRACK_SUMMARY: {
            u64_t currentTrackerTime =
                activityTracker.getCurrentActivityDuration();

            displayManager.showActivityTrackSummary(
                activityName, currentTrackerTime + weekActivityDuration,
                currentTrackerTime + monthActivityDuration, forceDisplayUpdate);
            break;
        }
        default:
            break;
    }
    forceDisplayUpdate = false;
}

void tickDisplayStateIdle() {
    switch(currentIdleDisplayState) {
        case DisplayStateIdle::SHOWING_CURRENT_TIME: {
            displayManager.showCurrentTime("CurrentTime", forceDisplayUpdate);
            break;
        }
        case DisplayStateIdle::SHOWING_QR_CODE:
            displayManager.showQRCode("QR", forceDisplayUpdate);
            break;
    }
    forceDisplayUpdate = false;
}

void loop() {
    handleDisplayButton();
    serialCommandHandler.handle();
    tickCardHandler();
    if(activityId != 0) {
        tickDisplayStateActive();
    } else {
        tickDisplayStateIdle();
    }
}
