# Dymolabel ESP32

Web-based DYMO label printer controller for ESP32-S3 Super Mini, inspired by [labelle](https://github.com/labelle-org/labelle).

## Features

- 📱 **Web UI**: User-friendly interface for label creation and printing
- 🔌 **REST API**: Programmatic label printing via HTTP endpoints
- 🖨️ **DYMO Support**: Compatible with DYMO LabelManager series printers
- 📝 **Multiple Content Types**: Text, QR codes, barcodes, and images
- 🌐 **WiFi Enabled**: Access from any device on your network

## Hardware Requirements

### Minimum Setup
- **ESP32-S3 Super Mini** development board
- **DYMO Label Printer** (LabelManager PC, 280, 420P, or compatible)
- **USB Host Shield** (MAX3421E) - Recommended for Arduino framework
- **5V 2A Power Supply** (USB-C for ESP32)
- **USB Cable** for DYMO printer

### Alternative USB Connection Methods

1. **USB Host Shield** (Easiest - Recommended)
   - Works with Arduino framework
   - Well-supported libraries
   - ~$10-15 additional cost

2. **Native USB OTG** (Advanced)
   - Requires ESP-IDF framework
   - No additional hardware
   - More complex implementation

See [Hardware Guide](docs/HARDWARE.md) for detailed wiring instructions.

### USB Hub & Switch Compatibility

✅ **Downstream USB Hub** (Multiple printers on one ESP32):
- Connect 2-4 DYMO printers via powered USB hub
- Control multiple printers from single web interface
- Ideal for high-volume or multi-location setups
- See [Multi-Printer Guide](docs/MULTI-PRINTER.md)

✅ **Upstream USB Switch** (Share one printer between devices):
- Manual USB switches (button-operated)
- USB sharing switches
- KVM switches with USB pass-through
- See [USB Switch Guide](docs/USB-SWITCH.md)

⚠️ **Note**: Upstream switching requires exclusive access during printing. Auto-switching USB switches may cause conflicts.

## Quick Start

### 1. Hardware Setup
- Connect USB Host Shield to ESP32-S3 via SPI pins
- Connect DYMO printer to USB Host Shield
- See [Setup Guide](docs/SETUP.md) for detailed wiring

### 2. Software Setup
```bash
# Clone repository
git clone https://github.com/darkfiberiru/Dymolabel-esp32.git
cd Dymolabel-esp32

# Configure WiFi
cp src/config.h.example src/config.h
# Edit src/config.h with your WiFi credentials

# Build and upload
pio run --target upload

# Monitor output
pio device monitor
```

### 3. Access Web Interface
Navigate to:
- `http://dymolabel.local` (mDNS)
- Or use IP address shown in serial monitor

## Documentation

- 📖 [Setup Guide](docs/SETUP.md) - Detailed installation instructions
- 🔧 [Hardware Guide](docs/HARDWARE.md) - Wiring diagrams and USB implementation
- 🖨️ [Multi-Printer Setup](docs/MULTI-PRINTER.md) - Connect multiple printers via USB hub
- 🔄 [USB Switch Guide](docs/USB-SWITCH.md) - Share printer between multiple hosts
- 📡 [API Documentation](docs/API.md) - REST API reference
- 💡 [Examples](docs/EXAMPLES.md) - Code examples and integrations

## API Endpoints

### Print Label
```bash
POST /api/print
Content-Type: application/json

{
  "text": "Hello World",
  "fontSize": 12,
  "align": "center"
}
```

### Print QR Code
```bash
POST /api/print/qr
Content-Type: application/json

{
  "data": "https://example.com",
  "size": 3
}
```

### Get Status
```bash
GET /api/status
```

## Development

Built with:
- PlatformIO
- ESP32 Arduino Framework
- ESPAsyncWebServer
- ArduinoJson

## License

MIT License - See LICENSE file for details
