# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Web-based DYMO label printer controller for ESP32-S3, enabling WiFi-connected label printing through a web UI and REST API. Uses native ESP32-S3 USB OTG for direct printer communication without requiring USB Host Shield hardware.

**Hardware**: ESP32-S3 Super Mini with ESP32-S3FH4R2 chip (4MB Flash, 2MB OPI PSRAM)

## Build and Development Commands

### Setup
```bash
# Copy configuration template (required for first build)
cp src/config.h.example src/config.h

# Install dependencies (PlatformIO handles this automatically)
pio run

# Verify custom board is recognized
pio boards | grep supermini
# Should show: esp32_s3_supermini ... ESP32-S3 SuperMini (ESP32-S3FH4R2)
```

### Build & Upload
```bash
# Build firmware
pio run

# Upload to ESP32-S3
pio run --target upload

# Monitor serial output (115200 baud)
pio device monitor

# Build and upload in one command
pio run --target upload && pio device monitor
```

### Development Workflow
```bash
# Clean build artifacts
pio run --target clean

# Build with verbose output for debugging
pio run -v
```

## Architecture Overview

### Core Components

**Main Application Flow** (`src/main.cpp`):
- Initializes USB printer communication via `DymoUSB` class
- Handles WiFi provisioning via BLE (if no stored credentials) or connects using NVS-stored credentials
- Sets up mDNS for `http://dymolabel.local` access
- Creates `LabelWebServer` instance for web UI and REST API

**USB Printer Communication** (`src/DymoUSB.cpp`, `src/DymoUSB.h`):
- Implements DYMO LabelManager PnP USB protocol (extracted from labelle/dymoprint projects)
- Uses ESP-IDF USB Host stack (native ESP32-S3 USB OTG on GPIO19/GPIO20)
- Thread-safe design with mutexes protecting shared state (`_printing`, `_connected` flags)
- Handles USB device enumeration, bulk transfers, and protocol commands
- Renders labels from text/QR/barcode to 1-bit monochrome bitmap format

**Web Server** (`src/WebServer.cpp`, `src/WebServer.h`):
- Async HTTP server using ESPAsyncWebServer
- Embeds HTML/CSS/JS for responsive web UI (stored in PROGMEM)
- REST API endpoints for programmatic control
- Tab-based interface: Text, QR Code, Barcode printing

### Key Architectural Patterns

**USB Communication Stack**:
```
ESP-IDF USB Host (native HW) → USB Bulk Transfers → DYMO Protocol Commands → Raster Image Data
```

**Label Rendering Pipeline**:
```
Input (text/QR/barcode) → MonoBitmap Canvas (Adafruit GFX) → 1-bit Image Buffer → DYMO Raster Format → USB Transfer
```

**Thread Safety**:
- `_stateMutex`: Protects `_printing` and `_connected` flags
- `_transferCompleteSem`: Synchronizes USB transfer completion (binary semaphore)
- `_usbHostReadySem`: Signals USB host library initialization

**WiFi Provisioning Flow** (BLE-based):
1. First boot → Attempts connection with stored credentials (from NVS)
2. If no credentials → Starts BLE advertising as "Dymolabel-ESP32"
3. User connects via "ESP BLE Provisioning" app (iOS/Android)
4. Enters Proof of Possession (PoP): `dymolabel123` (from config.h)
5. Selects WiFi network and enters password
6. Credentials stored in NVS (non-volatile storage)
7. Subsequent boots → Auto-connect using stored credentials

**WiFiProv Library**: Uses patched local version in `lib/WiFiProv/` with QR code functionality disabled (QR code dependencies not available in current framework).

### Critical Implementation Details

**DYMO USB Protocol** (`DymoUSB.cpp:377-462`):
- Vendor ID: 0x0922, Product ID: 0x1002 (after mode switch from 0x1001)
- Uses bulk OUT endpoint for commands/data, bulk IN for status
- Commands prefixed with ESC (0x1B), data lines with SYN (0x16)
- Tape specs: 6mm=48px, 9mm=64px, 12mm=64px (default), 19mm=128px
- Fixed issues: USB transfer race conditions now use semaphore-based synchronization (see BUG_REPORT.md)

**Image Rendering** (`DymoUSB.cpp:11-43`):
- Custom `MonoBitmap` class extends `Adafruit_GFX` for 1-bit graphics
- Ceiling division for buffer allocation: `(w + 7) / 8` bytes per row (prevents overflow)
- Bounds checking on `drawPixel()` operations
- Buffer stored as `std::vector<uint8_t>` for dynamic sizing

**QR Code Generation** (`DymoUSB.cpp:qrToImage()`):
- ricmoo/QRCode library with ECC_LOW error correction
- Size parameter: 2=25x25, 3=29x29 (default), 4=33x33 modules
- 3x3 pixel scale per module for readability
- Input validation: max 500 characters (DYMO_MAX_QR_DATA_LENGTH)

**Barcode Rendering** (`DymoUSB.cpp:barcodeToImage()`):
- Current implementation: Code 39 (numeric-only, basic)
- Input validation: max 100 characters (DYMO_MAX_BARCODE_LENGTH)
- Production note: Consider integrating full barcode library for Code128/EAN13

### Configuration

**`src/config.h` Settings**:
- WiFi provisioning: Device name, PoP password, reset behavior
- mDNS hostname (default: "dymolabel")
- Tape width defaults (64 pixels for 12mm tape)
- Debug serial output toggle

**PlatformIO Configuration** (`platformio.ini`):
- Platform: espressif32 (ESP32-S3)
- **Board: esp32_s3_supermini** (custom board definition in `boards/esp32_s3_supermini.json`)
- Framework: Arduino with ESP-IDF components (hybrid mode)
- Hardware specs:
  - ESP32-S3FH4R2: 4MB Flash, 2MB OPI PSRAM embedded
  - PSRAM configured as OPI (Octal SPI) mode
  - Flash in QIO (Quad I/O) mode
- Partition scheme (min_spiffs.csv for 4MB flash):
  - app0 (OTA 0): 1.875MB (larger to accommodate BLE stack)
  - app1 (OTA 1): 1.875MB
  - SPIFFS: 128KB (minimal - only for future use)
  - NVS, OTA data, coredump: ~0.12MB
- Critical build flags:
  - `USE_ESP_IDF_USB_HOST=1`: Enables native USB OTG support
  - `BOARD_HAS_PSRAM=1`: Required for image processing (2MB available - defined in board JSON)
  - `ARDUINO_LOOP_STACK_SIZE=8192`: Increased for rendering
  - `CONFIG_FREERTOS_HZ=1000`: FreeRTOS tick rate
  - `ARDUINO_USB_CDC_ON_BOOT=1`, `ARDUINO_USB_MODE=1`: Defined in board JSON
- Library ignores: `WiFiProv` (disabled due to framework QR code dependency issues)
- Monitor: 115200 baud with ESP32 exception decoder

### Custom Board Definition

The project includes a custom PlatformIO board definition at `boards/esp32_s3_supermini.json` specifically for the ESP32-S3 SuperMini with ESP32-S3FH4R2 chip. This provides:
- Proper memory configuration (QIO flash + OPI PSRAM)
- Correct USB CDC/OTG settings
- 4MB flash partition scheme
- PSRAM support enabled by default

**Benefits over generic board**:
- Accurate hardware configuration for ESP32-S3FH4R2
- Pre-configured USB settings (CDC on boot, USB mode)
- Automatic PSRAM detection and initialization
- Simplified platformio.ini (fewer manual overrides needed)

### REST API

**Endpoints**:
- `POST /api/print` - Print text label (JSON: `{text, fontSize, align}`)
- `POST /api/print/qr` - Print QR code (JSON: `{data, size}`)
- `POST /api/print/barcode` - Print barcode (JSON: `{data, type}`)
- `GET /api/status` - Get printer status
- `POST /api/feed` - Feed blank label
- `GET /update` - OTA firmware update page (web UI)
- `POST /update` - OTA firmware upload endpoint

**Implementation Note**: API endpoints use ESPAsyncWebServer's `on()` method with body handlers instead of `AsyncCallbackJsonWebHandler` (which doesn't exist in the library version being used). The body handler parses JSON from the first data chunk.

**Example**:
```bash
curl -X POST http://dymolabel.local/api/print \
  -H "Content-Type: application/json" \
  -d '{"text":"Hello World","fontSize":16,"align":"center"}'
```

### OTA Firmware Updates

**Web-based OTA Updates** are supported via the built-in `/update` endpoint:

1. Navigate to `http://dymolabel.local/update` in your web browser
2. Select the compiled `firmware.bin` file from `.pio/build/esp32_s3_supermini/firmware.bin`
3. Click "Upload Firmware"
4. Progress bar shows upload status
5. Device automatically reboots after successful update

**Implementation**: Uses ESP32's built-in `Update` library with AsyncWebServer file upload handler. The dual OTA partition scheme (app0/app1) enables safe updates - if new firmware fails, the device can rollback to the previous version.

**Building firmware for OTA**:
```bash
# Build firmware
pio run

# Firmware binary location
.pio/build/esp32_s3_supermini/firmware.bin
```

**OTA Update Process**:
1. New firmware uploaded to inactive OTA partition (app1 if running app0)
2. Boot configuration updated to boot from new partition
3. Device reboots
4. If new firmware boots successfully, update confirmed
5. If boot fails, device rolls back to previous partition automatically

### Hardware Connection

**ESP32-S3 Native USB OTG** (Recommended - Current Implementation):
```
DYMO Printer → USB-A Cable → USB-C OTG Adapter → ESP32-S3 Super Mini USB-C Port
```
- Board: ESP32-S3 Super Mini with ESP32-S3FH4R2 chip (4MB Flash, 2MB PSRAM)
- Uses built-in USB OTG capability on GPIO19/GPIO20 (no external shield needed)
- Requires ESP-IDF USB Host library (already integrated)
- Power: 5V 2A supply (printer draws 300-500mA during printing)

**Alternative: USB Host Shield** (not currently implemented):
- MAX3421E shield via SPI (pins: MOSI=11, MISO=13, SCK=12, CS=10)
- Requires USB Host Shield Library 2.0 integration
- See IMPLEMENTATION.md for migration instructions

### Known Issues and Fixes

See `BUG_REPORT.md` and `CRITICAL_FIXES.md` for detailed analysis. Key fixes applied:
- **Fix #1**: USB transfer race conditions → semaphore synchronization
- **Fix #2**: USB client handle memory leak → proper cleanup in destructor
- **Fix #3**: Static variable issues → instance members for multi-printer support
- **Fix #5**: Buffer overflow in bitmap → ceiling division for allocation
- **Fix #7**: USB device enumeration → correct 3-parameter API calls for `usb_host_device_addr_list_fill()`
- **Fix #12**: Thread-safety → mutexes for shared state access
- **Fix #13**: Magic numbers → named timing constants
- **Fix #14**: Input validation → length limits for text/QR/barcode

### Recent Build Fixes

**WiFiProv Library Patched** (QR Code Dependencies Fixed):
- The Arduino-ESP32 framework's WiFiProv library has QR code generation dependencies that aren't available
- Created local patched version in `lib/WiFiProv/` with QR code functionality stubbed out
- BLE provisioning now works with ESP BLE Provisioning app (iOS/Android)
- Partition scheme changed to `min_spiffs.csv` for larger app partitions (1.875MB each) to accommodate BLE stack
- Current firmware size: ~1.5MB (76.5% of 1.875MB partition)

**ESPAsyncWebServer JSON Handling**:
- `AsyncCallbackJsonWebHandler` class doesn't exist in the library version
- Replaced with `on()` method using body handler callbacks
- JSON parsing happens in the first chunk of the body handler

**Unused Libraries Removed**:
- TFT_eSPI and u8g2 libraries removed from platformio.ini (not used in code)
- This fixes build errors related to missing u8x8.h header

### Multi-Printer Support

Downstream USB hub support (see `docs/MULTI-PRINTER.md`):
- Connect 2-4 DYMO printers via powered USB hub
- ESP-IDF USB Host enumerates all devices
- Future enhancement: API to select target printer by device address

### Testing and Debugging

**Serial Monitor Output**:
- WiFi provisioning status with BLE instructions (if not provisioned)
- Stored credential connection attempts
- USB device enumeration details (VID/PID/endpoints)
- Transfer success/failure with ESP error codes
- Printer status responses
- OTA update progress

**Common Debug Flags** (`config.h`):
- `DEBUG_SERIAL true`: Enable verbose logging
- `CORE_DEBUG_LEVEL=3`: ESP32 core debug level (platformio.ini)

**Troubleshooting**:
- "Printer not connected": Check USB cable, ensure 5V power adequate
- "Transfer failed -1": USB enumeration issue, try USB hub reset
- WiFi connection failed: No stored credentials - use ESP BLE Provisioning app to provision
- BLE provisioning not starting: Check that device hasn't been provisioned already (reset via config.h if needed)
- "Program size too large": Partition scheme may have reverted - ensure `min_spiffs.csv` in platformio.ini
- Build errors: Run `pio lib install` to fetch dependencies

### Development Guidelines

**Code Style**:
- Use existing naming conventions: `_privateMember`, `publicMethod()`
- Thread-safe operations: Always lock `_stateMutex` when accessing `_printing`/`_connected`
- USB transfers: Use semaphore pattern (see `sendCommand()` for reference)
- Input validation: Check against `DYMO_MAX_*_LENGTH` constants before processing

**Adding New Features**:
- New print types: Follow `printText()`/`printQRCode()` pattern in `DymoUSB.cpp`
- New API endpoints: Add route in `WebServer::setupRoutes()`, implement handler
- New fonts: Add to includes, register in `textToImage()` switch statement
- Configuration options: Define in `config.h.example`, document in README.md

**Testing Changes**:
1. Build and upload firmware
2. Monitor serial output during WiFi connection
3. Access web UI at `http://dymolabel.local` (if WiFi connected)
4. Test API endpoints with curl
5. Verify printer output quality and timing

**WiFi Credential Provisioning Methods**:
1. **BLE Provisioning** (Recommended):
   - Install "ESP BLE Provisioning" app (iOS/Android)
   - Power on ESP32 (ensure no stored credentials)
   - App will find "Dymolabel-ESP32"
   - Enter PoP: `dymolabel123`
   - Select WiFi network and enter password

2. **Hardcode temporarily** for testing:
   - Add to main.cpp: `WiFi.begin("SSID", "password");`

3. **Reset stored credentials**:
   - Enable `WIFI_PROV_RESET_ON_BOOT` in config.h
   - Upload firmware
   - Disable and re-upload to prevent continuous reset
