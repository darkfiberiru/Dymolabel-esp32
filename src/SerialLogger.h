#ifndef SERIAL_LOGGER_H
#define SERIAL_LOGGER_H

#include <Arduino.h>

// Forward declaration
class LabelWebServer;

// Custom Print class that logs to both Serial and Web Console
class SerialLogger : public Print {
public:
    SerialLogger();

    size_t write(uint8_t c) override {
        // Write to serial
        size_t result = Serial.write(c);

        // Buffer character for web console
        if (c == '\n' || c == '\r') {
            if (_lineBuffer.length() > 0) {
                _sendToConsole(_lineBuffer);
                _lineBuffer = "";
            }
        } else {
            _lineBuffer += (char)c;
        }

        return result;
    }

    size_t write(const uint8_t *buffer, size_t size) override {
        // Write to serial
        size_t result = Serial.write(buffer, size);

        // Process buffer for web console
        for (size_t i = 0; i < size; i++) {
            if (buffer[i] == '\n' || buffer[i] == '\r') {
                if (_lineBuffer.length() > 0) {
                    _sendToConsole(_lineBuffer);
                    _lineBuffer = "";
                }
            } else {
                _lineBuffer += (char)buffer[i];
            }
        }

        return result;
    }

    void flush() {
        Serial.flush();
        if (_lineBuffer.length() > 0) {
            _sendToConsole(_lineBuffer);
            _lineBuffer = "";
        }
    }

    // Called by LabelWebServer when it's ready
    void flushEarlyMessages();

private:
    void _sendToConsole(const String& message);

    String _lineBuffer;
    static const int MAX_EARLY_MESSAGES = 100;
    String _earlyMessages[MAX_EARLY_MESSAGES];
    int _earlyMessageCount;
    bool _webServerReady;
};

// Global instance
extern SerialLogger ConsoleLog;

// Convenience macros for logging
#define LOG_PRINT(...) ConsoleLog.print(__VA_ARGS__)
#define LOG_PRINTLN(...) ConsoleLog.println(__VA_ARGS__)
#define LOG_PRINTF(...) ConsoleLog.printf(__VA_ARGS__)

#endif
