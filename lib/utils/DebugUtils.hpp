#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <stdint.h>

#include <vector>

namespace DebugUtils {

void printUUID(const char* label, const std::vector<uint8_t>& uuid);

}  // namespace DebugUtils

#endif
