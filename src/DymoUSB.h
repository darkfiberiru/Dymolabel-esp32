#ifndef DYMO_USB_H
#define DYMO_USB_H

#include <Arduino.h>
#include <vector>

// USB Host Shield Library
// Uncomment when using USB Host Shield
// #include <usbhid.h>
// #include <hiduniversal.h>
// #include <usbhub.h>

// DYMO LabelManager PnP USB Device IDs
#define DYMO_VENDOR_ID  0x0922
#define DYMO_PRODUCT_ID 0x1002  // After mode switch (from 0x1001)

// DYMO Protocol Constants (based on labelle/dymoprint)
#define DYMO_ESC        0x1B    // Escape character for commands
#define DYMO_SYN        0x16    // Synchronization byte for data lines

// DYMO Tape Specifications
#define DYMO_TAPE_6MM   6       // 6mm tape = 48 pixels height
#define DYMO_TAPE_9MM   9       // 9mm tape = 64 pixels height
#define DYMO_TAPE_12MM  12      // 12mm tape = 64 pixels height (default)
#define DYMO_TAPE_19MM  19      // 19mm tape = 128 pixels height

// Default Configuration
#define DYMO_DEFAULT_TAPE_WIDTH DYMO_TAPE_12MM
#define DYMO_MAX_BYTES_PER_LINE 8   // 64 pixels / 8 = 8 bytes per line
#define DYMO_DEFAULT_MARGIN_LINES 56 // Trailing margin after label

class DymoUSB {
public:
    DymoUSB();

    // Initialize printer communication
    bool begin();

    // Check if printer is connected and ready
    bool isReady();

    // Print operations
    bool printLabel(const uint8_t* imageData, int width, int height);
    bool printText(const String& text, int fontSize = 12, const String& align = "left");
    bool printQRCode(const String& data, int size = 3);
    bool printBarcode(const String& data, const String& type = "CODE128");

    // Printer control
    bool feedLabel();
    void reset();

    // Status
    String getStatus();
    bool isPrinting();

    // Configuration
    void setTapeWidth(uint8_t widthMM);
    uint8_t getTapeWidth();

private:
    bool _connected;
    bool _printing;
    uint8_t _tapeWidth;
    uint8_t _bytesPerLine;
    uint8_t _dotTab;  // Vertical offset (0-8)

    // DYMO Protocol Commands
    bool cmdStatus();
    bool cmdDotTab(uint8_t value);
    bool cmdTapeColor(uint8_t value);
    bool cmdBytesPerLine(uint8_t value);
    bool cmdCut();
    bool cmdSendLine(const uint8_t* lineData, size_t length);
    bool cmdSkipLines(uint16_t count);

    // Low-level USB communication
    bool sendCommand(const uint8_t* data, size_t length);
    bool sendCommandWithResponse(const uint8_t* data, size_t length, uint8_t* response, size_t responseLen);
    bool sendImage(const uint8_t* imageData, int width, int height);

    // USB Host Shield methods (when using USB Host Shield)
    // bool initUSBHost();
    // bool detectPrinter();

    // Image processing
    std::vector<uint8_t> textToImage(const String& text, int fontSize, const String& align);
    std::vector<uint8_t> qrToImage(const String& data, int size);
    std::vector<uint8_t> barcodeToImage(const String& data, const String& type);

    // Convert image to DYMO raster format
    std::vector<uint8_t> convertToDymoRaster(const uint8_t* imageData, int width, int height);

    // Calculate tape parameters
    uint8_t calculateBytesPerLine();
    uint16_t calculatePixelHeight();
};

#endif
