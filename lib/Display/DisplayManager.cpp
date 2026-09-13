#include "DisplayManager.hpp"


DisplayManager::DisplayManager(uint8_t address)
    :
    i2cAddress(address),
    display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1),
    lastActivityDuration(0)
{}

bool DisplayManager::begin() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, i2cAddress)) {
        return false; // Let the caller decide how to handle the error
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.display();
    return true;
}


void DisplayManager::showActivityCurrentTime(const std::string& activityName,const uint64_t activityDuration){
    uint32_t hours   = (activityDuration / 3600);
    uint32_t minutes = (activityDuration % 3600) / 60;

    uint32_t prevHours   = (lastActivityDuration / 3600);
    uint32_t prevMinutes = (lastActivityDuration % 3600) / 60;

    if( (hours==prevHours and minutes==prevMinutes) and lastActivityDuration!=0) return;
    lastActivityDuration=activityDuration;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setFont();

    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

    displayActivityName(activityName);
    displayAnalogClock(hours,minutes);

    display.setTextSize(1);
    display.setFont(&FreeMono9pt7b);
    char timeStr[16];
    
    display.setCursor(68, 34);
    snprintf(timeStr, sizeof(timeStr), "%02uh", hours);
    display.print(timeStr);

    display.setCursor(68, 52);
    snprintf(timeStr, sizeof(timeStr), "%02um", minutes);
    display.print(timeStr);

    

    display.display();
}

void DisplayManager::displayAnalogClock(uint32_t hours, uint32_t minutes){
    const int16_t clockCenterX = 24;
    const int16_t clockCenterY = 38;
    const int16_t clockRadius  = 18;

    display.drawCircle(clockCenterX, clockCenterY, clockRadius, SSD1306_WHITE);
    display.fillCircle(clockCenterX, clockCenterY, 2, SSD1306_WHITE);

    // Clock hour markers
    display.drawPixel(clockCenterX, clockCenterY - clockRadius + 2, SSD1306_WHITE);
    display.drawPixel(clockCenterX + clockRadius - 2, clockCenterY, SSD1306_WHITE);
    display.drawPixel(clockCenterX, clockCenterY + clockRadius - 2, SSD1306_WHITE);
    display.drawPixel(clockCenterX - clockRadius + 2, clockCenterY, SSD1306_WHITE);
    
    float minuteAngle = (minutes * 6.0f - 90.0f) * (M_PI / 180.0f);
    float hourAngle   = (((hours % 12) + (minutes / 60.0f)) * 30.0f - 90.0f) * (M_PI / 180.0f);

    const int16_t minuteHandLen = clockRadius - 4;
    int16_t minuteX = clockCenterX + cos(minuteAngle) * minuteHandLen;
    int16_t minuteY = clockCenterY + sin(minuteAngle) * minuteHandLen;
    display.drawLine(clockCenterX, clockCenterY, minuteX, minuteY, SSD1306_WHITE);

    const int16_t hourHandLen   = clockRadius - 8;
    int16_t hourX = clockCenterX + cos(hourAngle) * hourHandLen;
    int16_t hourY = clockCenterY + sin(hourAngle) * hourHandLen;
    display.drawLine(clockCenterX, clockCenterY, hourX, hourY, SSD1306_WHITE);
}   

void DisplayManager::displayActivityName(const std::string& activityName){
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    display.getTextBounds(activityName.c_str(), 0, 0, &x1, &y1, &textWidth, &textHeight);

    int16_t titleX = (display.width() - textWidth) / 2;
    if (titleX < 0) titleX = 0;
    display.setCursor(titleX, 2);
    display.print(activityName.c_str());

    display.drawFastHLine(0, 13, display.width(), SSD1306_WHITE);
}

void DisplayManager::showMessage(const std::string& msg) {
    lastActivityDuration=0;
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