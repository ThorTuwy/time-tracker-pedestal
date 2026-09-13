#include <Arduino.h>

namespace Config {
    constexpr unsigned long BAUD_RATE = 115200;
    constexpr unsigned long SDA = 5;
    constexpr unsigned long SCL = 4;

    constexpr unsigned long CLRC663_I2C_ADDRESS = 0x28;

    constexpr unsigned long OLED_I2C_ADDRESS = 0x3C;
}