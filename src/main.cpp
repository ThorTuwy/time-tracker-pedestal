#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_rom_crc.h>

#include <ActivityHistory.hpp>
#include <ActivityTracker.hpp>
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

DisplayStateActive currentDisplayState = DisplayStateActive::SHOWING_ACTIVITY_TRACK_TIME;
DisplayStateIdle currentIdleDisplayState = DisplayStateIdle::SHOWING_CURRENT_TIME;
DisplayStateIdle lastIdleDisplayState = DisplayStateIdle::SHOWING_QR_CODE;
std::string lastIdleDisplayMessage;
bool idleDisplayDirty = true;
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
        if(currentUUID.empty()) {
            currentIdleDisplayState =
                cycleDisplayStateIdle(currentIdleDisplayState);
            idleDisplayDirty = true;
        } else {
            currentDisplayState = cycleDisplayStateActive(currentDisplayState);
        }
    }
}

void updateIdleDisplay() {
    std::string message;
    switch(currentIdleDisplayState) {
        case DisplayStateIdle::SHOWING_CURRENT_TIME: {
            struct tm timeinfo;
            if(!getLocalTime(&timeinfo)) {
                Serial.println("Failed to obtain time");
                return;
            }

            char currentTime[64];
            strftime(currentTime, sizeof(currentTime), "%d/%m/%Y %H:%M:%S",
                     &timeinfo);
            message = currentTime;
            break;
        }
        case DisplayStateIdle::SHOWING_QR_CODE:
            message = "QR code unavailable";
            break;
    }

    if(idleDisplayDirty or currentIdleDisplayState != lastIdleDisplayState or
       message != lastIdleDisplayMessage) {
        displayManager.showMessage(message);
        lastIdleDisplayState = currentIdleDisplayState;
        lastIdleDisplayMessage = message;
        idleDisplayDirty = false;
    }
}

// NFC reading + Activity tracking +  OLED display
void tickMainLogic() {
    if(millis() - lastTickLMainLogic < NFC_READER_DELAY) return;
    lastTickLMainLogic = millis();

    std::vector<uint8_t> readUUID = nfcReader.readTagUUID();

    //Not exactly sure why, but seens that if you are constaly priting this things, and you close serial monitor
    //  spoky behiviur starts to apper, so take that into account when using it.
    //printUUID("Read UUID   ", readUUID);
    //printUUID("Current UUID", currentUUID);

    if(readUUID.empty() and currentUUID.empty()) {
        updateIdleDisplay();
        return;
    }
    if(readUUID.empty() and !currentUUID.empty() and
       readRetry < MAX_READ_RETRY) {
        readRetry++;
        return;
    };
    bool activityWasActive = !currentUUID.empty();
    currentUUID = readUUID;

    readRetry = 0;

    if(currentUUID.size() != 0) {
        idleDisplayDirty = true;
        uint32_t uuidHash =
            ~esp_rom_crc32_le(~0UL, currentUUID.data(), currentUUID.size());
        activityTracker.updateActivity(uuidHash);

        ActivityNameRecord activityNameRecord;
        if(!NameRegister::searchRecord(uuidHash,activityNameRecord)){
            ActivityNameRecord newActivityNameRecord = {
                uuidHash,
                "CHANGE_ME_IN_THE_WEB",
                0,
                0,
                0,
            };
            NameRegister::addRecord(newActivityNameRecord);
            activityNameRecord=newActivityNameRecord;
        }

        switch(currentDisplayState) {
            case DisplayStateActive::SHOWING_ACTIVITY_TRACK_TIME: {
                displayManager.showActivityTrackTime(
                    activityNameRecord.name, activityTracker.getCurrentActivityDuration());
                break;
            }
            case DisplayStateActive::SHOWING_ACTIVITY_TRACK_SUMMARY: {
                time_t currentUnixTime;
                time(&currentUnixTime);

                time_t startWeekUnixTime =
                    TimeUtils::getStartOfWeekUnixTime(currentUnixTime);
                time_t startMonthUnixTime =
                    TimeUtils::getStartOfMonthUnixTime(currentUnixTime);

                uint64_t weekActivityDuration =
                    activityTracker.getCurrentActivityDuration() +
                    ActivityHistory::getFromToActivityDuration(
                        uuidHash, startWeekUnixTime, currentUnixTime);

                uint64_t monthActivityDuration =
                    activityTracker.getCurrentActivityDuration() +
                    ActivityHistory::getFromToActivityDuration(
                        uuidHash, startMonthUnixTime, currentUnixTime);

                displayManager.showActivityTrackSummary(
                    activityNameRecord.name, weekActivityDuration, monthActivityDuration);
                break;
            }
            default:
                break;
        }
    } else {
        if(activityWasActive) {
            activityTracker.stopActivity();
            currentIdleDisplayState = DisplayStateIdle::SHOWING_CURRENT_TIME;
            idleDisplayDirty = true;
        }
        updateIdleDisplay();
    }
}

void loop() {
    handleDisplayButton();
    serialCommandHandler.handle();
    tickMainLogic();
}
