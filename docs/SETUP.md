# Setup Guide

## Hardware Setup

### Components Required

1. **ESP32-S3 Super Mini** development board
2. **DYMO Label Printer** (compatible models):
   - DYMO LabelManager PC
   - DYMO LabelPoint 350
   - DYMO LabelManager 280
   - DYMO LabelManager 420P
   - DYMO LabelManager Wireless PnP
3. **USB Cable** to connect printer to ESP32
4. **Power Supply** for ESP32 (USB-C)

### ESP32-S3 Super Mini Pinout Overview

Based on the official pinout diagram, the board features:

- **11 GPIO pins**: GPIO1-GPIO21 (various)
- **6 ADC pins**: A0-A5 for analog input
- **I2C support**: Configurable on multiple GPIO pins
- **SPI support**: Configurable pins with SPI interface
- **UART support**: Multiple UART interfaces available
- **PWM support**: 11 PWM-capable pins
- **WS2812 RGB LED**: GPIO48
- **USB-C**: Native USB support (GPIO19/GPIO20)
- **Power**: BATTERY+, BATTERY-, BOOST, 5V, 3V3, GND

### Wiring Options

The ESP32-S3 Super Mini supports multiple connection methods for the DYMO printer:

#### Option 1: USB Host Mode (Recommended)

The ESP32-S3 has native USB OTG support on GPIO19/GPIO20. Connect your DYMO printer:

```
DYMO Printer USB → USB OTG Adapter → ESP32-S3
                                      ├─ GPIO19 (D-)
                                      └─ GPIO20 (D+)
```

**Requirements:**
- USB OTG cable or adapter
- ESP-IDF USB Host library (requires additional configuration)
- 5V power supply for the printer

**Note**: USB Host mode requires ESP-IDF framework. Arduino framework support is limited. See "USB Host Implementation" section below.

#### Option 2: USB-to-Serial Adapter (Alternative)

For simpler implementation, use a USB-to-Serial adapter between the printer and ESP32:

```
DYMO Printer → USB-Serial Adapter → ESP32-S3
                                     ├─ RX (any UART pin)
                                     └─ TX (any UART pin)
```

**Example UART pin assignment:**
- UART RX → GPIO44 (UART0 RX)
- UART TX → GPIO43 (UART0 TX)
- GND → GND

#### Option 3: USB Host Shield (Easiest for Arduino)

Use a MAX3421E USB Host Shield:

```
USB Host Shield → ESP32-S3 (SPI Interface)
├─ MOSI → GPIO11 (SPI MOSI)
├─ MISO → GPIO13 (SPI MISO)
├─ SCK  → GPIO12 (SPI SCK)
├─ CS   → GPIO10 (SPI CS)
└─ GND  → GND
```

**Advantages:**
- Works with Arduino framework
- Well-supported libraries available
- Easier to implement

### USB Host Implementation Notes

**Important**: The current implementation includes placeholder USB communication code. To fully support DYMO printer communication, you'll need to:

1. **For ESP-IDF (Professional)**:
   - Use ESP-IDF framework instead of Arduino
   - Include USB Host library
   - Implement USB device enumeration
   - Handle USB bulk transfers

2. **For Arduino (Easier)**:
   - Use USB Host Shield with MAX3421E chip
   - Install `USB Host Shield Library 2.0`
   - Modify `DymoUSB.cpp` to use the shield

3. **Serial Adapter Method (Simplest)**:
   - Some DYMO printers support serial communication
   - Use standard Arduino Serial library
   - Requires firmware modification for serial protocol

### Pin Recommendations

**Safe GPIO pins for general use** (per ESP32-S3 Super Mini specs):
- GPIO1, GPIO2, GPIO4-GPIO8
- GPIO15-GPIO18
- GPIO33-GPIO48 (avoid GPIO48 if using onboard LED)

**Avoid these pins**:
- GPIO26-GPIO32 (Reserved for flash/PSRAM on some variants)
- GPIO0 (Boot mode selection)
- GPIO46 (Boot mode selection)

**Power Pins**:
- 5V: Output from USB-C (max 500mA without boost)
- 3V3: 3.3V regulated output
- BATTERY+/BATTERY-: For LiPo battery connection
- BOOST: Enable charging boost mode (up to 300mA)

## Software Setup

### 1. Install PlatformIO

Install [PlatformIO](https://platformio.org/) for your development environment:

- **VS Code**: Install the PlatformIO IDE extension
- **CLI**: `pip install platformio`

### 2. Clone Repository

```bash
git clone https://github.com/yourusername/Dymolabel-esp32.git
cd Dymolabel-esp32
```

### 3. Configure WiFi

Copy the example config and edit with your WiFi credentials:

```bash
cp src/config.h.example src/config.h
```

Edit `src/config.h`:

```cpp
#define WIFI_SSID "YourWiFiNetwork"
#define WIFI_PASSWORD "YourWiFiPassword"
```

### 4. Build and Upload

```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

### 5. Access Web Interface

Once uploaded, the ESP32 will:

1. Connect to your WiFi network
2. Display its IP address in the serial monitor
3. Start the web server

Access the web interface at:

- `http://dymolabel.local` (if mDNS is supported on your network)
- `http://[IP_ADDRESS]` (use the IP shown in serial monitor)

## Configuration Options

### Label Settings

Edit `src/config.h` to adjust label dimensions:

```cpp
#define DYMO_LABEL_WIDTH 64   // Width in pixels (12mm = 64px)
#define DYMO_MAX_LENGTH 1024  // Max length in pixels
```

Common tape widths:

- 6mm tape = 32 pixels
- 9mm tape = 48 pixels
- 12mm tape = 64 pixels
- 19mm tape = 102 pixels

### Network Settings

```cpp
#define HOSTNAME "dymolabel"      // mDNS hostname
#define WEB_SERVER_PORT 80        // Web server port
```

### Debug Mode

Enable detailed serial logging:

```cpp
#define DEBUG_SERIAL true
```

## Troubleshooting

### WiFi Connection Issues

- Verify SSID and password in `config.h`
- Check 2.4GHz WiFi is enabled (ESP32 doesn't support 5GHz)
- Ensure WiFi network allows new device connections

### Printer Not Detected

- Check USB connections
- Verify printer is powered on
- Try different USB cable
- Check serial monitor for error messages

### Web Interface Not Loading

- Confirm ESP32 is connected to WiFi (check serial monitor)
- Try accessing via IP address instead of hostname
- Clear browser cache
- Check firewall settings

### Build Errors

```bash
# Clean build
pio run --target clean

# Update dependencies
pio pkg update

# Rebuild
pio run
```

## Next Steps

- Read the [API Documentation](API.md) for REST API usage
- Check [Examples](EXAMPLES.md) for code samples
- See [Contributing](../CONTRIBUTING.md) to help improve the project
