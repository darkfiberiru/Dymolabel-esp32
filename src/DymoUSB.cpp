#include "DymoUSB.h"
#include "config.h"

DymoUSB::DymoUSB() : _connected(false), _printing(false) {
}

bool DymoUSB::begin() {
    #if DEBUG_SERIAL
    Serial.println("[DYMO] Initializing DYMO printer...");
    #endif

    // Initialize USB communication
    // Note: ESP32-S3 USB host implementation would go here
    // For now, we'll simulate connection
    delay(100);

    // Send sync command
    uint8_t syncCmd[] = {DYMO_CMD_SYNC};
    if (sendCommand(syncCmd, sizeof(syncCmd))) {
        _connected = true;
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Printer connected successfully");
        #endif
        return true;
    }

    #if DEBUG_SERIAL
    Serial.println("[DYMO] Failed to connect to printer");
    #endif
    return false;
}

bool DymoUSB::isReady() {
    return _connected && !_printing;
}

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

    // Convert image to DYMO format
    std::vector<uint8_t> dymoData = convertToDymoFormat(imageData, width, height);

    // Send image data
    bool success = sendImage(dymoData.data(), width, height);

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
        return false;
    }

    // Print the image
    return printLabel(imageData.data(), DYMO_LABEL_WIDTH, imageData.size() / (DYMO_LABEL_WIDTH / 8));
}

bool DymoUSB::printQRCode(const String& data, int size) {
    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Printing QR code: '%s' (size: %d)\n", data.c_str(), size);
    #endif

    // Generate QR code image
    std::vector<uint8_t> imageData = qrToImage(data, size);

    if (imageData.empty()) {
        return false;
    }

    // Print the QR code
    return printLabel(imageData.data(), DYMO_LABEL_WIDTH, imageData.size() / (DYMO_LABEL_WIDTH / 8));
}

bool DymoUSB::printBarcode(const String& data, const String& type) {
    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Printing barcode: '%s' (type: %s)\n", data.c_str(), type.c_str());
    #endif

    // Generate barcode image
    std::vector<uint8_t> imageData = barcodeToImage(data, type);

    if (imageData.empty()) {
        return false;
    }

    // Print the barcode
    return printLabel(imageData.data(), DYMO_LABEL_WIDTH, imageData.size() / (DYMO_LABEL_WIDTH / 8));
}

bool DymoUSB::feedLabel() {
    uint8_t feedCmd[] = {DYMO_CMD_FORM_FEED};
    return sendCommand(feedCmd, sizeof(feedCmd));
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

// Private methods

bool DymoUSB::sendCommand(const uint8_t* data, size_t length) {
    // TODO: Implement actual USB communication with DYMO printer
    // For now, simulate successful command

    #if DEBUG_SERIAL
    Serial.print("[DYMO] Sending command: ");
    for (size_t i = 0; i < length; i++) {
        Serial.printf("0x%02X ", data[i]);
    }
    Serial.println();
    #endif

    return true;
}

bool DymoUSB::sendImage(const uint8_t* imageData, int width, int height) {
    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Sending image: %dx%d pixels\n", width, height);
    #endif

    // TODO: Implement actual image transmission to printer
    // DYMO printers expect raster data in specific format

    delay(100); // Simulate print time
    return true;
}

std::vector<uint8_t> DymoUSB::textToImage(const String& text, int fontSize, const String& align) {
    // TODO: Implement text rendering to bitmap
    // This would use a font library to render text to a bitmap image

    // For now, create a simple placeholder bitmap
    int height = fontSize * 2;
    int bytesPerRow = DYMO_LABEL_WIDTH / 8;
    std::vector<uint8_t> bitmap(bytesPerRow * height, 0);

    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Created text bitmap: %dx%d\n", DYMO_LABEL_WIDTH, height);
    #endif

    return bitmap;
}

std::vector<uint8_t> DymoUSB::qrToImage(const String& data, int size) {
    // TODO: Implement QR code generation
    // Would use a QR code library like qrcode.h

    int qrSize = size * 21; // QR codes are typically 21x21 modules minimum
    int bytesPerRow = DYMO_LABEL_WIDTH / 8;
    std::vector<uint8_t> bitmap(bytesPerRow * qrSize, 0);

    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Created QR code bitmap: %dx%d\n", DYMO_LABEL_WIDTH, qrSize);
    #endif

    return bitmap;
}

std::vector<uint8_t> DymoUSB::barcodeToImage(const String& data, const String& type) {
    // TODO: Implement barcode generation
    // Would use a barcode library

    int height = 50;
    int bytesPerRow = DYMO_LABEL_WIDTH / 8;
    std::vector<uint8_t> bitmap(bytesPerRow * height, 0);

    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Created barcode bitmap: %dx%d\n", DYMO_LABEL_WIDTH, height);
    #endif

    return bitmap;
}

std::vector<uint8_t> DymoUSB::convertToDymoFormat(const uint8_t* imageData, int width, int height) {
    // DYMO printers expect raster data
    // Each row is width/8 bytes (1 bit per pixel)

    int bytesPerRow = width / 8;
    std::vector<uint8_t> dymoData;

    // Add DYMO raster command header
    dymoData.push_back(0x1B); // ESC
    dymoData.push_back(0x69); // 'i'
    dymoData.push_back(0x61); // 'a'
    dymoData.push_back(width & 0xFF);
    dymoData.push_back((width >> 8) & 0xFF);

    // Add image data
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < bytesPerRow; x++) {
            dymoData.push_back(imageData[y * bytesPerRow + x]);
        }
    }

    return dymoData;
}
