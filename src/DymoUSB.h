#ifndef DYMO_USB_H
#define DYMO_USB_H

#include <Arduino.h>
#include <vector>

// DYMO printer commands
#define DYMO_CMD_STATUS 0x1B
#define DYMO_CMD_FORM_FEED 0x1B, 0x45
#define DYMO_CMD_SYNC 0x16

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

private:
    bool _connected;
    bool _printing;

    // Low-level communication
    bool sendCommand(const uint8_t* data, size_t length);
    bool sendImage(const uint8_t* imageData, int width, int height);

    // Image processing
    std::vector<uint8_t> textToImage(const String& text, int fontSize, const String& align);
    std::vector<uint8_t> qrToImage(const String& data, int size);
    std::vector<uint8_t> barcodeToImage(const String& data, const String& type);

    // Convert image to DYMO format
    std::vector<uint8_t> convertToDymoFormat(const uint8_t* imageData, int width, int height);
};

#endif
