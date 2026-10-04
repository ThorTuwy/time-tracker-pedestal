#include "SerialCommandHandler.hpp"

#include <string.h>

SerialCommandHandler::SerialCommandHandler() : buffer{}, length(0) {}

void SerialCommandHandler::handle() {
    while(Serial.available()) {
        char c = (char)Serial.read();
        if(c == '\n' or c == '\r') {
            if(length == 0) continue;
            buffer[length] = '\0';
            runCommand(buffer);
            length = 0;
            continue;
        }
        if(length < sizeof(buffer) - 1) buffer[length++] = c;
    }
}

void SerialCommandHandler::runCommand(const char* command) {
    if(strcmp(command, "dump") == 0) {
        printHistory();
        return;
    }
    if(strcmp(command, "help") == 0) {
        printHelp();
        return;
    }
    Serial.printf("Unknown command: %s\n", command);
    printHelp();
}

void SerialCommandHandler::printHelp() {
    Serial.println("Commands:");
    Serial.println("  dump   print all /history.bin records");
    Serial.println("  help   show this help");
}

void SerialCommandHandler::printHistory() {
    size_t count = ActivityHistory::getRecordCount();
    Serial.printf("Records: %u (%u bytes each)\n", (unsigned)count,
                  (unsigned)sizeof(ActivityRecord));
    for(size_t i = 0; i < count; i++) {
        ActivityRecord record;
        if(!ActivityHistory::readRecord(i, record)) continue;
        printRecord(i, record);
    }
    Serial.println("Done.");
}

void SerialCommandHandler::printRecord(size_t index,
                                       const ActivityRecord& record) {
    struct tm startInfo;
    struct tm stopInfo;
    localtime_r((const time_t*)&record.startUnixTime, &startInfo);
    localtime_r((const time_t*)&record.stopUnixTime, &stopInfo);

    char startText[24];
    char stopText[24];
    strftime(startText, sizeof(startText), "%d/%m/%Y %H:%M:%S", &startInfo);
    strftime(stopText, sizeof(stopText), "%d/%m/%Y %H:%M:%S", &stopInfo);

    uint32_t seconds = (uint32_t)(record.stopUnixTime - record.startUnixTime);
    Serial.printf("#%u id=0x%08lX  %s -> %s  %02lu:%02lu:%02lu\n",
                  (unsigned)index, (unsigned long)record.ActivityId, startText,
                  stopText, (unsigned long)(seconds / 3600),
                  (unsigned long)((seconds % 3600) / 60),
                  (unsigned long)(seconds % 60));
}
