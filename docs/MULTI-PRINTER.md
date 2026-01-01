# Multiple Printer Support (Downstream USB Hub)

## Overview

You can connect **multiple DYMO printers** to a single ESP32-S3 using a USB hub downstream of the USB Host Shield. This allows you to control multiple printers from one web interface.

## Architecture

```
┌─────────────────┐
│   ESP32-S3      │
│  Super Mini     │
└────────┬────────┘
         │ SPI
    ┌────▼────────────┐
    │  USB Host       │
    │  Shield         │
    │  (MAX3421E)     │
    └────────┬────────┘
             │ USB
        ┌────▼──────────┐
        │   USB Hub     │
        │   4-Port      │
        └───┬───┬───┬───┘
            │   │   │
    ┌───────▼┐ ┌▼──────┐ ┌▼────────┐
    │ DYMO   │ │ DYMO  │ │  DYMO   │
    │Printer1│ │Printer│ │ Printer │
    └────────┘ └───────┘ └─────────┘
```

## Hardware Requirements

### Option 1: Powered USB Hub (Recommended)

**Components:**
- ESP32-S3 Super Mini
- MAX3421E USB Host Shield
- **Powered USB 2.0 Hub** (4-7 ports)
- Multiple DYMO printers
- 5V 2A+ power supply for hub

**Why Powered Hub:**
- Each DYMO printer draws 300-500mA when printing
- ESP32 USB Host Shield cannot provide enough power for multiple printers
- Powered hub provides independent power to each printer

**Recommended Hubs:**
- Anker 4-Port USB 3.0 Hub with Power Adapter
- Sabrent 4-Port USB 2.0 Hub (HB-UM43)
- TP-Link UH400 USB 3.0 Hub

**Shopping List:**
- USB Host Shield MAX3421E: $10-15
- Powered USB Hub: $15-25
- DYMO Printers: $50-100 each
- Total: ~$100-150 for complete multi-printer setup

---

### Option 2: USB Hub with Per-Port Power Control

**Components:**
- ESP32-S3 Super Mini
- MAX3421E USB Host Shield
- USB Hub with individual port switches (Plugable, Cambrionix)
- Multiple DYMO printers

**Advantages:**
- Can power on/off individual printers
- Prevents power spikes when multiple printers start
- Better for troubleshooting

**Cost:** $40-60 for hub

---

### Option 3: Simple USB Hub (Limited - Not Recommended)

**Components:**
- ESP32-S3 Super Mini
- MAX3421E USB Host Shield
- Basic unpowered USB hub
- Single DYMO printer (max 1-2 printers)

**Limitations:**
- ⚠️ Power limited to ~500mA total
- ⚠️ Can only support 1-2 printers maximum
- ⚠️ Unreliable with multiple simultaneous prints

---

## Wiring

### ESP32-S3 to USB Host Shield

```
USB Host Shield    →    ESP32-S3 Super Mini
─────────────────────────────────────────────
VCC                →    3V3
GND                →    GND
MOSI               →    GPIO11 (SPI MOSI)
MISO               →    GPIO13 (SPI MISO)
SCK                →    GPIO12 (SPI SCK)
CS                 →    GPIO10 (SPI CS)
INT (optional)     →    GPIO9
```

### USB Host Shield to Hub

```
USB Host Shield → USB Hub → DYMO Printers
      (USB-A)      (4-Port)
                     ├─ Port 1: DYMO LabelManager PC
                     ├─ Port 2: DYMO LabelManager 280
                     ├─ Port 3: DYMO LabelPoint 350
                     └─ Port 4: (Reserved for future)
```

---

## Software Implementation

### Firmware Modifications Required

The current firmware supports a single printer. To support multiple printers, you need to:

#### 1. Detect Multiple Printers

Update `DymoUSB.cpp` to enumerate all connected USB devices:

```cpp
#include <usbhub.h>

class DymoUSB {
private:
    USB usb;
    USBHub hub1;  // Support for USB hub

    struct PrinterDevice {
        uint8_t address;
        String serialNumber;
        bool connected;
        bool busy;
    };

    std::vector<PrinterDevice> printers;

public:
    // Enumerate all connected printers
    bool scanPrinters();

    // Get list of available printers
    std::vector<String> getAvailablePrinters();

    // Print to specific printer by address
    bool printToDevice(uint8_t deviceAddress, const uint8_t* data, size_t len);
};
```

#### 2. Update USB Host Library

Add to `platformio.ini`:

```ini
lib_deps =
    felis/USB Host Shield Library 2.0@^1.6.3
    # USB Hub support is built into USB Host Shield Library
```

#### 3. Web API Updates

Add endpoint to select printer:

```cpp
// New API endpoint: List printers
_server.on("/api/printers", HTTP_GET, [this](AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray printersArray = doc["printers"].to<JsonArray>();

    for (auto& printer : _printer->getAvailablePrinters()) {
        JsonObject p = printersArray.add<JsonObject>();
        p["id"] = printer.address;
        p["name"] = printer.serialNumber;
        p["connected"] = printer.connected;
        p["busy"] = printer.busy;
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
});

// Updated print endpoint: Specify printer
_server.on("/api/print", HTTP_POST, [this](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject jsonObj = json.as<JsonObject>();

    // Optional: specify which printer to use
    uint8_t printerID = jsonObj.containsKey("printer") ?
                        jsonObj["printer"].as<uint8_t>() : 0;

    String text = jsonObj["text"].as<String>();

    if (_printer->printToDevice(printerID, text)) {
        sendJsonResponse(request, 200, "Label printed successfully");
    } else {
        sendJsonResponse(request, 500, "Failed to print label", false);
    }
});
```

---

## Updated Web UI

### Printer Selection Dropdown

Add to the web interface:

```html
<div class="form-group">
    <label for="printer-select">Select Printer</label>
    <select id="printer-select">
        <option value="0">Printer 1 (Default)</option>
        <option value="1">Printer 2</option>
        <option value="3">Printer 3</option>
    </select>
</div>
```

### JavaScript Updates

```javascript
// Fetch available printers on page load
async function loadPrinters() {
    const response = await fetch('/api/printers');
    const data = await response.json();

    const select = document.getElementById('printer-select');
    select.innerHTML = '';

    data.printers.forEach((printer, index) => {
        const option = document.createElement('option');
        option.value = printer.id;
        option.textContent = `Printer ${index + 1} - ${printer.name}`;
        option.disabled = !printer.connected || printer.busy;
        select.appendChild(option);
    });
}

// Include printer selection in print requests
async function printText(event) {
    event.preventDefault();

    const text = document.getElementById('text-content').value;
    const fontSize = parseInt(document.getElementById('text-size').value);
    const printerID = parseInt(document.getElementById('printer-select').value);

    const response = await fetch('/api/print', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            text,
            fontSize,
            printer: printerID
        })
    });

    const result = await response.json();
    showMessage('text-message', result.message, !result.success);
}

// Refresh printer list every 5 seconds
setInterval(loadPrinters, 5000);
loadPrinters();
```

---

## REST API Examples

### List Available Printers

```bash
curl http://dymolabel.local/api/printers
```

**Response:**
```json
{
  "printers": [
    {
      "id": 0,
      "name": "DYMO-PC-001",
      "connected": true,
      "busy": false
    },
    {
      "id": 1,
      "name": "DYMO-280-002",
      "connected": true,
      "busy": false
    },
    {
      "id": 2,
      "name": "DYMO-350-003",
      "connected": false,
      "busy": false
    }
  ]
}
```

### Print to Specific Printer

```bash
curl -X POST http://dymolabel.local/api/print \
  -H "Content-Type: application/json" \
  -d '{
    "text": "Hello from Printer 2",
    "fontSize": 12,
    "printer": 1
  }'
```

### Print to All Printers (Broadcast)

```python
import requests

def print_to_all_printers(text):
    """Print the same label to all connected printers"""

    # Get list of printers
    response = requests.get('http://dymolabel.local/api/printers')
    printers = response.json()['printers']

    # Print to each connected printer
    for printer in printers:
        if printer['connected'] and not printer['busy']:
            requests.post('http://dymolabel.local/api/print', json={
                'text': text,
                'printer': printer['id']
            })
            print(f"Printed to {printer['name']}")

print_to_all_printers("Inventory Check 2026-01-01")
```

---

## Use Cases

### 1. Multi-Location Labeling

Different printers in different locations:
- Printer 1: Shipping desk
- Printer 2: Receiving dock
- Printer 3: Inventory room

API can specify which location to print:

```python
def print_shipping_label(item):
    requests.post('http://dymolabel.local/api/print', json={
        'text': f'Ship: {item}',
        'printer': 0  # Shipping desk
    })

def print_receiving_label(item):
    requests.post('http://dymolabel.local/api/print', json={
        'text': f'Received: {item}',
        'printer': 1  # Receiving dock
    })
```

### 2. Different Label Sizes

Use different printers with different tape widths:
- Printer 1: 12mm tape for small labels
- Printer 2: 19mm tape for large labels

```python
def print_small_label(text):
    requests.post('http://dymolabel.local/api/print', json={
        'text': text,
        'fontSize': 10,
        'printer': 0  # 12mm tape
    })

def print_large_label(text):
    requests.post('http://dymolabel.local/api/print', json={
        'text': text,
        'fontSize': 16,
        'printer': 1  # 19mm tape
    })
```

### 3. Parallel Printing

Print different labels simultaneously:

```python
import concurrent.futures

def print_batch_labels(labels):
    """Print multiple labels in parallel to different printers"""

    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as executor:
        futures = []

        for i, label in enumerate(labels):
            printer_id = i % 3  # Round-robin across 3 printers

            future = executor.submit(
                requests.post,
                'http://dymolabel.local/api/print',
                json={'text': label, 'printer': printer_id}
            )
            futures.append(future)

        # Wait for all prints to complete
        for future in concurrent.futures.as_completed(futures):
            result = future.result()
            print(f"Print job completed: {result.json()}")

# Print 10 labels across 3 printers simultaneously
labels = [f"Item {i+1}" for i in range(10)]
print_batch_labels(labels)
```

---

## Power Considerations

### Power Budget

**Single Printer:**
- Idle: 50mA
- Printing: 300-500mA
- Peak: 800mA

**3 Printers Printing Simultaneously:**
- Total: 900-1500mA
- Peak: up to 2400mA

**Powered USB Hub Required:**
- Minimum: 5V 2A (2000mA)
- Recommended: 5V 3A (3000mA)
- Best: 5V 4A (4000mA) for 4+ printers

### ESP32 Power

ESP32-S3 + USB Host Shield:
- Idle: ~100mA
- WiFi active: ~200mA
- Peak: ~300mA

**Total System:**
- Use separate power supplies:
  - ESP32: 5V 1A via USB-C
  - USB Hub: 5V 3A+ via barrel jack

---

## USB Hub Compatibility

### ✅ Confirmed Compatible Hubs

1. **Anker 4-Port USB 3.0 Hub** (AK-A7516)
   - Works great with USB Host Shield
   - Good power supply (5V 2.5A)
   - $20-25

2. **Sabrent HB-UM43** (4-Port USB 2.0)
   - Budget option
   - Reliable enumeration
   - $15-20

3. **TP-Link UH700** (7-Port USB 3.0)
   - Supports many devices
   - Good power (5V 4A)
   - $30-35

### ⚠️ Avoid

- Unpowered USB hubs (insufficient power)
- USB 3.0 only hubs without USB 2.0 compatibility
- "Smart" hubs with auto-sleep (may disconnect printers)

---

## Troubleshooting

### Hub Not Detected

**Symptoms:** USB Host Shield doesn't detect hub

**Solutions:**
1. Verify hub is USB 2.0 compatible (not USB 3.0 only)
2. Use shorter USB cable between shield and hub (< 1 meter)
3. Check hub power supply is connected
4. Try different hub

### Printers Not Enumerated

**Symptoms:** Hub detected but printers don't show up

**Solutions:**
1. Connect printers one at a time
2. Wait 5 seconds between connections
3. Check serial monitor for enumeration messages
4. Ensure hub provides enough power (use powered hub)

### Random Disconnects

**Symptoms:** Printers randomly disconnect/reconnect

**Solutions:**
1. Use higher-quality USB cables
2. Upgrade to higher-amperage hub power supply
3. Enable USB reset in firmware
4. Add decoupling capacitors to USB Host Shield

---

## Advanced: USB Switch + Hub Combination

You can combine both upstream and downstream switching:

```
Computer ←→ [USB Switch] ←→ ESP32 → [USB Hub] → Multiple Printers
                                          ├─ Printer 1
                                          ├─ Printer 2
                                          └─ Printer 3
```

This allows:
- Computer can control all printers using DYMO software
- ESP32 can control all printers via web/API
- Switch between computer and ESP32 control
- Hub allows multiple printers on whichever host is selected

---

## Future Enhancements

### Automatic Failover

If one printer fails, automatically switch to backup:

```cpp
bool printWithFailover(const uint8_t* data, size_t len) {
    for (auto& printer : printers) {
        if (printer.connected && !printer.busy) {
            if (printToDevice(printer.address, data, len)) {
                return true;
            }
        }
    }
    return false;  // All printers failed
}
```

### Load Balancing

Distribute print jobs across printers:

```cpp
uint8_t selectLeastBusyPrinter() {
    // Track print count per printer
    // Return printer with lowest count
    return printers[indexOfMin(printCounts)].address;
}
```

### Printer Pool Management

```python
class PrinterPool:
    def __init__(self, esp32_url):
        self.url = esp32_url
        self.printer_status = {}

    def update_status(self):
        response = requests.get(f'{self.url}/api/printers')
        self.printer_status = response.json()['printers']

    def get_available_printer(self):
        """Get first available printer"""
        for printer in self.printer_status:
            if printer['connected'] and not printer['busy']:
                return printer['id']
        return None

    def print_label(self, text):
        printer_id = self.get_available_printer()
        if printer_id is not None:
            return requests.post(f'{self.url}/api/print', json={
                'text': text,
                'printer': printer_id
            })
        return None

pool = PrinterPool('http://dymolabel.local')
pool.print_label('Test Label')
```

---

## Summary

**Downstream USB Hub Support:**
✅ Fully supported with USB Host Shield
✅ Allows multiple DYMO printers on single ESP32
✅ Requires powered USB hub for reliability
✅ Firmware modifications needed for multi-printer API
✅ Great for high-volume or multi-location setups

**Next Steps:**
1. Get powered USB hub (4-port recommended)
2. Connect to USB Host Shield
3. Modify firmware for multi-printer enumeration
4. Update web UI with printer selection
5. Test with multiple printers
