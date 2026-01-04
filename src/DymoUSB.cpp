#include "DymoUSB.h"
#include "config.h"
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include "qrcode.h"

// Custom GFX canvas for monochrome bitmap
class MonoBitmap : public Adafruit_GFX {
public:
    MonoBitmap(uint16_t w, uint16_t h) : Adafruit_GFX(w, h) {
        _width = w;
        _height = h;
        // Fix #5: Round up to nearest byte boundary to prevent buffer overflow
        size_t bytesPerRow = (w + 7) / 8;  // Ceiling division
        _buffer.resize(bytesPerRow * h, 0);
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        if (x < 0 || x >= _width || y < 0 || y >= _height) return;

        size_t bytesPerRow = (_width + 7) / 8;
        size_t byteIndex = y * bytesPerRow + (x / 8);
        int bitIndex = 7 - (x % 8);

        // Bounds check to prevent buffer overflow
        if (byteIndex >= _buffer.size()) return;

        if (color) {
            _buffer[byteIndex] |= (1 << bitIndex);
        } else {
            _buffer[byteIndex] &= ~(1 << bitIndex);
        }
    }

    std::vector<uint8_t>& getBuffer() { return _buffer; }
    void clear() { std::fill(_buffer.begin(), _buffer.end(), 0); }

private:
    std::vector<uint8_t> _buffer;
};

DymoUSB::DymoUSB() : _connected(false), _printing(false),
                     _tapeWidth(DYMO_DEFAULT_TAPE_WIDTH), _dotTab(0) {
    _bytesPerLine = calculateBytesPerLine();

    // Fix #12: Initialize mutex for thread-safe flag access
    _stateMutex = xSemaphoreCreateMutex();

#ifdef USE_ESP_IDF_USB_HOST
    _usbDevice = NULL;
    _usbClientHandle = NULL;
    _usbOutEndpoint = 0;
    _usbInEndpoint = 0;
    _usbHostInitialized = false;

    // Initialize instance members (Fix #3: moved from static)
    _usbHostLibTaskRunning = false;
    _usbHostReadySem = NULL;
    _usbHostTaskHandle = NULL;

    // Initialize transfer synchronization (Fix #1)
    _transferCompleteSem = xSemaphoreCreateBinary();
    _lastTransferStatus = ESP_OK;
#endif
}

// Fix #2: Destructor for proper USB Host cleanup
DymoUSB::~DymoUSB() {
#ifdef USE_ESP_IDF_USB_HOST
    cleanupUSBHost();

    // Delete semaphores
    if (_transferCompleteSem) {
        vSemaphoreDelete(_transferCompleteSem);
        _transferCompleteSem = NULL;
    }
#endif

    // Fix #12: Delete state mutex
    if (_stateMutex) {
        vSemaphoreDelete(_stateMutex);
        _stateMutex = NULL;
    }
}

bool DymoUSB::begin() {
    #if DEBUG_SERIAL
    Serial.println("[DYMO] Initializing DYMO LabelManager PnP...");
    Serial.printf("[DYMO] Vendor ID: 0x%04X, Product ID: 0x%04X\n", DYMO_VENDOR_ID, DYMO_PRODUCT_ID);
    #endif

#ifdef USE_ESP_IDF_USB_HOST
    // Initialize ESP-IDF USB Host
    if (!initUSBHost()) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] USB Host initialization failed");
        #endif
        return false;
    }

    // Detect and open DYMO printer
    if (!detectAndOpenPrinter()) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] DYMO printer not detected");
        Serial.println("[DYMO] Please connect DYMO LabelManager PnP via USB");
        #endif
        return false;
    }
#else
    #if DEBUG_SERIAL
    Serial.println("[DYMO] Running in simulation mode (USE_ESP_IDF_USB_HOST not defined)");
    #endif
#endif

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
        // Fix #12: Thread-safe flag write
        if (_stateMutex) {
            xSemaphoreTake(_stateMutex, portMAX_DELAY);
            _connected = true;
            xSemaphoreGive(_stateMutex);
        }
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
    // Fix #12: Thread-safe flag write
    if (_stateMutex) {
        xSemaphoreTake(_stateMutex, portMAX_DELAY);
        _connected = true;
        xSemaphoreGive(_stateMutex);
    }
    return true;
}

bool DymoUSB::isReady() {
    // Fix #12: Thread-safe flag access
    if (!_stateMutex) return false;
    xSemaphoreTake(_stateMutex, portMAX_DELAY);
    bool ready = _connected && !_printing;
    xSemaphoreGive(_stateMutex);
    return ready;
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
    // Fix #12: Thread-safe flag checking
    if (!_stateMutex) return false;

    xSemaphoreTake(_stateMutex, portMAX_DELAY);
    bool connected = _connected;
    bool printing = _printing;
    xSemaphoreGive(_stateMutex);

    if (!connected) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Printer not connected");
        #endif
        return false;
    }

    if (printing) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Printer busy");
        #endif
        return false;
    }

    // Set printing flag
    xSemaphoreTake(_stateMutex, portMAX_DELAY);
    _printing = true;
    xSemaphoreGive(_stateMutex);

    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Printing label: %dx%d pixels\n", width, height);
    #endif

    // Convert image to DYMO raster format
    std::vector<uint8_t> rasterData = convertToDymoRaster(imageData, width, height);

    // Send image data using real protocol
    bool success = sendImage(rasterData.data(), width, height);

    // Clear printing flag
    xSemaphoreTake(_stateMutex, portMAX_DELAY);
    _printing = false;
    xSemaphoreGive(_stateMutex);

    return success;
}

bool DymoUSB::printText(const String& text, int fontSize, const String& align) {
    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Printing text: '%s' (size: %d, align: %s)\n",
                  text.c_str(), fontSize, align.c_str());
    #endif

    // Fix #14: Validate input length
    if (text.length() > DYMO_MAX_TEXT_LENGTH) {
        #if DEBUG_SERIAL
        Serial.printf("[DYMO] Text too long: %d chars (max %d)\n",
                     text.length(), DYMO_MAX_TEXT_LENGTH);
        #endif
        return false;
    }

    // Convert text to bitmap image
    std::vector<uint8_t> imageData = textToImage(text, fontSize, align);

    if (imageData.empty()) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Failed to generate text image");
        #endif
        return false;
    }

    // Fix #4: Validate dimensions BEFORE division to prevent divide-by-zero
    int imageWidth = calculatePixelHeight();  // Tape width in pixels
    if (imageWidth < 8) {
        #if DEBUG_SERIAL
        Serial.printf("[DYMO] Invalid tape width: %d pixels (minimum 8)\n", imageWidth);
        #endif
        return false;
    }

    int bytesPerLine = imageWidth / 8;
    if (bytesPerLine == 0 || imageData.size() % bytesPerLine != 0) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Image size mismatch");
        #endif
        return false;
    }

    int imageHeight = imageData.size() / bytesPerLine;

    // Print the image
    return printLabel(imageData.data(), imageWidth, imageHeight);
}

bool DymoUSB::printQRCode(const String& data, int size) {
    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Printing QR code: '%s' (size: %d)\n", data.c_str(), size);
    #endif

    // Fix #14: Validate input length
    if (data.length() > DYMO_MAX_QR_DATA_LENGTH) {
        #if DEBUG_SERIAL
        Serial.printf("[DYMO] QR data too long: %d chars (max %d)\n",
                     data.length(), DYMO_MAX_QR_DATA_LENGTH);
        #endif
        return false;
    }

    // Generate QR code image
    std::vector<uint8_t> imageData = qrToImage(data, size);

    if (imageData.empty()) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Failed to generate QR code");
        #endif
        return false;
    }

    // Fix #4: Validate dimensions
    int imageWidth = calculatePixelHeight();
    if (imageWidth < 8) {
        #if DEBUG_SERIAL
        Serial.printf("[DYMO] Invalid tape width: %d pixels (minimum 8)\n", imageWidth);
        #endif
        return false;
    }

    int bytesPerLine = imageWidth / 8;
    if (bytesPerLine == 0 || imageData.size() % bytesPerLine != 0) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Image size mismatch");
        #endif
        return false;
    }

    int imageHeight = imageData.size() / bytesPerLine;

    return printLabel(imageData.data(), imageWidth, imageHeight);
}

bool DymoUSB::printBarcode(const String& data, const String& type) {
    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Printing barcode: '%s' (type: %s)\n", data.c_str(), type.c_str());
    #endif

    // Fix #14: Validate input length
    if (data.length() > DYMO_MAX_BARCODE_LENGTH) {
        #if DEBUG_SERIAL
        Serial.printf("[DYMO] Barcode data too long: %d chars (max %d)\n",
                     data.length(), DYMO_MAX_BARCODE_LENGTH);
        #endif
        return false;
    }

    // Generate barcode image
    std::vector<uint8_t> imageData = barcodeToImage(data, type);

    if (imageData.empty()) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Failed to generate barcode");
        #endif
        return false;
    }

    // Fix #4: Validate dimensions
    int imageWidth = calculatePixelHeight();
    if (imageWidth < 8) {
        #if DEBUG_SERIAL
        Serial.printf("[DYMO] Invalid tape width: %d pixels (minimum 8)\n", imageWidth);
        #endif
        return false;
    }

    int bytesPerLine = imageWidth / 8;
    if (bytesPerLine == 0 || imageData.size() % bytesPerLine != 0) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Image size mismatch");
        #endif
        return false;
    }

    int imageHeight = imageData.size() / bytesPerLine;

    return printLabel(imageData.data(), imageWidth, imageHeight);
}

bool DymoUSB::feedLabel() {
    return cmdCut();
}

void DymoUSB::reset() {
    // Fix #12: Thread-safe flag write
    if (_stateMutex) {
        xSemaphoreTake(_stateMutex, portMAX_DELAY);
        _connected = false;
        _printing = false;
        xSemaphoreGive(_stateMutex);
    }
    begin();
}

String DymoUSB::getStatus() {
    // Fix #12: Thread-safe flag access
    if (!_stateMutex) return "error";

    xSemaphoreTake(_stateMutex, portMAX_DELAY);
    bool connected = _connected;
    bool printing = _printing;
    xSemaphoreGive(_stateMutex);

    if (!connected) {
        return "disconnected";
    }
    if (printing) {
        return "printing";
    }
    return "ready";
}

bool DymoUSB::isPrinting() {
    // Fix #12: Thread-safe flag access
    if (!_stateMutex) return false;

    xSemaphoreTake(_stateMutex, portMAX_DELAY);
    bool printing = _printing;
    xSemaphoreGive(_stateMutex);

    return printing;
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
    #if DEBUG_SERIAL
    Serial.print("[DYMO] Sending command: ");
    for (size_t i = 0; i < length; i++) {
        Serial.printf("0x%02X ", data[i]);
    }
    Serial.println();
    #endif

#ifdef USE_ESP_IDF_USB_HOST
    // Fix #1: Proper USB transfer synchronization with callback
    if (!_connected || !_usbDevice || _usbOutEndpoint == 0) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] USB not connected or endpoint not configured");
        #endif
        return false;
    }

    // Allocate USB transfer
    usb_transfer_t *transfer;
    esp_err_t err = usb_host_transfer_alloc(length, 0, &transfer);
    if (err != ESP_OK) {
        #if DEBUG_SERIAL
        Serial.printf("[DYMO] Transfer alloc failed: %s\n", esp_err_to_name(err));
        #endif
        return false;
    }

    // Setup transfer with callback
    transfer->device_handle = _usbDevice;
    transfer->bEndpointAddress = _usbOutEndpoint;
    transfer->callback = usbTransferCallback;  // Use callback for proper sync
    transfer->context = this;                  // Pass instance pointer
    transfer->num_bytes = length;
    memcpy(transfer->data_buffer, data, length);
    transfer->timeout_ms = 1000;

    // Submit transfer
    err = usb_host_transfer_submit(transfer);
    if (err != ESP_OK) {
        #if DEBUG_SERIAL
        Serial.printf("[DYMO] Transfer submit failed: %s\n", esp_err_to_name(err));
        #endif
        usb_host_transfer_free(transfer);
        return false;
    }

    // Wait for completion properly using semaphore
    bool completed = (xSemaphoreTake(_transferCompleteSem, pdMS_TO_TICKS(2000)) == pdTRUE);

    if (!completed) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Transfer timeout");
        #endif
        usb_host_transfer_free(transfer);
        return false;
    }

    // Check transfer status
    if (_lastTransferStatus != ESP_OK || transfer->status != USB_TRANSFER_STATUS_COMPLETED) {
        #if DEBUG_SERIAL
        Serial.printf("[DYMO] Transfer failed: status=%d\n", transfer->status);
        #endif
        usb_host_transfer_free(transfer);
        return false;
    }

    usb_host_transfer_free(transfer);

    #if DEBUG_SERIAL
    Serial.println("[DYMO] Command sent via USB");
    #endif

    return true;
#else
    // Simulation mode
    delay(DYMO_SIM_COMMAND_DELAY_MS);  // Fix #13: Use named constant
    return true;
#endif
}

bool DymoUSB::sendCommandWithResponse(const uint8_t* data, size_t length,
                                     uint8_t* response, size_t responseLen) {
    // Send command
    if (!sendCommand(data, length)) {
        return false;
    }

#ifdef USE_ESP_IDF_USB_HOST
    if (!_connected || !_usbDevice || _usbInEndpoint == 0) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] USB not connected or IN endpoint not configured");
        #endif
        return false;
    }

    if (!response || responseLen == 0) {
        return true;  // No response expected
    }

    // Fix #1: Proper USB transfer synchronization for IN transfers
    usb_transfer_t *transfer;
    esp_err_t err = usb_host_transfer_alloc(responseLen, 0, &transfer);
    if (err != ESP_OK) {
        #if DEBUG_SERIAL
        Serial.printf("[DYMO] Transfer alloc (IN) failed: %s\n", esp_err_to_name(err));
        #endif
        return false;
    }

    // Setup transfer with callback
    transfer->device_handle = _usbDevice;
    transfer->bEndpointAddress = _usbInEndpoint;
    transfer->callback = usbTransferCallback;  // Use callback
    transfer->context = this;                   // Pass instance
    transfer->num_bytes = responseLen;
    transfer->timeout_ms = 1000;

    // Submit transfer
    err = usb_host_transfer_submit(transfer);
    if (err != ESP_OK) {
        #if DEBUG_SERIAL
        Serial.printf("[DYMO] Transfer submit (IN) failed: %s\n", esp_err_to_name(err));
        #endif
        usb_host_transfer_free(transfer);
        return false;
    }

    // Wait for completion properly using semaphore
    bool completed = (xSemaphoreTake(_transferCompleteSem, pdMS_TO_TICKS(2000)) == pdTRUE);

    if (!completed) {
        #if DEBUG_SERIAL
        Serial.println("[DYMO] Transfer (IN) timeout");
        #endif
        usb_host_transfer_free(transfer);
        return false;
    }

    // Check transfer status
    if (_lastTransferStatus != ESP_OK || transfer->status != USB_TRANSFER_STATUS_COMPLETED) {
        #if DEBUG_SERIAL
        Serial.printf("[DYMO] Transfer (IN) failed: status=%d\n", transfer->status);
        #endif
        usb_host_transfer_free(transfer);
        return false;
    }

    // Copy response data
    if (transfer->actual_num_bytes > 0) {
        memcpy(response, transfer->data_buffer, min((size_t)transfer->actual_num_bytes, responseLen));
        #if DEBUG_SERIAL
        Serial.printf("[DYMO] Received %d bytes\n", transfer->actual_num_bytes);
        #endif
    }

    usb_host_transfer_free(transfer);
    return true;
#else
    // Simulation mode
    delay(DYMO_SIM_RESPONSE_DELAY_MS);  // Fix #13: Use named constant
    if (response && responseLen >= 8) {
        memset(response, 0, responseLen);
        response[0] = 0x00;  // Status byte
    }
    return true;
#endif
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

    delay(DYMO_PRINT_COMPLETE_DELAY_MS);  // Fix #13: Use named constant
    return true;
}

// ============================================================================
// Image Processing (Placeholder implementations)
// ============================================================================

std::vector<uint8_t> DymoUSB::textToImage(const String& text, int fontSize, const String& align) {
    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Rendering text: '%s' (size: %d, align: %s)\n",
                  text.c_str(), fontSize, align.c_str());
    #endif

    // Calculate tape dimensions
    int tapeHeight = calculatePixelHeight();
    int maxLabelLength = 500;  // Reasonable max length

    // Create bitmap canvas
    MonoBitmap canvas(tapeHeight, maxLabelLength);
    canvas.clear();
    canvas.setRotation(0);
    canvas.setTextWrap(false);
    canvas.setTextColor(1);  // Black text

    // Select font based on size
    const GFXfont* font = nullptr;
    if (fontSize <= 9) {
        font = &FreeSans9pt7b;
    } else if (fontSize <= 12) {
        font = &FreeSans12pt7b;
    } else if (fontSize <= 18) {
        font = &FreeSans18pt7b;
    } else {
        font = &FreeSans24pt7b;
    }
    canvas.setFont(font);

    // Get text bounds
    int16_t x1, y1;
    uint16_t w, h;
    canvas.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);

    // Calculate position based on alignment
    int16_t x = 2;  // Small left margin
    int16_t y = h + 2;  // Baseline position

    if (align == "center") {
        x = (tapeHeight - w) / 2;
    } else if (align == "right") {
        x = tapeHeight - w - 2;
    }

    // Ensure text fits
    if (x < 0) x = 2;

    // Draw text
    canvas.setCursor(x, y);
    canvas.print(text);

    // Calculate actual used height
    int usedHeight = h + 10;  // Text height plus margins

    // Rotate 90 degrees for label orientation
    // DYMO labels print with text rotated 90° from tape feed direction
    std::vector<uint8_t> rotated((tapeHeight / 8) * usedHeight, 0);

    for (int y = 0; y < usedHeight; y++) {
        for (int x = 0; x < tapeHeight; x++) {
            int srcByteIdx = y * (tapeHeight / 8) + (x / 8);
            int srcBitIdx = 7 - (x % 8);

            if (canvas.getBuffer()[srcByteIdx] & (1 << srcBitIdx)) {
                int dstX = tapeHeight - 1 - x;
                int dstY = y;
                int dstByteIdx = dstY * (tapeHeight / 8) + (dstX / 8);
                int dstBitIdx = 7 - (dstX % 8);
                rotated[dstByteIdx] |= (1 << dstBitIdx);
            }
        }
    }

    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Text rendered: %dx%d pixels\n", tapeHeight, usedHeight);
    #endif

    return rotated;
}

std::vector<uint8_t> DymoUSB::qrToImage(const String& data, int size) {
    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Generating QR code: '%s' (size: %d)\n", data.c_str(), size);
    #endif

    // Create QR code
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(size + 2)];  // Size 3-5 recommended

    qrcode_initText(&qrcode, qrcodeData, size + 2, ECC_LOW, data.c_str());

    int qrPixelSize = qrcode.size;
    int scale = 3;  // Scale each module to 3x3 pixels for readability
    int scaledSize = qrPixelSize * scale;
    int tapeHeight = calculatePixelHeight();

    // Add margin
    int margin = 4;
    int totalSize = scaledSize + (margin * 2);

    // Create bitmap
    std::vector<uint8_t> bitmap((tapeHeight / 8) * totalSize, 0xFF);  // White background

    // Center QR code on tape
    int offsetX = (tapeHeight - scaledSize) / 2;
    int offsetY = margin;

    // Draw QR code
    for (int y = 0; y < qrPixelSize; y++) {
        for (int x = 0; x < qrPixelSize; x++) {
            bool module = qrcode_getModule(&qrcode, x, y);

            if (module) {  // Black module
                // Scale up
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        int pixelX = offsetX + (x * scale) + sx;
                        int pixelY = offsetY + (y * scale) + sy;

                        if (pixelX >= 0 && pixelX < tapeHeight && pixelY >= 0 && pixelY < totalSize) {
                            int byteIdx = pixelY * (tapeHeight / 8) + (pixelX / 8);
                            int bitIdx = 7 - (pixelX % 8);
                            bitmap[byteIdx] &= ~(1 << bitIdx);  // Set to black
                        }
                    }
                }
            }
        }
    }

    #if DEBUG_SERIAL
    Serial.printf("[DYMO] QR code generated: %dx%d modules, %dx%d pixels\n",
                  qrPixelSize, qrPixelSize, scaledSize, scaledSize);
    #endif

    return bitmap;
}

std::vector<uint8_t> DymoUSB::barcodeToImage(const String& data, const String& type) {
    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Generating barcode: '%s' (type: %s)\n", data.c_str(), type.c_str());
    #endif

    int tapeHeight = calculatePixelHeight();
    int barcodeHeight = 40;
    int textHeight = 15;
    int totalHeight = barcodeHeight + textHeight + 10;

    // Create bitmap
    std::vector<uint8_t> bitmap((tapeHeight / 8) * totalHeight, 0xFF);  // White background

    // Simple Code 39 implementation (basic barcode)
    // For production, integrate a proper barcode library

    // Code 39 encoding (simplified - only supports numbers and uppercase)
    const char* code39[] = {
        "000110100",  // 0
        "100100001",  // 1
        "001100001",  // 2
        "101100000",  // 3
        "000110001",  // 4
        "100110000",  // 5
        "001110000",  // 6
        "000100101",  // 7
        "100100100",  // 8
        "001100100"   // 9
    };

    int barWidth = 2;  // Width of narrow bar
    int wideWidth = 5;  // Width of wide bar
    int currentX = 10;  // Start position

    // Start character (*)
    currentX += 20;

    // Encode data
    for (size_t i = 0; i < data.length() && currentX < tapeHeight - 30; i++) {
        char c = data[i];
        int idx = -1;

        if (c >= '0' && c <= '9') {
            idx = c - '0';
        }

        if (idx >= 0) {
            const char* pattern = code39[idx];

            for (int j = 0; j < 9; j++) {
                int width = (pattern[j] == '1') ? wideWidth : barWidth;
                bool isBlack = (j % 2 == 0);

                // Draw bar
                for (int w = 0; w < width && currentX < tapeHeight; w++) {
                    for (int h = 5; h < barcodeHeight + 5; h++) {
                        if (isBlack) {
                            int byteIdx = h * (tapeHeight / 8) + (currentX / 8);
                            int bitIdx = 7 - (currentX % 8);
                            bitmap[byteIdx] &= ~(1 << bitIdx);  // Black
                        }
                    }
                    currentX++;
                }
            }
            currentX += barWidth;  // Inter-character gap
        }
    }

    // Add text below barcode using simple rendering
    MonoBitmap textCanvas(tapeHeight, textHeight);
    textCanvas.clear();
    textCanvas.setTextSize(1);
    textCanvas.setTextColor(1);

    // Center text
    int textX = (tapeHeight - (data.length() * 6)) / 2;
    if (textX < 0) textX = 2;

    textCanvas.setCursor(textX, 2);
    textCanvas.print(data);

    // Copy text to barcode bitmap
    for (int y = 0; y < textHeight; y++) {
        for (int x = 0; x < tapeHeight; x++) {
            int srcByteIdx = y * (tapeHeight / 8) + (x / 8);
            int srcBitIdx = 7 - (x % 8);

            if (textCanvas.getBuffer()[srcByteIdx] & (1 << srcBitIdx)) {
                int dstY = barcodeHeight + 5 + y;
                int dstByteIdx = dstY * (tapeHeight / 8) + (x / 8);
                int dstBitIdx = 7 - (x % 8);
                bitmap[dstByteIdx] &= ~(1 << dstBitIdx);  // Black text
            }
        }
    }

    #if DEBUG_SERIAL
    Serial.printf("[DYMO] Barcode generated: %dx%d pixels\n", tapeHeight, totalHeight);
    Serial.println("[DYMO] Note: Using simplified Code 39 - integrate full barcode library for production");
    #endif

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

    // Fix #11: Use wider type for calculation to prevent integer overflow
    return (uint8_t)((uint16_t)_tapeWidth * 8 / 12);
}

uint16_t DymoUSB::calculatePixelHeight() {
    // Height in pixels = 8 * bytes_per_line
    return calculateBytesPerLine() * 8;
}

// ============================================================================
// ESP-IDF USB Host Implementation (ESP32-S3 Native USB OTG)
// ============================================================================

#ifdef USE_ESP_IDF_USB_HOST

// Fix #1: USB Transfer callback for proper synchronization
void DymoUSB::usbTransferCallback(usb_transfer_t* transfer) {
    if (!transfer || !transfer->context) return;

    DymoUSB* instance = (DymoUSB*)transfer->context;
    instance->_lastTransferStatus = (transfer->status == USB_TRANSFER_STATUS_COMPLETED) ? ESP_OK : ESP_FAIL;

    // Signal completion
    if (instance->_transferCompleteSem) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(instance->_transferCompleteSem, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

// USB Host library task - required for USB Host operations
void DymoUSB::usbHostLibTask(void* arg) {
    if (!arg) return;
    DymoUSB* instance = (DymoUSB*)arg;

    #if DEBUG_SERIAL
    Serial.println("[USB] USB Host library task started");
    #endif

    // Fix #3: Use instance member instead of static
    while (instance->_usbHostLibTaskRunning) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);

        // Handle USB Host library events
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            #if DEBUG_SERIAL
            Serial.println("[USB] No more clients");
            #endif
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            #if DEBUG_SERIAL
            Serial.println("[USB] All devices freed");
            #endif
        }
    }

    #if DEBUG_SERIAL
    Serial.println("[USB] USB Host library task ended");
    #endif
    vTaskDelete(NULL);
}

// USB Client event callback
void DymoUSB::usbClientEventCallback(const usb_host_client_event_msg_t* event_msg, void* arg) {
    if (!arg) return;
    DymoUSB* instance = (DymoUSB*)arg;

    #if DEBUG_SERIAL
    Serial.printf("[USB] Client event: %d\n", event_msg->event);
    #endif

    switch (event_msg->event) {
        case USB_HOST_CLIENT_EVENT_NEW_DEV:
            #if DEBUG_SERIAL
            Serial.printf("[USB] New device detected: address %d\n", event_msg->new_dev.address);
            #endif
            // Fix #3: Use instance member instead of static
            if (instance->_usbHostReadySem) {
                xSemaphoreGive(instance->_usbHostReadySem);
            }
            break;
        case USB_HOST_CLIENT_EVENT_DEV_GONE:
            #if DEBUG_SERIAL
            Serial.printf("[USB] Device gone: address %d\n", event_msg->dev_gone.dev_hdl);
            #endif
            break;
        default:
            break;
    }
}

bool DymoUSB::initUSBHost() {
    if (_usbHostInitialized) {
        return true;
    }

    #if DEBUG_SERIAL
    Serial.println("[USB] Initializing ESP-IDF USB Host...");
    #endif

    // Fix #3: Create semaphore as instance member
    _usbHostReadySem = xSemaphoreCreateBinary();
    if (!_usbHostReadySem) {
        #if DEBUG_SERIAL
        Serial.println("[USB] Failed to create semaphore");
        #endif
        return false;
    }

    // Install USB Host driver
    usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    esp_err_t err = usb_host_install(&host_config);
    if (err != ESP_OK) {
        #if DEBUG_SERIAL
        Serial.printf("[USB] USB Host install failed: %s\n", esp_err_to_name(err));
        #endif
        vSemaphoreDelete(_usbHostReadySem);
        _usbHostReadySem = NULL;
        return false;
    }

    #if DEBUG_SERIAL
    Serial.println("[USB] USB Host driver installed");
    #endif

    // Fix #3: Start USB Host library task with instance pointer
    _usbHostLibTaskRunning = true;
    BaseType_t task_created = xTaskCreatePinnedToCore(
        usbHostLibTask,
        "usb_host",
        4096,
        this,  // Pass instance pointer to task
        5,     // Priority
        &_usbHostTaskHandle,  // Store task handle
        0      // Core 0
    );

    if (task_created != pdPASS) {
        #if DEBUG_SERIAL
        Serial.println("[USB] Failed to create USB Host task");
        #endif
        usb_host_uninstall();
        vSemaphoreDelete(_usbHostReadySem);
        _usbHostReadySem = NULL;
        return false;
    }

    #if DEBUG_SERIAL
    Serial.println("[USB] USB Host task created");
    #endif

    // Register USB Host client
    usb_host_client_config_t client_config = {
        .is_synchronous = false,
        .max_num_event_msg = 5,
        .async = {
            .client_event_callback = usbClientEventCallback,
            .callback_arg = this  // Pass instance pointer to callback
        }
    };

    // Fix #2: Store client handle in instance member
    err = usb_host_client_register(&client_config, &_usbClientHandle);
    if (err != ESP_OK) {
        #if DEBUG_SERIAL
        Serial.printf("[USB] Client register failed: %s\n", esp_err_to_name(err));
        #endif
        _usbHostLibTaskRunning = false;
        usb_host_uninstall();
        vSemaphoreDelete(_usbHostReadySem);
        _usbHostReadySem = NULL;
        return false;
    }

    #if DEBUG_SERIAL
    Serial.println("[USB] USB Host client registered");
    Serial.println("[USB] Waiting for device connection...");
    #endif

    _usbHostInitialized = true;
    return true;
}

// Fix #2: Cleanup method for proper resource management
void DymoUSB::cleanupUSBHost() {
    #if DEBUG_SERIAL
    Serial.println("[USB] Cleaning up USB Host resources...");
    #endif

    // Close device if open
    if (_usbDevice) {
        usb_host_device_close(_usbClientHandle, _usbDevice);
        _usbDevice = NULL;
    }

    // Deregister client
    if (_usbClientHandle) {
        usb_host_client_deregister(_usbClientHandle);
        _usbClientHandle = NULL;
    }

    // Stop USB Host task
    _usbHostLibTaskRunning = false;
    if (_usbHostTaskHandle) {
        delay(DYMO_USB_TASK_EXIT_DELAY_MS);  // Fix #13: Give task time to exit
        _usbHostTaskHandle = NULL;
    }

    // Delete semaphore
    if (_usbHostReadySem) {
        vSemaphoreDelete(_usbHostReadySem);
        _usbHostReadySem = NULL;
    }

    // Uninstall USB Host
    if (_usbHostInitialized) {
        usb_host_uninstall();
        _usbHostInitialized = false;
    }

    // Fix #12: Thread-safe flag write
    if (_stateMutex) {
        xSemaphoreTake(_stateMutex, portMAX_DELAY);
        _connected = false;
        xSemaphoreGive(_stateMutex);
    }

    #if DEBUG_SERIAL
    Serial.println("[USB] USB Host cleanup complete");
    #endif
}

bool DymoUSB::detectAndOpenPrinter() {
    #if DEBUG_SERIAL
    Serial.println("[USB] Detecting DYMO printer...");
    #endif

    // Fix #3: Wait for device detection using instance member
    if (xSemaphoreTake(_usbHostReadySem, pdMS_TO_TICKS(5000)) != pdTRUE) {
        #if DEBUG_SERIAL
        Serial.println("[USB] No device detected (timeout)");
        #endif
        return false;
    }

    delay(DYMO_USB_ENUM_DELAY_MS);  // Fix #13: Give time for enumeration

    // Fix #7: Get device count first (parameter is buffer size, not max)
    int num_devices = 0;
    esp_err_t err = usb_host_device_addr_list_fill(0, NULL, &num_devices);
    if (err != ESP_OK || num_devices == 0) {
        #if DEBUG_SERIAL
        Serial.println("[USB] No USB devices found");
        #endif
        return false;
    }

    // Fix #7: Limit to reasonable maximum to prevent stack overflow
    #define MAX_USB_DEVICES 16
    if (num_devices > MAX_USB_DEVICES) {
        #if DEBUG_SERIAL
        Serial.printf("[USB] Too many devices: %d (max %d), truncating\n", num_devices, MAX_USB_DEVICES);
        #endif
        num_devices = MAX_USB_DEVICES;
    }

    #if DEBUG_SERIAL
    Serial.printf("[USB] Found %d USB device(s)\n", num_devices);
    #endif

    // Fix #7: Use fixed-size array instead of VLA
    uint8_t dev_addr_list[MAX_USB_DEVICES];
    int num_devices_ret = 0;
    err = usb_host_device_addr_list_fill(num_devices, dev_addr_list, &num_devices_ret);
    if (err != ESP_OK) {
        #if DEBUG_SERIAL
        Serial.println("[USB] Failed to get device addresses");
        #endif
        return false;
    }

    // Try to open each device and check if it's a DYMO printer
    for (int i = 0; i < num_devices; i++) {
        usb_device_handle_t dev_hdl;
        err = usb_host_device_open(_usbClientHandle, dev_addr_list[i], &dev_hdl);
        if (err != ESP_OK) {
            continue;
        }

        // Get device descriptor
        const usb_device_desc_t *dev_desc;
        err = usb_host_get_device_descriptor(dev_hdl, &dev_desc);
        if (err != ESP_OK) {
            usb_host_device_close(_usbClientHandle, dev_hdl);
            continue;
        }

        #if DEBUG_SERIAL
        Serial.printf("[USB] Device %d: VID=0x%04X, PID=0x%04X\n",
                     i, dev_desc->idVendor, dev_desc->idProduct);
        #endif

        // Check if it's a DYMO printer
        if (dev_desc->idVendor == DYMO_VENDOR_ID &&
            dev_desc->idProduct == DYMO_PRODUCT_ID) {

            #if DEBUG_SERIAL
            Serial.println("[USB] DYMO LabelManager PnP found!");
            #endif

            // Fix #6: Reset endpoints before parsing
            _usbOutEndpoint = 0;
            _usbInEndpoint = 0;

            // Get configuration descriptor to find endpoints
            const usb_config_desc_t *config_desc;
            err = usb_host_get_active_config_descriptor(dev_hdl, &config_desc);
            if (err == ESP_OK) {
                // Fix #10: Parse configuration with bounds checking
                const usb_standard_desc_t *next_desc = (const usb_standard_desc_t *)config_desc;
                size_t offset = 0;

                while (offset < config_desc->wTotalLength) {
                    next_desc = (const usb_standard_desc_t *)(((uint8_t *)config_desc) + offset);

                    // Fix #10: Validate descriptor length
                    if (next_desc->bLength < 2 || offset + next_desc->bLength > config_desc->wTotalLength) {
                        #if DEBUG_SERIAL
                        Serial.println("[USB] Invalid descriptor length, stopping parse");
                        #endif
                        break;
                    }

                    if (next_desc->bDescriptorType == USB_B_DESCRIPTOR_TYPE_ENDPOINT) {
                        const usb_ep_desc_t *ep_desc = (const usb_ep_desc_t *)next_desc;

                        // Check for bulk endpoints
                        if ((ep_desc->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK) ==
                            USB_BM_ATTRIBUTES_XFER_BULK) {

                            if (ep_desc->bEndpointAddress & 0x80) {
                                // IN endpoint
                                _usbInEndpoint = ep_desc->bEndpointAddress;
                                #if DEBUG_SERIAL
                                Serial.printf("[USB] IN endpoint: 0x%02X\n", _usbInEndpoint);
                                #endif
                            } else {
                                // OUT endpoint
                                _usbOutEndpoint = ep_desc->bEndpointAddress;
                                #if DEBUG_SERIAL
                                Serial.printf("[USB] OUT endpoint: 0x%02X\n", _usbOutEndpoint);
                                #endif
                            }
                        }
                    }

                    offset += next_desc->bLength;
                }
            }

            // Fix #6: Verify endpoints were found before accepting device
            if (_usbOutEndpoint == 0 || _usbInEndpoint == 0) {
                #if DEBUG_SERIAL
                Serial.printf("[USB] Required endpoints not found (OUT=0x%02X, IN=0x%02X)\n",
                             _usbOutEndpoint, _usbInEndpoint);
                #endif
                usb_host_device_close(_usbClientHandle, dev_hdl);
                continue;  // Try next device
            }

            // All checks passed - device is ready
            _usbDevice = dev_hdl;

            // Fix #12: Thread-safe flag write
            if (_stateMutex) {
                xSemaphoreTake(_stateMutex, portMAX_DELAY);
                _connected = true;
                xSemaphoreGive(_stateMutex);
            }

            return true;
        } else {
            usb_host_device_close(_usbClientHandle, dev_hdl);
        }
    }

    #if DEBUG_SERIAL
    Serial.println("[USB] DYMO printer not found");
    #endif
    return false;
}

#endif // USE_ESP_IDF_USB_HOST
