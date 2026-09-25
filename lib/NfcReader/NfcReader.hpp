#ifndef NFC_READER_H
#define NFC_READER_H

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "CLRC663.h"
#include <vector>
#include <iterator>

class NfcReader {
public:
    CLRC663 reader;
    NfcReader(int i2cAdress);
    bool begin(int sda, int scl);
    std::vector<uint8_t>  readTagUUID();

private:
    
};

#endif