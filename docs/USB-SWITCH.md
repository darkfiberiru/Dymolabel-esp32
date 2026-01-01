# USB Switch Compatibility Guide (Upstream Switching)

## Overview

This guide covers **UPSTREAM USB switching** - sharing a single DYMO printer between multiple hosts (computer, ESP32, etc.).

```
Multiple Hosts → [USB Switch] → Single DYMO Printer
├─ Computer
└─ ESP32-S3
```

**Looking for something else?**
- **Multiple printers on one ESP32?** See [Multi-Printer Guide](MULTI-PRINTER.md)
- **Downstream USB hub?** See [Multi-Printer Guide](MULTI-PRINTER.md)

---

The Dymolabel ESP32 system can work with DYMO printers connected through USB switches, allowing you to share a single printer between multiple devices (computer, ESP32, etc.).

## Compatible USB Switch Types

### ✅ Fully Compatible

#### 1. Manual USB Switches
**Examples:**
- UGREEN USB 3.0 Sharing Switch
- ABLEWE USB Switch Selector
- Sabrent 4-Port USB 2.0 Sharing Switch

**How it works:**
- Press a button to switch between hosts
- No automatic switching
- ESP32 has exclusive access when selected

**Pros:**
- ✅ Simple and reliable
- ✅ No conflicts
- ✅ Inexpensive ($10-20)
- ✅ Works with any USB device

**Cons:**
- ❌ Manual operation required
- ❌ Cannot auto-switch based on activity

**Setup:**
```
Port 1: Computer (for Labelle software)
Port 2: ESP32-S3 (for web/API access)
Shared: DYMO Printer

Press button to switch between Port 1 and Port 2
```

---

#### 2. USB Sharing Switches (Software-Controlled)

**Examples:**
- ATEN US224 2-Port USB Switch
- IOGear GUS404 4-Port USB Switch
- Plugable USB 2.0 Switch

**How it works:**
- Remote control or software switching
- Can be automated via scripts
- Supports multiple simultaneous hosts (device-dependent)

**Pros:**
- ✅ Remote control capability
- ✅ Can automate switching
- ✅ Some support concurrent access

**Cons:**
- ❌ More expensive ($30-60)
- ❌ May require driver software
- ❌ Potential timing issues

---

#### 3. Powered USB Hubs with Per-Port Control

**Examples:**
- Plugable 7-Port USB 3.0 Hub with Per-Port Power
- Anker 10-Port USB Hub with Switches

**How it works:**
- Each port has individual power switch
- Can enable/disable specific ports
- ESP32 and computer can both be connected

**Pros:**
- ✅ Independent port control
- ✅ No switching latency
- ✅ Supports multiple devices
- ✅ Useful for other projects

**Cons:**
- ❌ Larger footprint
- ❌ More expensive
- ❌ Requires manual port management

---

### ⚠️ Partially Compatible

#### 4. Auto-Switching USB Switches

**Examples:**
- Generic auto-detect USB switches
- Some KVM switches with auto-selection

**How it works:**
- Automatically switches based on host activity
- May switch away from ESP32 if computer accesses printer

**Issues:**
- ⚠️ May interrupt ESP32 print jobs
- ⚠️ Unpredictable switching behavior
- ⚠️ Requires careful configuration

**Recommendation:** Use manual mode if available

---

### ❌ Not Recommended

#### 5. USB-C Switches (Without USB-A Adapters)

The ESP32-S3 Super Mini uses USB-C for power/programming, but the USB Host Shield expects USB-A. Mixing USB-C switches with USB-A devices requires adapters and may cause issues.

#### 6. USB Ethernet Adapters / Network USB

While interesting, USB-over-network solutions add significant latency and complexity. Not recommended for real-time printer control.

---

## Recommended Setup Configurations

### Configuration 1: Simple Manual Switch

**Use Case:** Home/office with occasional label printing

```
┌─────────────┐
│  Computer   │
└──────┬──────┘
       │
    ┌──▼────────────┐      ┌──────────────┐
    │  USB Switch   │◄─────┤ DYMO Printer │
    │  (Manual)     │      └──────────────┘
    └──┬────────────┘
       │
┌──────▼─────────┐
│   ESP32-S3     │
│  + USB Host    │
│    Shield      │
└────────────────┘

Press button to switch between devices
```

**Shopping List:**
- UGREEN USB 3.0 Switch ($15)
- USB cables (USB-A to printer connector)

---

### Configuration 2: Smart Switching with Automation

**Use Case:** Automated printing integrated with home automation

```
┌─────────────┐      ┌─────────────┐
│  Computer   │      │ Raspberry   │
└──────┬──────┘      │     Pi      │
       │             └──────┬──────┘
       │                    │
    ┌──▼────────────────────▼──┐      ┌──────────────┐
    │  Smart USB Switch        │◄─────┤ DYMO Printer │
    │  (Software Controlled)   │      └──────────────┘
    └──┬───────────────────────┘
       │
┌──────▼─────────┐
│   ESP32-S3     │
│  + USB Host    │
│    Shield      │
└────────────────┘

Switch controlled via network commands
```

**Shopping List:**
- ATEN US224 USB Switch ($35)
- Network control software/scripts

---

### Configuration 3: Hub with Port Control (Recommended for Multiple Devices)

**Use Case:** Makerspace, lab, or office with multiple USB devices

```
                    ┌────────────────────┐
                    │   7-Port USB Hub   │
                    │  (Per-Port Power)  │
                    └─────────┬──────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
   ┌────▼─────┐         ┌────▼─────┐         ┌────▼─────┐
   │  DYMO    │         │  Other   │         │  Other   │
   │ Printer  │         │  Device  │         │  Device  │
   └────┬─────┘         └──────────┘         └──────────┘
        │
   ┌────▼──────────┐     ┌──────────┐
   │   ESP32-S3    │     │ Computer │
   │ + USB Shield  │     └──────────┘
   └───────────────┘

Enable/disable ports as needed
```

**Shopping List:**
- Plugable 7-Port Hub ($40)
- USB cables

---

## Software Implementation

### Detecting Printer Availability

Update `DymoUSB.cpp` to handle printer disconnection/reconnection:

```cpp
bool DymoUSB::begin() {
    // Try to connect to printer
    if (!detectPrinter()) {
        Serial.println("[DYMO] Printer not detected");
        _connected = false;
        return false;
    }

    _connected = true;
    return true;
}

bool DymoUSB::detectPrinter() {
    // Attempt USB enumeration
    // Returns true if DYMO printer found
    // Implementation depends on USB Host Shield library
}

// Call this periodically to check if printer is still connected
void DymoUSB::update() {
    if (_connected && !detectPrinter()) {
        Serial.println("[DYMO] Printer disconnected");
        _connected = false;
    } else if (!_connected && detectPrinter()) {
        Serial.println("[DYMO] Printer reconnected");
        _connected = true;
    }
}
```

### Handling Print Failures

```cpp
bool DymoUSB::printLabel(const uint8_t* imageData, int width, int height) {
    // Check printer before printing
    if (!_connected || !detectPrinter()) {
        return false;
    }

    // Attempt to print
    bool success = sendImage(imageData, width, height);

    // If failed, check if printer disconnected
    if (!success && !detectPrinter()) {
        _connected = false;
        Serial.println("[DYMO] Print failed: Printer disconnected");
    }

    return success;
}
```

### REST API Status Endpoint

The `/api/status` endpoint should reflect switch state:

```json
{
  "status": "disconnected",
  "ready": false,
  "printing": false,
  "message": "Printer not connected. Check USB switch position."
}
```

---

## Best Practices

### 1. **Document Switch Position**
Add labels to USB switch ports:
- Port 1: "Computer"
- Port 2: "ESP32"
- LED indicator shows active port

### 2. **Implement Retry Logic**

```python
import requests
import time

def print_with_retry(text, max_retries=3):
    for attempt in range(max_retries):
        try:
            response = requests.post(
                'http://dymolabel.local/api/print',
                json={'text': text},
                timeout=5
            )

            if response.json()['success']:
                return True

            print(f"Attempt {attempt + 1} failed, retrying...")
            time.sleep(2)

        except Exception as e:
            print(f"Error: {e}")
            time.sleep(2)

    return False
```

### 3. **Check Status Before Printing**

```python
def safe_print(text):
    # Check printer status
    status = requests.get('http://dymolabel.local/api/status').json()

    if status['status'] != 'ready':
        print("Printer not ready. Please check USB switch.")
        return False

    # Print label
    return requests.post(
        'http://dymolabel.local/api/print',
        json={'text': text}
    ).json()['success']
```

### 4. **Add Visual Indicators**

Consider adding an LED to the ESP32 to show printer status:
- Green: Printer connected and ready
- Yellow: Printing in progress
- Red: Printer disconnected (wrong switch position)

Use the onboard WS2812 RGB LED (GPIO48):

```cpp
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel led(1, 48, NEO_GRB + NEO_KHZ800);

void updateStatusLED() {
    if (!printer.isReady()) {
        led.setPixelColor(0, led.Color(255, 0, 0)); // Red
    } else if (printer.isPrinting()) {
        led.setPixelColor(0, led.Color(255, 255, 0)); // Yellow
    } else {
        led.setPixelColor(0, led.Color(0, 255, 0)); // Green
    }
    led.show();
}
```

---

## Troubleshooting

### Printer Not Detected After Switching

**Cause:** USB Host needs time to re-enumerate device

**Solution:**
1. Wait 2-3 seconds after switching
2. Check serial monitor for enumeration messages
3. Power cycle ESP32 if needed

### Print Jobs Fail Mid-Print

**Cause:** USB switch changed during printing

**Solution:**
- Use manual switches only
- Disable auto-switching
- Add "busy" indicator during prints

### Intermittent Connection Issues

**Cause:** Poor USB cable or switch quality

**Solution:**
- Use shorter, high-quality USB cables
- Avoid USB 3.0 switches if possible (USB 2.0 more reliable)
- Consider powered hub instead of passive switch

---

## Product Recommendations

**Budget Setup ($15-20):**
- UGREEN USB 3.0 Sharing Switch (2-Port)
- Works great for simple switching

**Mid-Range Setup ($30-40):**
- ATEN US224 USB 2.0 Switch
- Software control available
- Very reliable

**Premium Setup ($40-60):**
- Plugable 7-Port USB Hub with switches
- Future-proof for multiple devices
- Individual port control

---

## Alternative: No Switch Required

If you dedicate the DYMO printer to ESP32 control only, you don't need a USB switch at all! The web UI and REST API provide full printing capabilities, potentially replacing the need for computer software like DYMO Label.

**Benefits:**
- No switch complexity
- More reliable
- Simpler setup
- Access from any device via web browser

**Trade-off:**
- Can't use DYMO's official software
- Web UI must have all needed features
