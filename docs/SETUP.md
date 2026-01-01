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

### Wiring

The ESP32-S3 Super Mini has native USB support. Connect your DYMO printer to the ESP32 using:

- USB Data+ (D+) → GPIO19
- USB Data- (D-) → GPIO20
- USB VCC → 5V
- USB GND → GND

> **Note**: Some DYMO printers may require a USB Host Shield or USB-OTG adapter.

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
