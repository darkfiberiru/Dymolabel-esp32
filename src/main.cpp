#include <Arduino.h>
#include <WiFi.h>
#include <WiFiProv.h>
#include <ESPmDNS.h>
#include "config.h"
#include "DymoUSB.h"
#include "WebServer.h"

// Global objects
DymoUSB printer;
LabelWebServer* webServer;

// WiFi provisioning event handler
void sysProvEvent(arduino_event_t *sys_event) {
    switch (sys_event->event_id) {
        case ARDUINO_EVENT_PROV_START:
            Serial.println("[PROV] Provisioning started");
            Serial.println("[PROV] Use ESP BLE Provisioning app to configure WiFi");
            break;
        case ARDUINO_EVENT_PROV_CRED_RECV:
            Serial.println("[PROV] Received WiFi credentials");
            Serial.printf("[PROV] SSID: %s\n", (const char *)sys_event->event_info.prov_cred_recv.ssid);
            break;
        case ARDUINO_EVENT_PROV_CRED_FAIL:
            Serial.println("[PROV] ✗ Provisioning failed");
            Serial.printf("[PROV] Reason: %s\n",
                sys_event->event_info.prov_fail_reason == WIFI_PROV_STA_AUTH_ERROR ?
                "WiFi Auth Error" : "WiFi AP Not Found");
            break;
        case ARDUINO_EVENT_PROV_CRED_SUCCESS:
            Serial.println("[PROV] ✓ Provisioning successful");
            break;
        case ARDUINO_EVENT_PROV_END:
            Serial.println("[PROV] Provisioning ended");
            break;
        default:
            break;
    }
}

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n╔════════════════════════════════════╗");
    Serial.println("║    Dymolabel ESP32-S3 v1.0.0      ║");
    Serial.println("╚════════════════════════════════════╝\n");

    // Initialize printer
    Serial.println("[SETUP] Initializing DYMO printer...");
    if (printer.begin()) {
        Serial.println("[SETUP] ✓ Printer initialized successfully");
    } else {
        Serial.println("[SETUP] ✗ Printer initialization failed");
        Serial.println("[SETUP] Will continue without printer (demo mode)");
    }

    // WiFi Provisioning Setup
    Serial.println("[SETUP] Initializing WiFi...");
    WiFi.mode(WIFI_STA);

    // Register provisioning event handler
    WiFi.onEvent(sysProvEvent);

    // Check if device is already provisioned
    bool provisioned = false;

#ifdef WIFI_PROV_RESET_ON_BOOT
    // Force reset provisioning for testing
    Serial.println("[SETUP] Resetting WiFi provisioning (WIFI_PROV_RESET_ON_BOOT enabled)");
    WiFiProv.resetProvisioning();
#else
    // Check if already provisioned
    if (WiFiProv.isProvisioned()) {
        Serial.println("[SETUP] Device already provisioned");
        provisioned = true;
    }
#endif

    if (!provisioned) {
        // Start provisioning via BLE
        Serial.println("[SETUP] Starting WiFi provisioning via BLE...");
        Serial.println("[SETUP] ┌────────────────────────────────────────┐");
        Serial.println("[SETUP] │  WiFi Provisioning Instructions       │");
        Serial.println("[SETUP] ├────────────────────────────────────────┤");
        Serial.println("[SETUP] │ 1. Install 'ESP BLE Provisioning' app │");
        Serial.println("[SETUP] │    - iOS: App Store                   │");
        Serial.println("[SETUP] │    - Android: Play Store              │");
        Serial.println("[SETUP] │                                        │");
        Serial.println("[SETUP] │ 2. Open app and scan for devices      │");
        Serial.println("[SETUP] │                                        │");
        Serial.printf ("[SETUP] │ 3. Connect to: %-23s │\n", WIFI_PROV_DEVICE_NAME);

#ifdef WIFI_PROV_USE_POP
        Serial.printf ("[SETUP] │ 4. Enter proof of possession (PoP):   │\n");
        Serial.printf ("[SETUP] │    %s                                  │\n", WIFI_PROV_POP);
#else
        Serial.println("[SETUP] │ 4. No password required (open)        │");
#endif
        Serial.println("[SETUP] │                                        │");
        Serial.println("[SETUP] │ 5. Select your WiFi network and enter │");
        Serial.println("[SETUP] │    the password                        │");
        Serial.println("[SETUP] └────────────────────────────────────────┘");

        // Start BLE provisioning with device name and PoP
#ifdef WIFI_PROV_USE_POP
        WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,
                               WIFI_PROV_SECURITY_1, WIFI_PROV_POP, WIFI_PROV_DEVICE_NAME);
#else
        WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,
                               WIFI_PROV_SECURITY_0, NULL, WIFI_PROV_DEVICE_NAME);
#endif
    }

    // Wait for WiFi connection
    Serial.println("[SETUP] Waiting for WiFi connection...");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 60) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[SETUP] ✓ WiFi connected");
        Serial.printf("[SETUP] SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("[SETUP] IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[SETUP] Signal Strength: %d dBm\n", WiFi.RSSI());

        // Setup mDNS
        if (MDNS.begin(HOSTNAME)) {
            Serial.printf("[SETUP] ✓ mDNS started: http://%s.local\n", HOSTNAME);
            MDNS.addService("http", "tcp", WEB_SERVER_PORT);
        } else {
            Serial.println("[SETUP] ✗ mDNS failed to start");
        }
    } else {
        Serial.println("\n[SETUP] ✗ WiFi connection failed");
        Serial.println("[SETUP] Please provision WiFi credentials via BLE");
        Serial.println("[SETUP] The device will continue in AP mode for provisioning");
    }

    // Initialize web server
    Serial.println("[SETUP] Starting web server...");
    webServer = new LabelWebServer(&printer);
    if (webServer->begin()) {
        Serial.println("[SETUP] ✓ Web server started");
        Serial.printf("[SETUP] Access the UI at: http://%s.local or http://%s\n",
                     HOSTNAME, WiFi.localIP().toString().c_str());
    } else {
        Serial.println("[SETUP] ✗ Web server failed to start");
    }

    Serial.println("\n[SETUP] ✓ Setup complete!\n");
    Serial.println("╔════════════════════════════════════╗");
    Serial.println("║          SYSTEM READY             ║");
    Serial.println("╚════════════════════════════════════╝\n");
}

void loop() {
    // Handle web server requests
    webServer->handleClients();

    // Keep mDNS alive
    MDNS.update();

    // Small delay to prevent watchdog issues
    delay(10);
}
