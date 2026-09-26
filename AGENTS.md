# AGENTS.md

ESP32-C3 NFC time tracker ("pedestal"). Arduino framework via PlatformIO, single env, no tests, no CI, no linter/formatter config. **The build is the only automated check** — `/home/thortuwy/.platformio/penv/bin/platformio run` must pass before you call anything done.

## Commands

`pio` is **not on PATH**; the core lives in the PlatformIO venv:

```bash
~/.platformio/penv/bin/pio run          # build (last verified: SUCCESS, RAM 12%, Flash 63%)
~/.platformio/penv/bin/pio run -t upload
~/.platformio/penv/bin/pio device monitor   # 115200 baud
```

- Only env is `env:esp32-c3-devkitm-1`; pass `-e esp32-c3-devkitm-1` if you add a second one.
- `test/` contains only the PlatformIO template README — there are no test suites. Don't invent a test command; `pio test` has nothing to run.
- `.vscode/extensions.json` is the only tracked editor config, and it deliberately *unrecommends* `ms-vscode.cpptools-extension-pack` and `pioarduino.pioarduino-ide`. The rest of `.vscode/` is gitignored and local.

## Hardware map (`include/Config.h`)

SDA=5, SCL=4, CLRC663 NFC reader @ `0x28`, SSD1306 128x64 OLED @ `0x3C`. All I2C, one shared bus. Change hardware here, not inline.

## Gotchas an agent will get wrong

- **`Serial` is native USB CDC, not UART.** `build_flags` set `ARDUINO_USB_MODE=1` and `ARDUINO_USB_CDC_ON_BOOT=1`. The `delay(2000)` at the top of `setup()` is there to let the USB host enumerate before the first print — do not "remove the pointless delay".
- **`NfcReader::begin()` re-initializes the global `Wire`.** The vendored CLRC663 lib's `begin(sda, scl)` calls `Wire.setPins(...)` + `Wire.begin()`, and that is the *same* bus the OLED hangs off. Keep `Wire.begin()` and `nfcReader.begin()` on the same `Config::` pins, and keep `Wire.begin()` first.
- **No partition labeled `littlefs`.** The default table is used; the partition labeled `spiffs` (offset `0x290000`, subtype littlefs) is what `LittleFS` actually mounts. If you add `build.partitions`, the label must still match.
- **`LittleFS.begin(true)` formats on mount failure**, silently destroying the activity history in `/history.bin`. Treat any change to the partition table or `ActivityTracker::begin()` as data-loss-relevant.
- **`ActivityRecord` is memcpy'd raw** to `/history.bin` as `sizeof(ActivityRecord)` bytes. Reordering or resizing the struct silently invalidates every record already on device.
- **`ActivityTracker` uses `time()`, not `getLocalTime()`** — timestamps are epoch seconds, unaffected by the configured TZ. `configTzTime()` in `setup()` only affects the display.
- **Time is only valid after boot sync.** `setup()` blocks until NTP succeeds, so `time()`-based logic must never run before that; don't move module `begin()` calls above the NTP block without thinking.
- **WiFi credentials and NTP server/timezone are hardcoded in `src/main.cpp`**, each marked `//TODO: Make something on device to configure this.` Don't add new secrets, and don't refactor them into a settings module unless asked.
- `readTagUUID()` does a full `softReset()` + `AN1102_recommended_registers()` for ISO14443A, then retries as ISO15693 with a hardcoded `0x0F0F0F0F` password. Both protocols are probed on every poll (~300 ms), so the reset cost is real.

## Conventions

- **Non-blocking by design.** `loop()` just calls `tickMainLogic()`, which self-gates on `millis()` with `NFC_READER_DELAY` (300 ms). Commit 14d984a deliberately moved from `delay()` to early returns — never add blocking delays to the loop path; return early instead.
- **Use `and` / `or`, not `&&` / `||`.** The codebase has zero `&&`/`||`; match the surrounding token style rather than "fixing" it.
- 4-space indent, Allman braces, tight opening paren: `if(x) {`.
- Project headers are included with angle brackets (`#include <NfcReader.hpp>`) because PlatformIO puts `lib/*` on the include path; `Config.h` uses quotes.
- New headers need include guards, named `<UPPER_SNAKE>_H` (see `NFC_READER_H`, `DISPLAY_MANAGER_H`, `ACTIVITY_TRACKER_H`).
- Use `u32_t`/`u8_t` style typedefs alongside the plain `uint32_t`/`uint8_t` already present; both appear in-tree.

## Architecture

- `src/main.cpp` owns the state machine and constructs all three module instances at global scope. Modules live in `lib/<Name>/<Name>.{hpp,cpp}` and are only ever reached through their headers.
- **`NfcReader`** — thin wrapper over the vendored CLRC663 lib; returns the tag UID as `std::vector<uint8_t>`, empty when no tag.
- **`ActivityTracker`** — current state in NVS namespace `ev_tracker`, append-only log in LittleFS `/history.bin`. The activity id is a **CRC32 of the tag UID** computed in `main.cpp` (`~esp_rom_crc32_le(...)`), not a stored mapping; there is no UID→name table yet, so `main.cpp` passes a hardcoded `"Prueba123"` label to the display.
- **`DisplayManager`** — `showActivityTrackTime()` self-throttles: it only redraws when the displayed hour/minute changes, and `showMessage()` resets `lastActivityDuration` to force one. Don't add redraw throttling on top.
- Tag-removal detection in `tickMainLogic()` uses a `MAX_READ_RETRY` (3) counter, not a single miss, to avoid stopping activity on a transient read failure.

## Known gaps

- `DisplayState` (in `DisplayManager.hpp`) is declared but unused; the summary screen is not implemented.
- The `DisplayState`/`showActivityTrackSummary` path and the tag-UID→activity-name mapping are unimplemented features, not bugs to route around.
