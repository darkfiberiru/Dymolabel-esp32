# Dymolabel ESP32

Web-based DYMO label printer controller for ESP32-S3 Super Mini, inspired by [labelle](https://github.com/labelle-org/labelle).

## Features

- 📱 **Web UI**: User-friendly interface for label creation and printing
- 🔌 **REST API**: Programmatic label printing via HTTP endpoints
- 🖨️ **DYMO Support**: Compatible with DYMO LabelManager series printers
- 📝 **Multiple Content Types**: Text, QR codes, barcodes, and images
- 🌐 **WiFi Enabled**: Access from any device on your network

## Hardware Requirements

- ESP32-S3 Super Mini
- DYMO Label Printer (LabelManager PC, 280, 420P, or compatible)
- USB connection between ESP32 and printer

## Quick Start

1. **Configure WiFi**: Edit `src/config.h` with your WiFi credentials
2. **Upload Firmware**: Use PlatformIO to build and upload
3. **Access Web UI**: Navigate to `http://dymolabel.local` or the IP shown in serial monitor

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
