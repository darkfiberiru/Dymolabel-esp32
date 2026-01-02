# Critical Bug Fixes - Implementation Guide

## Fix #1: USB Transfer Synchronization (CRITICAL)

**Problem:** Transfers freed before completion, causing use-after-free

**Solution:** Use callback with semaphore for proper synchronization

```cpp
// Add to DymoUSB.h (private section):
#ifdef USE_ESP_IDF_USB_HOST
    SemaphoreHandle_t _transferCompleteSem;
    static void usbTransferCallback(usb_transfer_t *transfer);
#endif

// Add to DymoUSB.cpp constructor:
#ifdef USE_ESP_IDF_USB_HOST
    _transferCompleteSem = xSemaphoreCreateBinary();
#endif

// Add callback implementation:
void DymoUSB::usbTransferCallback(usb_transfer_t *transfer) {
    DymoUSB* instance = (DymoUSB*)transfer->context;
    if (instance && instance->_transferCompleteSem) {
        xSemaphoreGive(instance->_transferCompleteSem);
    }
}

// Fix sendCommand():
bool DymoUSB::sendCommand(const uint8_t* data, size_t length) {
#ifdef USE_ESP_IDF_USB_HOST
    if (!_connected || !_usbDevice || _usbOutEndpoint == 0) {
        return false;
    }

    // Allocate USB transfer
    usb_transfer_t *transfer;
    esp_err_t err = usb_host_transfer_alloc(length, 0, &transfer);
    if (err != ESP_OK) {
        return false;
    }

    // Setup transfer with callback
    transfer->device_handle = _usbDevice;
    transfer->bEndpointAddress = _usbOutEndpoint;
    transfer->callback = usbTransferCallback;  // ✅ Use callback
    transfer->context = this;                   // ✅ Pass instance
    transfer->num_bytes = length;
    memcpy(transfer->data_buffer, data, length);
    transfer->timeout_ms = 1000;

    // Submit transfer
    err = usb_host_transfer_submit(transfer);
    if (err != ESP_OK) {
        usb_host_transfer_free(transfer);
        return false;
    }

    // ✅ Wait for completion properly
    bool success = (xSemaphoreTake(_transferCompleteSem, pdMS_TO_TICKS(2000)) == pdTRUE);

    // ✅ Check transfer status
    if (!success || transfer->status != USB_TRANSFER_STATUS_COMPLETED) {
        Serial.printf("[DYMO] Transfer failed: status=%d\n", transfer->status);
        usb_host_transfer_free(transfer);
        return false;
    }

    usb_host_transfer_free(transfer);
    return true;
#else
    delay(1);
    return true;
#endif
}
```

---

## Fix #2: USB Client Handle Storage (CRITICAL)

**Problem:** Client handle lost, cannot cleanup

**Solution:** Store handle as class member

```cpp
// Add to DymoUSB.h (private section):
#ifdef USE_ESP_IDF_USB_HOST
    usb_host_client_handle_t _usbClientHandle;
#endif

// Update DymoUSB.cpp constructor:
#ifdef USE_ESP_IDF_USB_HOST
    _usbClientHandle = NULL;
#endif

// Fix initUSBHost():
bool DymoUSB::initUSBHost() {
    // ... existing code ...

    // ✅ Store the client handle
    err = usb_host_client_register(&client_config, &_usbClientHandle);
    if (err != ESP_OK) {
        Serial.printf("[USB] Client register failed: %s\n", esp_err_to_name(err));
        s_usb_host_lib_task_running = false;
        usb_host_uninstall();
        vSemaphoreDelete(s_usb_host_ready_sem);
        s_usb_host_ready_sem = NULL;
        return false;
    }

    _usbHostInitialized = true;
    return true;
}

// Add destructor to DymoUSB.h:
~DymoUSB();

// Implement destructor in DymoUSB.cpp:
DymoUSB::~DymoUSB() {
#ifdef USE_ESP_IDF_USB_HOST
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
    s_usb_host_lib_task_running = false;
    delay(100);  // Give task time to exit

    // Delete semaphore
    if (s_usb_host_ready_sem) {
        vSemaphoreDelete(s_usb_host_ready_sem);
        s_usb_host_ready_sem = NULL;
    }

    if (_transferCompleteSem) {
        vSemaphoreDelete(_transferCompleteSem);
        _transferCompleteSem = NULL;
    }

    // Uninstall USB Host
    usb_host_uninstall();
#endif
}
```

---

## Fix #3: Static Variable Race Conditions (CRITICAL)

**Problem:** Multiple instances share statics

**Solution:** Move to instance members OR use singleton pattern

### Option A: Instance Members (Recommended if single printer)

```cpp
// Remove from DymoUSB.cpp:
// static bool s_usb_host_lib_task_running = false;
// static SemaphoreHandle_t s_usb_host_ready_sem = NULL;

// Add to DymoUSB.h (private):
#ifdef USE_ESP_IDF_USB_HOST
    bool _usbHostLibTaskRunning;
    SemaphoreHandle_t _usbHostReadySem;
    TaskHandle_t _usbHostTaskHandle;
#endif

// Update all references to use instance members:
_usbHostLibTaskRunning instead of s_usb_host_lib_task_running
_usbHostReadySem instead of s_usb_host_ready_sem
```

### Option B: Singleton Pattern (If multi-printer needed)

```cpp
// Create USB Host manager singleton
class USBHostManager {
private:
    static USBHostManager* instance;
    bool taskRunning;
    SemaphoreHandle_t deviceReadySem;
    usb_host_client_handle_t clientHandle;
    int refCount;

public:
    static USBHostManager* getInstance() {
        if (!instance) {
            instance = new USBHostManager();
        }
        return instance;
    }

    bool init() {
        if (refCount == 0) {
            // Initialize USB Host
        }
        refCount++;
        return true;
    }

    void deinit() {
        refCount--;
        if (refCount == 0) {
            // Cleanup USB Host
        }
    }
};
```

---

## Fix #4: Division By Zero (CRITICAL)

**Problem:** Crashes if imageWidth < 8

**Solution:** Add validation

```cpp
bool DymoUSB::printText(const String& text, int fontSize, const String& align) {
    // Convert text to bitmap image
    std::vector<uint8_t> imageData = textToImage(text, fontSize, align);

    if (imageData.empty()) {
        return false;
    }

    // ✅ Validate dimensions BEFORE division
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

// Apply same fix to printQRCode() and printBarcode()
```

---

## Fix #5: MonoBitmap Buffer Overflow (CRITICAL)

**Problem:** Buffer too small if width not divisible by 8

**Solution:** Round up buffer size

```cpp
class MonoBitmap : public Adafruit_GFX {
public:
    MonoBitmap(uint16_t w, uint16_t h) : Adafruit_GFX(w, h) {
        _width = w;
        _height = h;
        // ✅ Round up to nearest byte boundary
        size_t bytesPerRow = (w + 7) / 8;  // Ceiling division
        _buffer.resize(bytesPerRow * h, 0);
        #if DEBUG_SERIAL
        Serial.printf("[MonoBitmap] Created: %dx%d (%d bytes)\n", w, h, _buffer.size());
        #endif
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        if (x < 0 || x >= _width || y < 0 || y >= _height) return;

        size_t bytesPerRow = (_width + 7) / 8;
        int byteIndex = y * bytesPerRow + (x / 8);
        int bitIndex = 7 - (x % 8);

        // ✅ Add bounds check
        if (byteIndex >= _buffer.size()) {
            #if DEBUG_SERIAL
            Serial.printf("[MonoBitmap] ERROR: Out of bounds: x=%d, y=%d, idx=%d\n",
                         x, y, byteIndex);
            #endif
            return;
        }

        if (color) {
            _buffer[byteIndex] |= (1 << bitIndex);
        } else {
            _buffer[byteIndex] &= ~(1 << bitIndex);
        }
    }

    // ... rest of class
};
```

---

## Fix #6: USB Device Cleanup on Failure (HIGH)

**Problem:** Device left open if endpoints not found

**Solution:** Validate endpoints before returning success

```cpp
bool DymoUSB::detectAndOpenPrinter() {
    // ... existing code to open device ...

    // Check if it's a DYMO printer
    if (dev_desc->idVendor == DYMO_VENDOR_ID &&
        dev_desc->idProduct == DYMO_PRODUCT_ID) {

        Serial.println("[USB] DYMO LabelManager PnP found!");

        // ✅ Reset endpoints before parsing
        _usbOutEndpoint = 0;
        _usbInEndpoint = 0;

        // Get configuration descriptor to find endpoints
        const usb_config_desc_t *config_desc;
        err = usb_host_get_active_config_descriptor(dev_hdl, &config_desc);
        if (err == ESP_OK) {
            // ... existing endpoint parsing code ...
        }

        // ✅ VERIFY endpoints were found
        if (_usbOutEndpoint == 0 || _usbInEndpoint == 0) {
            #if DEBUG_SERIAL
            Serial.printf("[USB] Required endpoints not found (OUT=0x%02X, IN=0x%02X)\n",
                         _usbOutEndpoint, _usbInEndpoint);
            #endif
            usb_host_device_close(_usbClientHandle, dev_hdl);
            continue;  // Try next device
        }

        // ✅ Only set these if endpoints are valid
        _usbDevice = dev_hdl;
        _connected = true;
        return true;
    } else {
        usb_host_device_close(_usbClientHandle, dev_hdl);
    }

    // ... rest of function
}
```

---

## Fix #7: Stack Overflow Risk (HIGH)

**Problem:** VLA could overflow stack

**Solution:** Use fixed-size array with bounds check

```cpp
bool DymoUSB::detectAndOpenPrinter() {
    // ... existing code ...

    // Get list of devices
    uint8_t num_devices;
    esp_err_t err = usb_host_device_addr_list_fill(0, &num_devices);  // ✅ Get count first
    if (err != ESP_OK || num_devices == 0) {
        Serial.println("[USB] No USB devices found");
        return false;
    }

    // ✅ Limit to reasonable maximum
    #define MAX_USB_DEVICES 16
    if (num_devices > MAX_USB_DEVICES) {
        Serial.printf("[USB] Too many devices: %d (max %d)\n", num_devices, MAX_USB_DEVICES);
        num_devices = MAX_USB_DEVICES;  // Truncate
    }

    Serial.printf("[USB] Found %d USB device(s)\n", num_devices);

    // ✅ Fixed-size array instead of VLA
    uint8_t dev_addr_list[MAX_USB_DEVICES];
    err = usb_host_device_addr_list_fill(num_devices, dev_addr_list);
    if (err != ESP_OK) {
        Serial.println("[USB] Failed to get device addresses");
        return false;
    }

    // ... rest of function ...
}
```

---

## Testing Checklist After Fixes

- [ ] Compile with no warnings
- [ ] Test with valid DYMO printer
- [ ] Test with invalid/no printer (timeout handling)
- [ ] Test multiple print jobs rapidly (race conditions)
- [ ] Test with unusual tape widths (0mm, 255mm)
- [ ] Test disconnect/reconnect during printing
- [ ] Verify no memory leaks (monitor free heap)
- [ ] Test destructor (create/destroy DymoUSB instances)
- [ ] Valgrind/AddressSanitizer if available

---

## Additional Recommendations

1. **Add Unit Tests** for:
   - Tape calculations
   - Image dimension validation
   - Buffer size calculations

2. **Add Watchdog Timer** in USB task to detect hangs

3. **Add Heap Monitoring**:
   ```cpp
   Serial.printf("[HEAP] Free: %d, Min: %d\n",
                 ESP.getFreeHeap(), ESP.getMinFreeHeap());
   ```

4. **Enable Compiler Warnings**:
   ```ini
   build_flags =
       -Wall
       -Wextra
       -Werror
       -Wno-unused-parameter
   ```

5. **Add Runtime Assertions**:
   ```cpp
   #define ASSERT(condition, msg) if (!(condition)) { \
       Serial.printf("[ASSERT] %s:%d: %s\n", __FILE__, __LINE__, msg); \
       while(1); \
   }
   ```
