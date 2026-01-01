#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "config.h"
#include "DymoUSB.h"
#include "WebServer.h"

// Global objects
DymoUSB printer;
LabelWebServer* webServer;

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

    // Connect to WiFi
    Serial.printf("[SETUP] Connecting to WiFi: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[SETUP] ✓ WiFi connected");
        Serial.printf("[SETUP] IP Address: %s\n", WiFi.localIP().toString().c_str());

        // Setup mDNS
        if (MDNS.begin(HOSTNAME)) {
            Serial.printf("[SETUP] ✓ mDNS started: http://%s.local\n", HOSTNAME);
            MDNS.addService("http", "tcp", WEB_SERVER_PORT);
        } else {
            Serial.println("[SETUP] ✗ mDNS failed to start");
        }
    } else {
        Serial.println("\n[SETUP] ✗ WiFi connection failed");
        Serial.println("[SETUP] Please check your WiFi credentials in config.h");
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
