#include <Arduino.h>
#include <WiFi.h>
#include <WiFiProv.h>
#include <ESPmDNS.h>
#include "config.h"
#include "DymoUSB.h"
#include "WebServer.h"
#include "SerialLogger.h"

// Global objects
DymoUSB printer;
LabelWebServer* webServer;

// WiFi provisioning event handler
void sysProvEvent(arduino_event_t *sys_event) {
    switch (sys_event->event_id) {
        case ARDUINO_EVENT_PROV_START:
            ConsoleLog.println("[PROV] Provisioning started");
            ConsoleLog.println("[PROV] Use ESP BLE Provisioning app to configure WiFi");
            break;
        case ARDUINO_EVENT_PROV_CRED_RECV:
            ConsoleLog.println("[PROV] Received WiFi credentials");
            ConsoleLog.printf("[PROV] SSID: %s\n", (const char *)sys_event->event_info.prov_cred_recv.ssid);
            break;
        case ARDUINO_EVENT_PROV_CRED_FAIL:
            ConsoleLog.println("[PROV] ✗ Provisioning failed");
            ConsoleLog.printf("[PROV] Reason: %s\n",
                sys_event->event_info.prov_fail_reason == WIFI_PROV_STA_AUTH_ERROR ?
                "WiFi Auth Error" : "WiFi AP Not Found");
            break;
        case ARDUINO_EVENT_PROV_CRED_SUCCESS:
            ConsoleLog.println("[PROV] ✓ Provisioning successful");
            break;
        case ARDUINO_EVENT_PROV_END:
            ConsoleLog.println("[PROV] Provisioning ended");
            break;
        default:
            break;
    }
}

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000);

    ConsoleLog.println("\n\n╔════════════════════════════════════╗");
    ConsoleLog.println("║    Dymolabel ESP32-S3 v1.0.0      ║");
    ConsoleLog.println("╚════════════════════════════════════╝\n");

    // Initialize printer
    ConsoleLog.println("[SETUP] Initializing DYMO printer...");
    if (printer.begin()) {
        ConsoleLog.println("[SETUP] ✓ Printer initialized successfully");
    } else {
        ConsoleLog.println("[SETUP] ✗ Printer initialization failed");
        ConsoleLog.println("[SETUP] Will continue without printer (demo mode)");
    }

    // WiFi Provisioning Setup
    ConsoleLog.println("[SETUP] Initializing WiFi...");
    WiFi.mode(WIFI_STA);

    // Register provisioning event handler
    WiFi.onEvent(sysProvEvent);

    // Check if device is already provisioned
    bool provisioned = false;

#ifdef WIFI_PROV_RESET_ON_BOOT
    // Force reset provisioning for testing
    ConsoleLog.println("[SETUP] Resetting WiFi provisioning (WIFI_PROV_RESET_ON_BOOT enabled)");
    WiFiProv.resetProvisioning();
#endif

    // Try to connect with stored credentials
    ConsoleLog.println("[SETUP] Attempting to connect with stored credentials...");
    WiFi.begin();

    int check_attempts = 0;
    while (WiFi.status() != WL_CONNECTED && check_attempts < 10) {
        delay(500);
        ConsoleLog.print(".");
        check_attempts++;
    }
    ConsoleLog.println();

    if (WiFi.status() == WL_CONNECTED) {
        ConsoleLog.println("[SETUP] ✓ Device already provisioned");
        provisioned = true;
    }

    if (!provisioned) {
        // Start provisioning via BLE
        ConsoleLog.println("[SETUP] Starting WiFi provisioning via BLE...");
        ConsoleLog.println("[SETUP] ┌────────────────────────────────────────┐");
        ConsoleLog.println("[SETUP] │  WiFi Provisioning Instructions       │");
        ConsoleLog.println("[SETUP] ├────────────────────────────────────────┤");
        ConsoleLog.println("[SETUP] │ 1. Install 'ESP BLE Provisioning' app │");
        ConsoleLog.println("[SETUP] │    - iOS: App Store                   │");
        ConsoleLog.println("[SETUP] │    - Android: Play Store              │");
        ConsoleLog.println("[SETUP] │                                        │");
        ConsoleLog.println("[SETUP] │ 2. Open app and scan for devices      │");
        ConsoleLog.println("[SETUP] │                                        │");
        ConsoleLog.printf ("[SETUP] │ 3. Connect to: %-23s │\n", WIFI_PROV_DEVICE_NAME);

#ifdef WIFI_PROV_USE_POP
        ConsoleLog.printf ("[SETUP] │ 4. Enter proof of possession (PoP):   │\n");
        ConsoleLog.printf ("[SETUP] │    %-35s │\n", WIFI_PROV_POP);
#else
        ConsoleLog.println("[SETUP] │ 4. No password required (open)        │");
#endif
        ConsoleLog.println("[SETUP] │                                        │");
        ConsoleLog.println("[SETUP] │ 5. Select your WiFi network and enter │");
        ConsoleLog.println("[SETUP] │    the password                        │");
        ConsoleLog.println("[SETUP] └────────────────────────────────────────┘");

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
    ConsoleLog.println("[SETUP] Waiting for WiFi connection...");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 60) {
        delay(500);
        ConsoleLog.print(".");
        attempts++;
    }
    ConsoleLog.println();

    if (WiFi.status() == WL_CONNECTED) {
        ConsoleLog.println("[SETUP] ✓ WiFi connected");
        ConsoleLog.printf("[SETUP] SSID: %s\n", WiFi.SSID().c_str());
        ConsoleLog.printf("[SETUP] IP Address: %s\n", WiFi.localIP().toString().c_str());
        ConsoleLog.printf("[SETUP] Signal Strength: %d dBm\n", WiFi.RSSI());

        // Setup mDNS
        if (MDNS.begin(HOSTNAME)) {
            ConsoleLog.printf("[SETUP] ✓ mDNS started: http://%s.local\n", HOSTNAME);
            MDNS.addService("http", "tcp", WEB_SERVER_PORT);
        } else {
            ConsoleLog.println("[SETUP] ✗ mDNS failed to start");
        }
    } else {
        ConsoleLog.println("[SETUP] ✗ WiFi connection failed");
        ConsoleLog.println("[SETUP] Please provision WiFi credentials via BLE");
        ConsoleLog.println("[SETUP] The device will continue in AP mode for provisioning");
    }

    // Initialize web server
    ConsoleLog.println("[SETUP] Starting web server...");
    webServer = new LabelWebServer(&printer);

    // Flush early boot messages to web server buffer
    ConsoleLog.flushEarlyMessages();

    if (webServer->begin()) {
        ConsoleLog.println("[SETUP] ✓ Web server started");
        ConsoleLog.printf("[SETUP] Web UI: http://%s.local or http://%s\n",
                     HOSTNAME, WiFi.localIP().toString().c_str());
        ConsoleLog.printf("[SETUP] Console: http://%s.local/console or http://%s/console\n",
                     HOSTNAME, WiFi.localIP().toString().c_str());
    } else {
        ConsoleLog.println("[SETUP] ✗ Web server failed to start");
    }

    ConsoleLog.println("\n[SETUP] ✓ Setup complete!\n");
    ConsoleLog.println("╔════════════════════════════════════╗");
    ConsoleLog.println("║          SYSTEM READY             ║");
    ConsoleLog.println("╚════════════════════════════════════╝\n");
}

void loop() {
    // Handle web server requests
    webServer->handleClients();

    // Small delay to prevent watchdog issues
    delay(10);
}
