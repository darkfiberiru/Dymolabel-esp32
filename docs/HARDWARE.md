# Hardware Implementation Guide

## USB Host Setup for DYMO Printer

This guide covers the hardware implementation details for connecting a DYMO printer to the ESP32-S3 Super Mini.

## ESP32-S3 Native USB OTG

**Good News!** The ESP32-S3 Super Mini has **built-in USB OTG** capability on GPIO19/GPIO20. This means you can connect USB devices directly without needing an external USB Host Shield!

## Implementation Options

### Option 1: Native USB OTG (Recommended - No Extra Hardware!)

**This is the best option for ESP32-S3 Super Mini!**

**Hardware Required:**
- USB-C to USB-A OTG adapter (or cable)
- Standard USB cable for DYMO printer
- External 5V power supply for DYMO printer

**Wiring:**

The ESP32-S3 has USB OTG on its **USB-C port**:

```
DYMO Printer → USB Cable → USB-C OTG Adapter → ESP32-S3 USB-C Port
                                                (GPIO19/D-, GPIO20/D+)
```

**Important Power Considerations:**
- ESP32-S3 USB can provide ~100mA
- DYMO printers need 200-500mA when printing
- **Solution**: Use a powered USB hub between ESP32-S3 and printer, OR
- **Solution**: Power printer separately via Y-cable with external 5V supply

**Software Requirements:**
- ESP-IDF framework with USB Host library
- More complex than Arduino but native support

**Advantages:**
✅ **No external USB Host Shield needed!**
✅ Uses built-in ESP32-S3 hardware
✅ One less component to buy
✅ Smaller footprint
✅ Potentially faster

**Disadvantages:**
❌ Requires ESP-IDF framework (more complex than Arduino)
❌ Power supply needs careful consideration
❌ Fewer Arduino library examples

---

### Option 2: USB Host Shield (Arduino-Friendly Alternative)

**Hardware Required:**
- MAX3421E USB Host Shield
- Female USB-A connector (usually on shield)
- Standard USB cable for DYMO printer

**Wiring:**

```
MAX3421E Shield    →    ESP32-S3 Super Mini
─────────────────────────────────────────────
VCC                →    3V3
GND                →    GND
MOSI               →    GPIO11 (SPI MOSI)
MISO               →    GPIO13 (SPI MISO)
SCK                →    GPIO12 (SPI SCK)
CS/SS              →    GPIO10 (SPI CS)
INT                →    GPIO9  (Interrupt, optional)
RESET              →    GPIO8  (Reset, optional)
```

**Software Setup:**

```ini
# Add to platformio.ini
lib_deps =
    felis/USB Host Shield Library 2.0@^1.6.3
    me-no-dev/ESPAsyncWebServer@^1.2.3
    me-no-dev/AsyncTCP@^1.1.1
    bblanchon/ArduinoJson@^7.2.1
```

**Advantages:**
✅ Works with Arduino framework
✅ Well-supported library ecosystem
✅ Easier to debug and implement
✅ Proven solution for USB devices

**Disadvantages:**
❌ Requires additional hardware (~$10-15)
❌ Takes up SPI pins
❌ Adds to overall size

---

### Option 3: USB-to-Serial Bridge (Not Recommended)

**Hardware Required:**
- CP2102, FT232, or CH340 USB-to-Serial adapter
- Modified DYMO printer or serial-capable model

**Note**: Most DYMO printers do NOT support serial communication. This option only works if you have a vintage model with serial support or custom firmware.

---

## Recommended Implementation

**For ESP32-S3 Super Mini:** Use **Option 1 (Native USB OTG)** because:

1. ✅ No additional hardware needed (just a USB-C OTG adapter)
2. ✅ Uses built-in ESP32-S3 USB capability
3. ✅ Smallest footprint
4. ✅ Lower cost (~$2 for OTG adapter vs $10-15 for shield)

**Alternative:** If you prefer Arduino framework and want easier setup, use **Option 2 (USB Host Shield)**

### Shopping List

**USB Host Shield Options:**
- Sparkfun USB Host Shield (DEV-09628)
- Generic MAX3421E shield from AliExpress/Amazon
- Adafruit USB Host Shield

**Power Supply:**
- 5V 1A USB power supply (for ESP32)
- Or use battery pack (3.7V LiPo with onboard boost)

### Example Shield Wiring

```
  ┌─────────────────┐
  │   ESP32-S3      │
  │   Super Mini    │
  │                 │
  │  GPIO11 ────────┼──── MOSI
  │  GPIO13 ────────┼──── MISO         ┌──────────────┐
  │  GPIO12 ────────┼──── SCK    ──────┤ MAX3421E     │
  │  GPIO10 ────────┼──── CS           │ USB Host     │
  │   3V3   ────────┼──── VCC          │ Shield       │
  │   GND   ────────┼──── GND          │              │
  └─────────────────┘                  │   USB Port   │
                                       └──────┬───────┘
                                              │
                                         ┌────┴────┐
                                         │  DYMO   │
                                         │ Printer │
                                         └─────────┘
```

## Code Modifications Required

### Update platformio.ini

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

build_flags =
    -DCORE_DEBUG_LEVEL=3
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DUSE_USB_HOST_SHIELD=1

lib_deps =
    felis/USB Host Shield Library 2.0@^1.6.3
    me-no-dev/ESPAsyncWebServer@^1.2.3
    me-no-dev/AsyncTCP@^1.1.1
    bblanchon/ArduinoJson@^7.2.1
```

### Update DymoUSB.cpp

You'll need to modify the `DymoUSB` class to use the USB Host Shield library. Here's a basic outline:

```cpp
#include <usbhub.h>
#include <hiduniversal.h>

class DymoUSB {
private:
    USB usb;
    USBHub hub;
    HIDBoot<USB_HID_PROTOCOL_KEYBOARD> hid;

    // USB Host initialization
    bool initUSBHost();

    // Send data via USB
    bool sendUSB(const uint8_t* data, size_t len);
};
```

## Testing Without Printer

During development, you can test the web interface and API without a connected printer:

1. The code includes simulation mode
2. All print commands will log to serial monitor
3. Status will show "ready" even without printer
4. Enable `DEBUG_SERIAL` to see simulated output

## Power Considerations

**ESP32-S3 Power Draw:**
- Idle: ~40mA
- WiFi active: ~160mA
- Peak: ~240mA

**DYMO Printer Power Draw:**
- Idle: ~50mA
- Printing: ~300-500mA
- Peak (motor): up to 800mA

**Total System:**
- Use 5V 2A power supply minimum
- If battery powered, use 2000mAh+ LiPo

## Next Steps

1. Choose your implementation option
2. Order necessary hardware
3. Update firmware code for your chosen method
4. Test with printer connected
5. Calibrate label dimensions for your tape size

## Resources

- [ESP32-S3 USB Host Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/usb_host.html)
- [USB Host Shield Library](https://github.com/felis/USB_Host_Shield_2.0)
- [DYMO SDK Documentation](https://developers.dymo.com/)
- [ESP32-S3 Super Mini Pinout](https://www.espboards.dev/esp32/esp32-s3-super-mini/)
