#include "SerialLogger.h"
#include "WebServer.h"

SerialLogger::SerialLogger()
    : _earlyMessageCount(0), _webServerReady(false) {
}

void SerialLogger::_sendToConsole(const String& message) {
    if (_webServerReady) {
        // Web server is ready, send directly
        LabelWebServer::logToConsole(message);
    } else {
        // Web server not ready yet, buffer the message
        if (_earlyMessageCount < MAX_EARLY_MESSAGES) {
            _earlyMessages[_earlyMessageCount++] = message;
        }
        // If buffer is full, oldest messages are lost (could wrap around if needed)
    }
}

void SerialLogger::flushEarlyMessages() {
    _webServerReady = true;

    // Send all buffered early messages to the web server
    for (int i = 0; i < _earlyMessageCount; i++) {
        LabelWebServer::logToConsole(_earlyMessages[i]);
    }

    // Clear the early message buffer to free memory
    _earlyMessageCount = 0;
}

// Global instance of SerialLogger
SerialLogger ConsoleLog;
