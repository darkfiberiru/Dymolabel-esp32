#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "DymoUSB.h"

class LabelWebServer {
public:
    LabelWebServer(DymoUSB* printer);

    // Initialize web server
    bool begin();

    // Handle client requests
    void handleClients();

private:
    AsyncWebServer _server;
    DymoUSB* _printer;

    // Web UI handlers
    void setupRoutes();
    void handleRoot(AsyncWebServerRequest *request);
    void handleNotFound(AsyncWebServerRequest *request);

    // REST API handlers
    void handlePrintText(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handlePrintQR(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handlePrintBarcode(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleStatus(AsyncWebServerRequest *request);
    void handleFeed(AsyncWebServerRequest *request);

    // Helper methods
    void sendJsonResponse(AsyncWebServerRequest *request, int code, const String& message, bool success = true);
};

#endif
