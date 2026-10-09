#include "DisplayManager.hpp"

DisplayManager::DisplayManager(uint8_t address)
    : i2cAddress(address),
      display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1),
      lastActivityDuration(0) {}

bool DisplayManager::begin() {
    if(!display.begin(SSD1306_SWITCHCAPVCC, i2cAddress)) {
        return false;
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.display();
    return true;
}

void DisplayManager::setupNewShow() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setFont();

    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
}

void DisplayManager::showActivityTrackTime(const std::string& activityName,
                                           const uint64_t activityDuration,
                                           bool forceUpdate) {
    uint32_t hours = (activityDuration / 3600);
    uint32_t minutes = (activityDuration % 3600) / 60;

    uint32_t prevHours = (lastActivityDuration / 3600);
    uint32_t prevMinutes = (lastActivityDuration % 3600) / 60;

    if(!forceUpdate and (hours == prevHours and minutes == prevMinutes) and
       lastActivityDuration != 0)
        return;
    lastActivityDuration = activityDuration;

    setupNewShow();

    displayActivityName(activityName);
    displayAnalogClock(hours, minutes);

    display.setTextSize(1);
    display.setFont(&FreeMono9pt7b);

    char timeStr[16];

    display.setCursor(73, 34);
    snprintf(timeStr, sizeof(timeStr), "%02uh", hours);
    display.print(timeStr);

    display.setCursor(73, 52);
    snprintf(timeStr, sizeof(timeStr), "%02um", minutes);
    display.print(timeStr);

    display.display();
}

void DisplayManager::displayAnalogClock(uint32_t hours, uint32_t minutes) {
    const int16_t clockCenterX = 34;
    const int16_t clockCenterY = 38;
    const int16_t clockRadius = 18;

    display.drawCircle(clockCenterX, clockCenterY, clockRadius, SSD1306_WHITE);
    display.fillCircle(clockCenterX, clockCenterY, 2, SSD1306_WHITE);

    // Clock hour markers
    display.drawPixel(clockCenterX, clockCenterY - clockRadius + 2,
                      SSD1306_WHITE);
    display.drawPixel(clockCenterX + clockRadius - 2, clockCenterY,
                      SSD1306_WHITE);
    display.drawPixel(clockCenterX, clockCenterY + clockRadius - 2,
                      SSD1306_WHITE);
    display.drawPixel(clockCenterX - clockRadius + 2, clockCenterY,
                      SSD1306_WHITE);

    float minuteAngle = (minutes * 6.0f - 90.0f) * (M_PI / 180.0f);
    float hourAngle =
        (((hours % 12) + (minutes / 60.0f)) * 30.0f - 90.0f) * (M_PI / 180.0f);

    const int16_t minuteHandLen = clockRadius - 4;
    int16_t minuteX = clockCenterX + cos(minuteAngle) * minuteHandLen;
    int16_t minuteY = clockCenterY + sin(minuteAngle) * minuteHandLen;
    display.drawLine(clockCenterX, clockCenterY, minuteX, minuteY,
                     SSD1306_WHITE);

    const int16_t hourHandLen = clockRadius - 8;
    int16_t hourX = clockCenterX + cos(hourAngle) * hourHandLen;
    int16_t hourY = clockCenterY + sin(hourAngle) * hourHandLen;
    display.drawLine(clockCenterX, clockCenterY, hourX, hourY, SSD1306_WHITE);
}

void DisplayManager::showActivityTrackSummary(
    const std::string& activityName, const uint64_t activityWeekDuration,
    const uint64_t activityMonthDuration, bool forceUpdate) {
    uint32_t weekHours = (activityWeekDuration / 3600);
    uint32_t monthHours = (activityMonthDuration / 3600);

    uint32_t prevWeekHours = (lastActivityWeekDuration / 3600);
    uint32_t prevMonthHours = (lastActivityMonthDuration / 3600);

    if(!forceUpdate and
       (weekHours == prevWeekHours and monthHours == prevMonthHours) and
       lastActivityWeekDuration != 0 and lastActivityMonthDuration != 0)
        return;

    lastActivityWeekDuration = activityWeekDuration;
    lastActivityMonthDuration = activityMonthDuration;

    setupNewShow();

    displayActivityName(activityName);

    display.setFont(&FreeMono9pt7b);
    display.setTextSize(1);
    char timeStr[16];

    display.setCursor(10, 33);
    snprintf(timeStr, sizeof(timeStr), "Week:  %02uh", weekHours);
    display.print(timeStr);

    display.setCursor(10, 51);
    snprintf(timeStr, sizeof(timeStr), "Month: %02uh", monthHours);
    display.print(timeStr);

    display.display();
}

void DisplayManager::showCurrentTime(const std::string& currentTime,
                                     bool forceUpdate) {
    showIdleText(DisplayStateIdle::SHOWING_CURRENT_TIME, currentTime,
                 forceUpdate);
}

void DisplayManager::showQRCode(const std::string& qrCode, bool forceUpdate) {
    showIdleText(DisplayStateIdle::SHOWING_QR_CODE, qrCode, forceUpdate);
}

void DisplayManager::showIdleText(DisplayStateIdle state,
                                  const std::string& text, bool forceUpdate) {
    if(!forceUpdate) return;

    setupNewShow();
    display.setTextSize(1);

    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &textWidth,
                          &textHeight);
    int16_t textX = (display.width() - textWidth) / 2;
    if(textX < 3) textX = 3;
    display.setCursor(textX, (display.height() + textHeight) / 2);
    display.print(text.c_str());
    display.display();
}

void DisplayManager::displayActivityName(const std::string& activityName) {
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    display.getTextBounds(activityName.c_str(), 0, 0, &x1, &y1, &textWidth,
                          &textHeight);

    int16_t titleX = (display.width() - textWidth) / 2;
    if(titleX < 0) titleX = 0;
    display.setCursor(titleX, 3);

    display.setFont();
    display.setTextSize(1);
    display.print(activityName.c_str());

    display.drawFastHLine(0, 13, display.width(), SSD1306_WHITE);
}

void DisplayManager::showMessage(const std::string& msg) {
    lastActivityDuration = 0;
    lastActivityWeekDuration = 0;
    lastActivityMonthDuration = 0;

    display.clearDisplay();

    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(15, 20);
    display.print(msg.c_str());

    display.display();
}

void DisplayManager::clear() {
    display.clearDisplay();
    display.display();
}
