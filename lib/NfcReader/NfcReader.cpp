#include "NfcReader.hpp"

NfcReader::NfcReader(int i2cAdress) : reader(i2cAdress, -1) {}

bool NfcReader::begin(int sda, int scl) {
    reader.begin(sda, scl);
    return true;
}

/// @brief Make a poll of the current card place in the NFC sensor.
/// @return Vector with the UUID, or empty vector if no UUID was found.
std::vector<uint8_t> NfcReader::readTagUUID() {
    uint8_t uid[10] = {0};

    // Check for ISO ISO-14443
    reader.softReset();
    reader.AN1102_recommended_registers(
        MFRC630_PROTO_ISO14443A_106_MILLER_MANCHESTER);

    uint8_t uid_len = reader.read_iso14443_uid(uid);

    if(uid_len != 0) {
        std::vector<uint8_t> uid_vec(std::begin(uid),
                                     std::begin(uid) + uid_len);
        return uid_vec;
    }

    // Check for ISO ISO-15693
    reader.softReset();
    reader.AN1102_recommended_registers(MFRC630_PROTO_ISO15693_1_OF_4_SSC);

    uint8_t password[] = {0x0F, 0x0F, 0x0F, 0x0F};
    uid_len = reader.read_iso18693_uid(uid, password);

    if(uid_len != 0) {
        std::vector<uint8_t> uid_vec(std::begin(uid),
                                     std::begin(uid) + uid_len);
        return uid_vec;
    }

    return {};
}