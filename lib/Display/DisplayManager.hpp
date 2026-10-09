#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Wire.h>

#include <string>

enum class DisplayStateActive : uint8_t {
    SHOWING_ACTIVITY_TRACK_TIME = 0,
    SHOWING_ACTIVITY_TRACK_SUMMARY = 1
};
inline DisplayStateActive cycleDisplayStateActive(
    DisplayStateActive currentState) {
    switch(currentState) {
        case DisplayStateActive::SHOWING_ACTIVITY_TRACK_TIME:
            return DisplayStateActive::SHOWING_ACTIVITY_TRACK_SUMMARY;
        case DisplayStateActive::SHOWING_ACTIVITY_TRACK_SUMMARY:
            return DisplayStateActive::SHOWING_ACTIVITY_TRACK_TIME;
    }
    return DisplayStateActive::SHOWING_ACTIVITY_TRACK_TIME;
}

enum class DisplayStateIdle : uint8_t {
    SHOWING_CURRENT_TIME = 0,
    SHOWING_QR_CODE = 1
};
inline DisplayStateIdle cycleDisplayStateIdle(DisplayStateIdle currentState) {
    switch(currentState) {
        case DisplayStateIdle::SHOWING_CURRENT_TIME:
            return DisplayStateIdle::SHOWING_QR_CODE;
        case DisplayStateIdle::SHOWING_QR_CODE:
            return DisplayStateIdle::SHOWING_CURRENT_TIME;
    }
    return DisplayStateIdle::SHOWING_CURRENT_TIME;
}

class DisplayManager {
   public:
    DisplayManager(uint8_t i2cAddress = 0x3C);

    bool begin();

    void showActivityTrackTime(const std::string& activityName,
                               const uint64_t activityDuration,
                               bool forceUpdate = false);

    void showActivityTrackSummary(const std::string& activityName,
                                  const uint64_t activityWeekDuration,
                                  const uint64_t activityMonthDuration,
                                  bool forceUpdate = false);

    void showCurrentTime(bool forceUpdate = false);
    void showQRCode(const std::string& qrCode, bool forceUpdate = false);

    void showMessage(const std::string& msg);
    void clear();

   private:
    static constexpr uint8_t SCREEN_WIDTH = 128;
    static constexpr uint8_t SCREEN_HEIGHT = 64;
    static constexpr int8_t OLED_RESET = -1;

    void setupNewShow();
    void displayActivityName(const std::string& activityName);
    void displayAnalogClock(uint32_t hours, uint32_t minutes);

    uint8_t i2cAddress;
    Adafruit_SSD1306 display;

    std::string activityName;

    uint64_t lastActivityDuration = 0;
    uint32_t lastCurrentTimeMinute = UINT32_MAX;

    uint64_t lastActivityWeekDuration = 0;
    uint64_t lastActivityMonthDuration = 0;
};

#endif
