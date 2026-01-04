#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "DymoUSB.h"

// Serial message buffer configuration
#define SERIAL_BUFFER_SIZE 200  // Number of messages to keep in history

// Circular buffer for serial output history
class SerialBuffer {
public:
    SerialBuffer();
    void addMessage(const String& message);
    void sendHistoryTo(AsyncWebSocketClient* client);
    void broadcastToAll(AsyncWebSocket* ws);
    String getLogsAsJson(int limit = 50);  // Get last N logs as JSON array

private:
    String _buffer[SERIAL_BUFFER_SIZE];
    int _writeIndex;
    int _count;
};

class LabelWebServer {
public:
    LabelWebServer(DymoUSB* printer);

    // Initialize web server
    bool begin();

    // Handle client requests
    void handleClients();

    // Send log message to web console
    static void logToConsole(const String& message);

private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;
    DymoUSB* _printer;
    SerialBuffer _serialBuffer;

    // Web UI handlers
    void setupRoutes();
    void handleRoot(AsyncWebServerRequest *request);
    void handleConsole(AsyncWebServerRequest *request);
    void handleNotFound(AsyncWebServerRequest *request);

    // WebSocket handlers
    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                   void *arg, uint8_t *data, size_t len);

    // REST API handlers
    void handlePrintText(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handlePrintQR(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handlePrintBarcode(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleStatus(AsyncWebServerRequest *request);
    void handleFeed(AsyncWebServerRequest *request);
    void handlePrinterCheck(AsyncWebServerRequest *request);
    void handlePrinterReinit(AsyncWebServerRequest *request);
    void handleLogs(AsyncWebServerRequest *request);

    // Helper methods
    void sendJsonResponse(AsyncWebServerRequest *request, int code, const String& message, bool success = true);

    // Static instance for logging
    static LabelWebServer* _instance;
};

#endif
