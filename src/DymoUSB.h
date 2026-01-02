#ifndef DYMO_USB_H
#define DYMO_USB_H

#include <Arduino.h>
#include <vector>

// ESP-IDF USB Host Library (native ESP32-S3 USB OTG)
#ifdef USE_ESP_IDF_USB_HOST
#include "usb/usb_host.h"
#endif

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

// Fix #13: Timing constants (milliseconds)
#define DYMO_USB_ENUM_DELAY_MS 100      // USB device enumeration delay
#define DYMO_USB_TASK_EXIT_DELAY_MS 100 // Time for USB task to exit
#define DYMO_PRINT_COMPLETE_DELAY_MS 100// Simulated print time
#define DYMO_SIM_COMMAND_DELAY_MS 1     // Simulation mode command delay
#define DYMO_SIM_RESPONSE_DELAY_MS 5    // Simulation mode response delay

// Fix #14: Input validation limits
#define DYMO_MAX_TEXT_LENGTH 200        // Maximum characters for text printing
#define DYMO_MAX_QR_DATA_LENGTH 500     // Maximum characters for QR code
#define DYMO_MAX_BARCODE_LENGTH 100     // Maximum characters for barcode

class DymoUSB {
public:
    DymoUSB();
    ~DymoUSB();  // Destructor for proper cleanup

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

    // Fix #12: Mutex for thread-safe access to shared flags
    SemaphoreHandle_t _stateMutex;

#ifdef USE_ESP_IDF_USB_HOST
    // ESP-IDF USB Host members
    usb_device_handle_t _usbDevice;
    usb_host_client_handle_t _usbClientHandle;
    uint8_t _usbOutEndpoint;
    uint8_t _usbInEndpoint;
    bool _usbHostInitialized;

    // Instance members (moved from static to support multiple instances)
    bool _usbHostLibTaskRunning;
    SemaphoreHandle_t _usbHostReadySem;
    TaskHandle_t _usbHostTaskHandle;

    // Transfer synchronization
    SemaphoreHandle_t _transferCompleteSem;
    volatile esp_err_t _lastTransferStatus;
#endif

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

#ifdef USE_ESP_IDF_USB_HOST
    // ESP-IDF USB Host methods
    bool initUSBHost();
    bool detectAndOpenPrinter();
    void cleanupUSBHost();
    static void usbHostLibTask(void* arg);
    static void usbClientEventCallback(const usb_host_client_event_msg_t* event_msg, void* arg);
    static void usbTransferCallback(usb_transfer_t* transfer);
#endif

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
