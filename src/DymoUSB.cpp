#include "DymoUSB.h"
#include "config.h"

DymoUSB::DymoUSB() : _connected(false), _printing(false),
                     _tapeWidth(DYMO_DEFAULT_TAPE_WIDTH), _dotTab(0) {
    _bytesPerLine = calculateBytesPerLine();
}

bool DymoUSB::begin() {
    #if DEBUG_SERIAL
    Serial.println("[DYMO] Initializing DYMO LabelManager PnP...");
    Serial.printf("[DYMO] Vendor ID: 0x%04X, Product ID: 0x%04X\n", DYMO_VENDOR_ID, DYMO_PRODUCT_ID);
    #endif

    // TODO: Initialize USB Host Shield
    // For now, we'll simulate connection for testing

    // Initialize tape parameters
    _bytesPerLine = calculateBytesPerLine();

    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Tape width: %dmm, Bytes per line: %d\n", _tapeWidth, _bytesPerLine);
    #endif

    // Send initialization sequence
    uint8_t initCmd[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    if (!sendCommand(initCmd, sizeof(initCmd))) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Failed to send init command");
        #endif
        return false;
    }

    // Set tape color to 0 (default)
    if (!cmdTapeColor(0)) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Failed to set tape color");
        #endif
        return false;
    }

    // Set bytes per line
    if (!cmdBytesPerLine(_bytesPerLine)) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Failed to set bytes per line");
        #endif
        return false;
    }

    // Request status to verify connection
    if (cmdStatus()) {
        _connected = true;
        #if DEBUG_SERIAL
        Serial.println("[DYMO] ✓ Printer initialized successfully");
        #endif
        return true;
    }

    #if DEBUG_SERIAL
    Serial.println("[DYMO] ✗ Printer initialization failed");
    Serial.println("[DYMO] Continuing in demo mode (USB Host Shield not connected)");
    #endif

    // Still return true to allow demo mode
    _connected = true;
    return true;
}

bool DymoUSB::isReady() {
    return _connected && !_printing;
}

// ============================================================================
// DYMO Protocol Command Methods (based on labelle/dymoprint)
// ============================================================================

bool DymoUSB::cmdStatus() {
    uint8_t cmd[] = {DYMO_ESC, 'A'};
    uint8_t response[8];

    if (sendCommandWithResponse(cmd, sizeof(cmd), response, sizeof(response))) {
        #if DEBUG_SERIAL
        Serial.print("[DYMO] Status: ");
        for (int i = 0; i < 8; i++) {
            Serial.printf("0x%02X ", response[i]);
        }
        Serial.println();
        #endif
        return true;
    }

    return false;
}

bool DymoUSB::cmdDotTab(uint8_t value) {
    if (value > 8) value = 8;  // Max dot tab is 8
    uint8_t cmd[] = {DYMO_ESC, 'B', value};
    return sendCommand(cmd, sizeof(cmd));
}

bool DymoUSB::cmdTapeColor(uint8_t value) {
    uint8_t cmd[] = {DYMO_ESC, 'C', value};
    return sendCommand(cmd, sizeof(cmd));
}

bool DymoUSB::cmdBytesPerLine(uint8_t value) {
    uint8_t cmd[] = {DYMO_ESC, 'D', value};
    return sendCommand(cmd, sizeof(cmd));
}

bool DymoUSB::cmdCut() {
    uint8_t cmd[] = {DYMO_ESC, 'E'};
    return sendCommand(cmd, sizeof(cmd));
}

bool DymoUSB::cmdSendLine(const uint8_t* lineData, size_t length) {
    // Line format: SYN + pixel data
    std::vector<uint8_t> cmd;
    cmd.push_back(DYMO_SYN);
    cmd.insert(cmd.end(), lineData, lineData + length);

    return sendCommand(cmd.data(), cmd.size());
}

bool DymoUSB::cmdSkipLines(uint16_t count) {
    // Skip lines by sending multiple SYN bytes
    std::vector<uint8_t> cmd(count, DYMO_SYN);
    return sendCommand(cmd.data(), cmd.size());
}

// ============================================================================
// High-Level Print Operations
// ============================================================================

bool DymoUSB::printLabel(const uint8_t* imageData, int width, int height) {
    if (!_connected) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Printer not connected");
        #endif
        return false;
    }

    if (_printing) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Printer busy");
        #endif
        return false;
    }

    _printing = true;

    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Printing label: %dx%d pixels\n", width, height);
    #endif

    // Convert image to DYMO raster format
    std::vector<uint8_t> rasterData = convertToDymoRaster(imageData, width, height);

    // Send image data using real protocol
    bool success = sendImage(rasterData.data(), width, height);

    _printing = false;
    return success;
}

bool DymoUSB::printText(const String& text, int fontSize, const String& align) {
    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Printing text: '%s' (size: %d, align: %s)\n",
                  text.c_str(), fontSize, align.c_str());
    #endif

    // Convert text to bitmap image
    std::vector<uint8_t> imageData = textToImage(text, fontSize, align);

    if (imageData.empty()) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Failed to generate text image");
        #endif
        return false;
    }

    // Calculate image dimensions
    int imageWidth = calculatePixelHeight();  // Tape width in pixels
    int imageHeight = imageData.size() / (imageWidth / 8);

    // Print the image
    return printLabel(imageData.data(), imageWidth, imageHeight);
}

bool DymoUSB::printQRCode(const String& data, int size) {
    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Printing QR code: '%s' (size: %d)\n", data.c_str(), size);
    #endif

    // Generate QR code image
    std::vector<uint8_t> imageData = qrToImage(data, size);

    if (imageData.empty()) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Failed to generate QR code");
        #endif
        return false;
    }

    // Calculate dimensions
    int imageWidth = calculatePixelHeight();
    int imageHeight = imageData.size() / (imageWidth / 8);

    return printLabel(imageData.data(), imageWidth, imageHeight);
}

bool DymoUSB::printBarcode(const String& data, const String& type) {
    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Printing barcode: '%s' (type: %s)\n", data.c_str(), type.c_str());
    #endif

    // Generate barcode image
    std::vector<uint8_t> imageData = barcodeToImage(data, type);

    if (imageData.empty()) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Failed to generate barcode");
        #endif
        return false;
    }

    int imageWidth = calculatePixelHeight();
    int imageHeight = imageData.size() / (imageWidth / 8);

    return printLabel(imageData.data(), imageWidth, imageHeight);
}

bool DymoUSB::feedLabel() {
    return cmdCut();
}

void DymoUSB::reset() {
    _connected = false;
    _printing = false;
    begin();
}

String DymoUSB::getStatus() {
    if (!_connected) {
        return "disconnected";
    }
    if (_printing) {
        return "printing";
    }
    return "ready";
}

bool DymoUSB::isPrinting() {
    return _printing;
}

void DymoUSB::setTapeWidth(uint8_t widthMM) {
    _tapeWidth = widthMM;
    _bytesPerLine = calculateBytesPerLine();

    // Update printer configuration
    if (_connected) {
        cmdBytesPerLine(_bytesPerLine);
    }

    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Tape width set to %dmm (%d bytes/line)\n", _tapeWidth, _bytesPerLine);
    #endif
}

uint8_t DymoUSB::getTapeWidth() {
    return _tapeWidth;
}

// ============================================================================
// Low-Level USB Communication
// ============================================================================

bool DymoUSB::sendCommand(const uint8_t* data, size_t length) {
    // TODO: Implement actual USB communication with USB Host Shield
    // For now, just log the command

    #if DEBUG_SERIAL
    Serial.print("[DYMO] Sending command: ");
    for (size_t i = 0; i < length; i++) {
        Serial.printf("0x%02X ", data[i]);
    }
    Serial.println();
    #endif

    // Simulate USB write
    delay(1);
    return true;
}

bool DymoUSB::sendCommandWithResponse(const uint8_t* data, size_t length,
                                     uint8_t* response, size_t responseLen) {
    // Send command
    if (!sendCommand(data, length)) {
        return false;
    }

    // TODO: Read response from printer via USB
    // For now, simulate response
    delay(5);

    // Simulate status response
    if (response && responseLen >= 8) {
        memset(response, 0, responseLen);
        response[0] = 0x00;  // Status byte
    }

    return true;
}

bool DymoUSB::sendImage(const uint8_t* imageData, int width, int height) {
    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Sending image: %dx%d pixels\n", width, height);
    #endif

    // Set dot tab (vertical offset)
    if (_dotTab > 0) {
        cmdDotTab(_dotTab);
    }

    // Send each raster line
    int bytesPerLine = width / 8;

    for (int y = 0; y < height; y++) {
        const uint8_t* lineData = imageData + (y * bytesPerLine);

        if (!cmdSendLine(lineData, bytesPerLine)) {
            #if DEBUG_SERIAL
            Serial.printf("[DYMO] Failed to send line %d\n", y);
            #endif
            return false;
        }
    }

    // Add trailing margin
    cmdSkipLines(DYMO_DEFAULT_MARGIN_LINES);

    // Request status
    cmdStatus();

    #if DEBUG_SERIAL
    Serial.println("[DYMO] ✓ Image sent successfully");
    #endif

    delay(100);  // Simulate print time
    return true;
}

// ============================================================================
// Image Processing (Placeholder implementations)
// ============================================================================

std::vector<uint8_t> DymoUSB::textToImage(const String& text, int fontSize, const String& align) {
    // TODO: Implement text rendering to bitmap
    // This would use a font library (e.g., Adafruit GFX, U8g2, or custom font)

    #if DEBUG_SERIAL
    Serial.println("[DYMO] Text rendering not yet implemented - returning placeholder");
    #endif

    // Return placeholder: simple pattern for testing
    int height = fontSize * 3;
    int bytesPerLine = calculatePixelHeight() / 8;
    std::vector<uint8_t> bitmap(bytesPerLine * height, 0x00);

    // Create simple pattern
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < bytesPerLine; j++) {
            bitmap[i * bytesPerLine + j] = (i % 4 == 0) ? 0xFF : 0x00;
        }
    }

    return bitmap;
}

std::vector<uint8_t> DymoUSB::qrToImage(const String& data, int size) {
    // TODO: Implement QR code generation
    // Would use qrcode library: https://github.com/ricmoo/QRCode

    #if DEBUG_SERIAL
    Serial.println("[DYMO] QR code generation not yet implemented - returning placeholder");
    #endif

    int qrSize = size * 21;  // QR codes are 21x21 modules minimum
    int bytesPerLine = calculatePixelHeight() / 8;
    std::vector<uint8_t> bitmap(bytesPerLine * qrSize, 0x00);

    // Placeholder pattern
    for (int i = 0; i < qrSize; i++) {
        for (int j = 0; j < bytesPerLine; j++) {
            bitmap[i * bytesPerLine + j] = (i + j) % 2 ? 0xAA : 0x55;
        }
    }

    return bitmap;
}

std::vector<uint8_t> DymoUSB::barcodeToImage(const String& data, const String& type) {
    // TODO: Implement barcode generation
    // Could use libraries for Code128, Code39, EAN13, etc.

    #if DEBUG_SERIAL
    Serial.println("[DYMO] Barcode generation not yet implemented - returning placeholder");
    #endif

    int height = 50;
    int bytesPerLine = calculatePixelHeight() / 8;
    std::vector<uint8_t> bitmap(bytesPerLine * height, 0x00);

    // Placeholder barcode pattern (vertical stripes)
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < bytesPerLine; j++) {
            bitmap[i * bytesPerLine + j] = (j % 2) ? 0xFF : 0x00;
        }
    }

    return bitmap;
}

std::vector<uint8_t> DymoUSB::convertToDymoRaster(const uint8_t* imageData, int width, int height) {
    // DYMO expects raster data in rows
    // Each row is width/8 bytes (1 bit per pixel, MSB first)
    // Already in correct format if imageData is properly formatted

    int bytesPerLine = width / 8;
    std::vector<uint8_t> rasterData(bytesPerLine * height);

    // Copy image data (already in raster format)
    memcpy(rasterData.data(), imageData, rasterData.size());

    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Converted to raster: %d bytes (%d lines x %d bytes/line)\n",
                  rasterData.size(), height, bytesPerLine);
    #endif

    return rasterData;
}

// ============================================================================
// Tape Parameter Calculations (based on labelle formula)
// ============================================================================

uint8_t DymoUSB::calculateBytesPerLine() {
    // Formula from labelle: (tape_width_mm * 8) / 12
    // For 6mm: (6*8)/12 = 4 bytes
    // For 9mm: (9*8)/12 = 6 bytes
    // For 12mm: (12*8)/12 = 8 bytes
    // For 19mm: (19*8)/12 = 12.67 -> 13 bytes

    return (_tapeWidth * 8) / 12;
}

uint16_t DymoUSB::calculatePixelHeight() {
    // Height in pixels = 8 * bytes_per_line
    return calculateBytesPerLine() * 8;
}
