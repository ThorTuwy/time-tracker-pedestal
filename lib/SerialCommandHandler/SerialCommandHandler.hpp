#ifndef SERIAL_COMMAND_HANDLER_H
#define SERIAL_COMMAND_HANDLER_H

#include <Arduino.h>

#include <ActivityHistory.hpp>

class SerialCommandHandler {
   public:
    SerialCommandHandler(ActivityHistory& activityHistory);
    void handle();

   private:
    static constexpr size_t COMMAND_BUFFER_SIZE = 16;

    ActivityHistory& activityHistory;
    char buffer[COMMAND_BUFFER_SIZE];
    size_t length;

    void runCommand(const char* command);
    void printHelp();
    void printHistory();
    void printRecord(size_t index, const ActivityRecord& record);
};

#endif
