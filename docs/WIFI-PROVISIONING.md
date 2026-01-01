# WiFi Provisioning Guide

## Overview

The Dymolabel ESP32 uses **WiFi Provisioning Manager** for easy, secure WiFi setup without hardcoding credentials in the firmware. This allows you to:

- ✅ Configure WiFi via smartphone app (no reflashing needed)
- ✅ Change WiFi networks anytime
- ✅ Secure credential storage in NVS (Non-Volatile Storage)
- ✅ No WiFi passwords in source code
- ✅ Multiple device provisioning without code changes

## Provisioning Methods

The device supports **BLE (Bluetooth Low Energy) provisioning**, which is the easiest and most user-friendly method.

### BLE Provisioning (Recommended)

**Advantages:**
- No need to switch WiFi networks on your phone
- Works from iOS and Android
- Secure encrypted communication
- User-friendly mobile app

**Requirements:**
- Smartphone with Bluetooth
- ESP BLE Provisioning app (free)

---

## First-Time Setup

### Step 1: Install Mobile App

Download the official Espressif provisioning app:

**iOS (iPhone/iPad):**
- Open App Store
- Search for "ESP BLE Provisioning"
- Install the app (free, by Espressif Systems)

**Android:**
- Open Google Play Store
- Search for "ESP BLE Provisioning"
- Install the app (free, by Espressif Systems)

### Step 2: Power On ESP32

1. Connect ESP32-S3 to power via USB-C
2. Open serial monitor (115200 baud) to see status
3. Device will start in provisioning mode if not yet configured

You'll see this in the serial monitor:

```
╔════════════════════════════════════╗
║    Dymolabel ESP32-S3 v1.0.0      ║
╚════════════════════════════════════╝

[SETUP] Starting WiFi provisioning via BLE...
[SETUP] ┌────────────────────────────────────────┐
[SETUP] │  WiFi Provisioning Instructions       │
[SETUP] ├────────────────────────────────────────┤
[SETUP] │ 1. Install 'ESP BLE Provisioning' app │
[SETUP] │    - iOS: App Store                   │
[SETUP] │    - Android: Play Store              │
[SETUP] │                                        │
[SETUP] │ 2. Open app and scan for devices      │
[SETUP] │                                        │
[SETUP] │ 3. Connect to: Dymolabel-ESP32        │
[SETUP] │ 4. Enter proof of possession (PoP):   │
[SETUP] │    dymolabel123                        │
[SETUP] │                                        │
[SETUP] │ 5. Select your WiFi network and enter │
[SETUP] │    the password                        │
[SETUP] └────────────────────────────────────────┘
```

### Step 3: Provision via App

**iOS Instructions:**

1. Open **ESP BLE Provisioning** app
2. Tap "Provision New Device"
3. Select "BLE" as transport
4. App will scan for nearby devices
5. Tap on **"Dymolabel-ESP32"** (or your custom device name)
6. If using Proof of Possession (PoP):
   - Enter PoP: `dymolabel123` (default)
   - Tap "Next"
7. Select your WiFi network from the list
8. Enter WiFi password
9. Tap "Provision"
10. Wait for success message

**Android Instructions:**

1. Open **ESP BLE Provisioning** app
2. Tap "Provision Device"
3. Select "BLE" transport
4. Grant Bluetooth and Location permissions if requested
5. App will scan for devices
6. Tap **"Dymolabel-ESP32"**
7. If using PoP:
   - Enter `dymolabel123`
   - Tap "Next"
8. Choose your WiFi network
9. Enter password
10. Tap "Provision"
11. Wait for confirmation

### Step 4: Verify Connection

After successful provisioning:

1. ESP32 will automatically connect to WiFi
2. Check serial monitor for IP address:

```
[PROV] ✓ Provisioning successful
[SETUP] ✓ WiFi connected
[SETUP] SSID: YourWiFiNetwork
[SETUP] IP Address: 192.168.1.100
[SETUP] Signal Strength: -45 dBm
[SETUP] ✓ mDNS started: http://dymolabel.local
```

3. Access web interface at:
   - `http://dymolabel.local`
   - Or the IP address shown

---

## Configuration Options

### Device Name

Change the device name shown during provisioning:

**Edit `src/config.h`:**
```cpp
#define WIFI_PROV_DEVICE_NAME "MyLabelPrinter"
```

### Proof of Possession (PoP)

PoP acts like a password to prevent unauthorized provisioning.

**Enable PoP (Recommended):**
```cpp
#define WIFI_PROV_USE_POP
#define WIFI_PROV_POP "your-secret-code"
```

**Disable PoP (Less Secure):**
```cpp
// #define WIFI_PROV_USE_POP    // Comment out this line
```

With PoP disabled, anyone can provision the device, but setup is simpler.

### Reset Provisioning on Boot (Testing Only)

For development, you can force re-provisioning on every boot:

```cpp
#define WIFI_PROV_RESET_ON_BOOT
```

⚠️ **Warning:** This erases stored WiFi credentials on each reboot. Only use for testing!

---

## Changing WiFi Networks

### Method 1: Re-provision via App

1. Enable reset mode in `config.h`:
   ```cpp
   #define WIFI_PROV_RESET_ON_BOOT
   ```
2. Upload firmware
3. Power cycle the device
4. Follow provisioning steps again
5. After successful provisioning, disable reset mode:
   ```cpp
   // #define WIFI_PROV_RESET_ON_BOOT
   ```
6. Upload firmware again

### Method 2: Reset via API (Future Feature)

Add a reset endpoint to the web server:

```cpp
_server.on("/api/reset-wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
    WiFiProv.resetProvisioning();
    request->send(200, "text/plain", "WiFi reset. Reboot device to provision.");
    delay(1000);
    ESP.restart();
});
```

Access via:
```bash
curl -X POST http://dymolabel.local/api/reset-wifi
```

### Method 3: Factory Reset Button

Add a physical button to GPIO (e.g., GPIO0):

```cpp
void loop() {
    // Check if button held for 5 seconds
    if (digitalRead(0) == LOW) {
        unsigned long pressStart = millis();
        while (digitalRead(0) == LOW) {
            if (millis() - pressStart > 5000) {
                Serial.println("Factory reset triggered");
                WiFiProv.resetProvisioning();
                ESP.restart();
            }
            delay(100);
        }
    }

    // ... rest of loop code
}
```

---

## Troubleshooting

### Device Not Found in App

**Symptoms:**
- App can't find "Dymolabel-ESP32" device

**Solutions:**
1. Ensure Bluetooth is enabled on phone
2. Grant location permissions (Android requirement)
3. Check serial monitor - device should show provisioning mode
4. Move phone closer to ESP32
5. Restart ESP32
6. Restart provisioning app

### Provisioning Fails

**Symptoms:**
- App shows "Provisioning failed" error

**Solutions:**
1. Verify WiFi password is correct
2. Ensure WiFi network is 2.4GHz (not 5GHz)
3. Check WiFi network is reachable from device location
4. Try without PoP first (comment out `WIFI_PROV_USE_POP`)
5. Check serial monitor for error messages

### Wrong PoP Entered

**Symptoms:**
- App rejects PoP code

**Solutions:**
1. Double-check PoP in `config.h` matches what you're entering
2. Default PoP: `dymolabel123`
3. Case-sensitive - ensure exact match
4. Try disabling PoP temporarily to test

### Device Won't Connect After Provisioning

**Symptoms:**
- Provisioning succeeds but device doesn't connect

**Solutions:**
1. Check WiFi password was entered correctly
2. Verify network is 2.4GHz (ESP32 doesn't support 5GHz)
3. Check router allows new devices to connect
4. Try closer to WiFi router
5. Check serial monitor for specific error

### Already Provisioned - Can't Re-provision

**Symptoms:**
- Device immediately connects, can't change WiFi

**Solutions:**

**Option A - Temporary Reset:**
```cpp
// In config.h, temporarily enable:
#define WIFI_PROV_RESET_ON_BOOT
// Upload, provision, then disable and upload again
```

**Option B - Manual NVS Erase:**
```bash
# Erase NVS partition
pio run --target erase
# Re-upload firmware
pio run --target upload
```

**Option C - Factory Reset via Serial:**
Connect to serial monitor and send:
```
WiFiProv.resetProvisioning()
ESP.restart()
```

---

## Security Considerations

### Proof of Possession (PoP)

**With PoP enabled:**
- ✅ Prevents unauthorized provisioning
- ✅ Encrypted BLE communication
- ✅ Recommended for production use

**Without PoP:**
- ⚠️ Anyone nearby can provision device
- ⚠️ Only use in secure environments
- ✅ Easier for testing and demos

### Best Practices

1. **Change Default PoP:**
   ```cpp
   #define WIFI_PROV_POP "your-unique-secret-123"
   ```

2. **Use Strong WiFi Passwords:**
   - Provisioned credentials are stored encrypted in NVS
   - But still use WPA2/WPA3 with strong password

3. **Disable Provisioning After Setup:**
   - Once provisioned, BLE is automatically disabled
   - Saves power and reduces attack surface

4. **Network Isolation:**
   - Consider placing ESP32 on IoT VLAN
   - Limit access from untrusted networks

---

## Advanced Configuration

### Custom Service UUID

Change BLE service UUID for multiple devices:

```cpp
// In main.cpp
uint8_t custom_service_uuid[16] = {
    0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
    0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02
};
WiFiProv.beginProvision(..., custom_service_uuid);
```

### Provisioning Timeout

Add timeout to provisioning:

```cpp
// Wait maximum 5 minutes for provisioning
int timeout = 300; // seconds
while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(1000);
    timeout--;
}

if (timeout == 0) {
    Serial.println("Provisioning timeout - restarting");
    ESP.restart();
}
```

### Multi-Device Provisioning

Provision multiple ESP32 devices with unique names:

```cpp
// Use MAC address in device name for uniqueness
String deviceName = "Dymolabel-" + WiFi.macAddress().substring(12, 17);
deviceName.replace(":", "");
WiFiProv.beginProvision(..., deviceName.c_str());
```

Example output: `Dymolabel-A1B2C3`

---

## Web Interface for Provisioning

You can also add a web-based provisioning interface for when the device is in AP mode:

```cpp
// Start AP mode if not provisioned
if (!provisioned) {
    WiFi.softAP("Dymolabel-Setup", "dymolabel123");
    // Serve web form for WiFi credentials
    // User submits form, device connects, stores credentials
}
```

This provides an alternative to the BLE app for users who prefer web-based setup.

---

## Comparison with Other Methods

| Feature | WiFi Provisioning | Hardcoded Credentials | SmartConfig | WPS |
|---------|-------------------|----------------------|-------------|-----|
| Security | ✅ Encrypted | ❌ Exposed in code | ⚠️ Medium | ⚠️ Deprecated |
| User Friendly | ✅ App-based | ❌ Requires flashing | ⚠️ Complex | ⚠️ Router button |
| Change WiFi | ✅ Easy | ❌ Re-flash needed | ⚠️ Medium | ⚠️ Medium |
| Multi-Device | ✅ Excellent | ❌ Manual per device | ✅ Good | ⚠️ One at a time |
| Mobile App | ✅ Official app | ❌ None | ⚠️ Various | ❌ None |

---

## References

- [ESP-IDF WiFi Provisioning Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/provisioning.html)
- [ESP BLE Provisioning App - iOS](https://apps.apple.com/app/esp-ble-provisioning/id1473590141)
- [ESP BLE Provisioning App - Android](https://play.google.com/store/apps/details?id=com.espressif.provble)
- [Arduino ESP32 WiFiProv Library](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFiProv)

---

## Quick Reference

**Provision New Device:**
1. Power on ESP32
2. Open ESP BLE Provisioning app
3. Scan and connect
4. Enter PoP if enabled
5. Select WiFi and enter password
6. Wait for success

**Reset Provisioning:**
```cpp
WiFiProv.resetProvisioning();
ESP.restart();
```

**Check if Provisioned:**
```cpp
bool isConfigured = WiFiProv.isProvisioned();
```

**Default Settings:**
- Device Name: `Dymolabel-ESP32`
- PoP: `dymolabel123`
- Security: WPA2/WPA3
- Transport: BLE
