#include "WebServer.h"
#include "config.h"
#include "SerialLogger.h"
#include <Update.h>

// Web UI HTML (embedded)
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dymolabel ESP32</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 16px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        .header h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
        }
        .status {
            display: inline-block;
            padding: 8px 16px;
            border-radius: 20px;
            background: rgba(255,255,255,0.2);
            margin-top: 10px;
        }
        .status.ready { background: #4ade80; color: #166534; }
        .status.printing { background: #fbbf24; color: #78350f; }
        .status.disconnected { background: #f87171; color: #7f1d1d; }
        .content {
            padding: 30px;
        }
        .tabs {
            display: flex;
            border-bottom: 2px solid #e5e7eb;
            margin-bottom: 30px;
        }
        .tab {
            flex: 1;
            padding: 15px;
            background: none;
            border: none;
            cursor: pointer;
            font-size: 1em;
            font-weight: 600;
            color: #6b7280;
            transition: all 0.3s;
        }
        .tab.active {
            color: #667eea;
            border-bottom: 3px solid #667eea;
            margin-bottom: -2px;
        }
        .tab-content {
            display: none;
        }
        .tab-content.active {
            display: block;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            font-weight: 600;
            margin-bottom: 8px;
            color: #374151;
        }
        input, textarea, select {
            width: 100%;
            padding: 12px;
            border: 2px solid #e5e7eb;
            border-radius: 8px;
            font-size: 1em;
            transition: border-color 0.3s;
        }
        input:focus, textarea:focus, select:focus {
            outline: none;
            border-color: #667eea;
        }
        textarea {
            resize: vertical;
            min-height: 100px;
        }
        button.primary {
            width: 100%;
            padding: 15px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 1.1em;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
        }
        button.primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
        }
        button.primary:active {
            transform: translateY(0);
        }
        button.primary:disabled {
            opacity: 0.5;
            cursor: not-allowed;
            transform: none;
        }
        .message {
            padding: 15px;
            border-radius: 8px;
            margin-top: 20px;
            display: none;
        }
        .message.success {
            background: #d1fae5;
            color: #065f46;
            border: 1px solid #6ee7b7;
        }
        .message.error {
            background: #fee2e2;
            color: #991b1b;
            border: 1px solid #fca5a5;
        }
        .info-box {
            background: #f3f4f6;
            padding: 20px;
            border-radius: 8px;
            margin-top: 20px;
        }
        .info-box h3 {
            color: #374151;
            margin-bottom: 10px;
        }
        .info-box code {
            background: #e5e7eb;
            padding: 2px 6px;
            border-radius: 4px;
            font-family: 'Courier New', monospace;
        }
        .info-box pre {
            background: #1f2937;
            color: #f9fafb;
            padding: 15px;
            border-radius: 6px;
            overflow-x: auto;
            margin-top: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🏷️ Dymolabel ESP32</h1>
            <div class="status" id="status">Connecting...</div>
        </div>

        <div class="content">
            <div class="tabs">
                <button class="tab active" onclick="showTab('text')">Text</button>
                <button class="tab" onclick="showTab('qr')">QR Code</button>
                <button class="tab" onclick="showTab('barcode')">Barcode</button>
                <button class="tab" onclick="showTab('api')">API</button>
                <button class="tab" onclick="window.location.href='/console'">Console</button>
            </div>

            <!-- Text Tab -->
            <div id="text-tab" class="tab-content active">
                <form onsubmit="printText(event)">
                    <div class="form-group">
                        <label for="text-content">Text Content</label>
                        <textarea id="text-content" placeholder="Enter text to print..." required></textarea>
                    </div>
                    <div class="form-group">
                        <label for="text-size">Font Size</label>
                        <select id="text-size">
                            <option value="8">Small (8pt)</option>
                            <option value="12" selected>Medium (12pt)</option>
                            <option value="16">Large (16pt)</option>
                            <option value="20">Extra Large (20pt)</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label for="text-align">Alignment</label>
                        <select id="text-align">
                            <option value="left">Left</option>
                            <option value="center" selected>Center</option>
                            <option value="right">Right</option>
                        </select>
                    </div>
                    <button type="submit" class="primary">Print Label</button>
                </form>
                <div id="text-message" class="message"></div>
            </div>

            <!-- QR Code Tab -->
            <div id="qr-tab" class="tab-content">
                <form onsubmit="printQR(event)">
                    <div class="form-group">
                        <label for="qr-data">QR Code Data</label>
                        <textarea id="qr-data" placeholder="Enter URL or text..." required></textarea>
                    </div>
                    <div class="form-group">
                        <label for="qr-size">QR Code Size</label>
                        <select id="qr-size">
                            <option value="2">Small</option>
                            <option value="3" selected>Medium</option>
                            <option value="4">Large</option>
                        </select>
                    </div>
                    <button type="submit" class="primary">Print QR Code</button>
                </form>
                <div id="qr-message" class="message"></div>
            </div>

            <!-- Barcode Tab -->
            <div id="barcode-tab" class="tab-content">
                <form onsubmit="printBarcode(event)">
                    <div class="form-group">
                        <label for="barcode-data">Barcode Data</label>
                        <input type="text" id="barcode-data" placeholder="Enter barcode number..." required>
                    </div>
                    <div class="form-group">
                        <label for="barcode-type">Barcode Type</label>
                        <select id="barcode-type">
                            <option value="CODE128" selected>Code 128</option>
                            <option value="CODE39">Code 39</option>
                            <option value="EAN13">EAN-13</option>
                            <option value="UPC">UPC</option>
                        </select>
                    </div>
                    <button type="submit" class="primary">Print Barcode</button>
                </form>
                <div id="barcode-message" class="message"></div>
            </div>

            <!-- API Tab -->
            <div id="api-tab" class="tab-content">
                <div class="info-box">
                    <h3>REST API Endpoints</h3>

                    <h4 style="margin-top: 20px;">Print Text Label</h4>
                    <pre>POST /api/print
Content-Type: application/json

{
  "text": "Hello World",
  "fontSize": 12,
  "align": "center"
}</pre>

                    <h4 style="margin-top: 20px;">Print QR Code</h4>
                    <pre>POST /api/print/qr
Content-Type: application/json

{
  "data": "https://example.com",
  "size": 3
}</pre>

                    <h4 style="margin-top: 20px;">Print Barcode</h4>
                    <pre>POST /api/print/barcode
Content-Type: application/json

{
  "data": "1234567890",
  "type": "CODE128"
}</pre>

                    <h4 style="margin-top: 20px;">Get Status</h4>
                    <pre>GET /api/status</pre>

                    <h4 style="margin-top: 20px;">Feed Label</h4>
                    <pre>POST /api/feed</pre>
                </div>
            </div>
        </div>
    </div>

    <script>
        function showTab(tabName) {
            // Hide all tabs
            document.querySelectorAll('.tab-content').forEach(tab => {
                tab.classList.remove('active');
            });
            document.querySelectorAll('.tab').forEach(tab => {
                tab.classList.remove('active');
            });

            // Show selected tab
            document.getElementById(tabName + '-tab').classList.add('active');
            event.target.classList.add('active');
        }

        function showMessage(messageId, text, isError = false) {
            const msg = document.getElementById(messageId);
            msg.textContent = text;
            msg.className = 'message ' + (isError ? 'error' : 'success');
            msg.style.display = 'block';
            setTimeout(() => {
                msg.style.display = 'none';
            }, 5000);
        }

        async function printText(event) {
            event.preventDefault();
            const text = document.getElementById('text-content').value;
            const fontSize = parseInt(document.getElementById('text-size').value);
            const align = document.getElementById('text-align').value;

            try {
                const response = await fetch('/api/print', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ text, fontSize, align })
                });
                const data = await response.json();
                showMessage('text-message', data.message, !data.success);
            } catch (error) {
                showMessage('text-message', 'Failed to connect to printer', true);
            }
        }

        async function printQR(event) {
            event.preventDefault();
            const data = document.getElementById('qr-data').value;
            const size = parseInt(document.getElementById('qr-size').value);

            try {
                const response = await fetch('/api/print/qr', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ data, size })
                });
                const result = await response.json();
                showMessage('qr-message', result.message, !result.success);
            } catch (error) {
                showMessage('qr-message', 'Failed to connect to printer', true);
            }
        }

        async function printBarcode(event) {
            event.preventDefault();
            const data = document.getElementById('barcode-data').value;
            const type = document.getElementById('barcode-type').value;

            try {
                const response = await fetch('/api/print/barcode', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ data, type })
                });
                const result = await response.json();
                showMessage('barcode-message', result.message, !result.success);
            } catch (error) {
                showMessage('barcode-message', 'Failed to connect to printer', true);
            }
        }

        async function updateStatus() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                const statusEl = document.getElementById('status');
                statusEl.textContent = data.status.charAt(0).toUpperCase() + data.status.slice(1);
                statusEl.className = 'status ' + data.status;
            } catch (error) {
                const statusEl = document.getElementById('status');
                statusEl.textContent = 'Disconnected';
                statusEl.className = 'status disconnected';
            }
        }

        // Update status every 2 seconds
        setInterval(updateStatus, 2000);
        updateStatus();
    </script>
</body>
</html>
)rawliteral";

// Console HTML (embedded)
const char CONSOLE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Serial Console - Dymolabel</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Courier New', monospace;
            background: #1e1e1e;
            color: #d4d4d4;
            height: 100vh;
            display: flex;
            flex-direction: column;
        }
        .header {
            background: #2d2d30;
            padding: 15px;
            border-bottom: 1px solid #3e3e42;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .header h1 {
            font-size: 1.2em;
            color: #ffffff;
        }
        .status {
            display: flex;
            align-items: center;
            gap: 10px;
        }
        .status-indicator {
            width: 10px;
            height: 10px;
            border-radius: 50%;
            background: #f87171;
        }
        .status-indicator.connected {
            background: #4ade80;
        }
        .controls {
            display: flex;
            gap: 10px;
        }
        button {
            padding: 8px 15px;
            background: #0e639c;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 0.9em;
        }
        button:hover {
            background: #1177bb;
        }
        #console {
            flex: 1;
            overflow-y: auto;
            padding: 15px;
            font-size: 14px;
            line-height: 1.5;
        }
        .log-entry {
            margin-bottom: 2px;
            white-space: pre-wrap;
            word-wrap: break-word;
        }
        .log-entry.error {
            color: #f87171;
        }
        .log-entry.warning {
            color: #fbbf24;
        }
        .log-entry.info {
            color: #60a5fa;
        }
        .log-entry.success {
            color: #4ade80;
        }
        ::-webkit-scrollbar {
            width: 10px;
        }
        ::-webkit-scrollbar-track {
            background: #2d2d30;
        }
        ::-webkit-scrollbar-thumb {
            background: #555;
            border-radius: 5px;
        }
        ::-webkit-scrollbar-thumb:hover {
            background: #777;
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>📡 Serial Console</h1>
        <div class="status">
            <div class="status-indicator" id="statusIndicator"></div>
            <span id="statusText">Connecting...</span>
        </div>
        <div class="controls">
            <button onclick="clearConsole()">Clear</button>
            <button onclick="toggleAutoScroll()" id="scrollBtn">Auto-scroll: ON</button>
        </div>
    </div>
    <div id="console"></div>

    <script>
        let ws = null;
        let autoScroll = true;
        const consoleEl = document.getElementById('console');
        const statusIndicator = document.getElementById('statusIndicator');
        const statusText = document.getElementById('statusText');

        function connect() {
            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const wsUrl = `${protocol}//${window.location.host}/ws`;

            ws = new WebSocket(wsUrl);

            ws.onopen = () => {
                statusIndicator.classList.add('connected');
                statusText.textContent = 'Connected';
                addLog('WebSocket connected', 'success');
            };

            ws.onclose = () => {
                statusIndicator.classList.remove('connected');
                statusText.textContent = 'Disconnected';
                addLog('WebSocket disconnected. Reconnecting...', 'warning');
                setTimeout(connect, 2000);
            };

            ws.onerror = (error) => {
                addLog('WebSocket error', 'error');
            };

            ws.onmessage = (event) => {
                addLog(event.data);
            };
        }

        function addLog(message, type = '') {
            const entry = document.createElement('div');
            entry.className = 'log-entry' + (type ? ' ' + type : '');

            // Add timestamp
            const now = new Date();
            const timestamp = `[${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}] `;
            entry.textContent = timestamp + message;

            consoleEl.appendChild(entry);

            // Auto-scroll to bottom
            if (autoScroll) {
                consoleEl.scrollTop = consoleEl.scrollHeight;
            }

            // Limit console to 1000 lines
            while (consoleEl.children.length > 1000) {
                consoleEl.removeChild(consoleEl.firstChild);
            }
        }

        function clearConsole() {
            consoleEl.innerHTML = '';
            addLog('Console cleared', 'info');
        }

        function toggleAutoScroll() {
            autoScroll = !autoScroll;
            document.getElementById('scrollBtn').textContent = `Auto-scroll: ${autoScroll ? 'ON' : 'OFF'}`;
        }

        // Connect on load
        connect();
    </script>
</body>
</html>
)rawliteral";

// Serial buffer implementation
SerialBuffer::SerialBuffer() : _writeIndex(0), _count(0) {
}

void SerialBuffer::addMessage(const String& message) {
    _buffer[_writeIndex] = message;
    _writeIndex = (_writeIndex + 1) % SERIAL_BUFFER_SIZE;
    if (_count < SERIAL_BUFFER_SIZE) {
        _count++;
    }
}

void SerialBuffer::sendHistoryTo(AsyncWebSocketClient* client) {
    if (_count == 0) return;

    // Calculate start index for circular buffer
    int startIndex = (_count < SERIAL_BUFFER_SIZE) ? 0 : _writeIndex;

    // Send messages in chronological order
    for (int i = 0; i < _count; i++) {
        int index = (startIndex + i) % SERIAL_BUFFER_SIZE;
        client->text(_buffer[index]);
    }
}

void SerialBuffer::broadcastToAll(AsyncWebSocket* ws) {
    // This is handled per-message, not needed for now
}

String SerialBuffer::getLogsAsJson(int limit) {
    if (_count == 0) {
        return "[]";
    }

    // Limit to actual count
    if (limit > _count) {
        limit = _count;
    }

    JsonDocument doc;
    JsonArray logs = doc.to<JsonArray>();

    // Calculate start index for last N messages
    int startOffset = _count - limit;
    int startIndex = (_count < SERIAL_BUFFER_SIZE) ? startOffset : (_writeIndex + startOffset) % SERIAL_BUFFER_SIZE;

    // Add messages in chronological order
    for (int i = 0; i < limit; i++) {
        int index = (startIndex + i) % SERIAL_BUFFER_SIZE;
        logs.add(_buffer[index]);
    }

    String result;
    serializeJson(doc, result);
    return result;
}

// Static instance for logging
LabelWebServer* LabelWebServer::_instance = nullptr;

LabelWebServer::LabelWebServer(DymoUSB* printer)
    : _server(WEB_SERVER_PORT), _ws("/ws"), _printer(printer) {
    _instance = this;
}

bool LabelWebServer::begin() {
    setupRoutes();
    _server.begin();

    #if DEBUG_SERIAL
    ConsoleLog.println("[WEB] Web server started on port " + String(WEB_SERVER_PORT));
    ConsoleLog.println("[WEB] OTA updates available at http://" + String(HOSTNAME) + ".local/update");
    #endif

    return true;
}

void LabelWebServer::handleClients() {
    // AsyncWebServer handles clients automatically
    // This method is kept for compatibility
}

void LabelWebServer::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                               void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            ConsoleLog.printf("[WS] Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            // Send buffered history to new client
            _serialBuffer.sendHistoryTo(client);
            client->text("--- Live console output ---");
            break;
        case WS_EVT_DISCONNECT:
            ConsoleLog.printf("[WS] Client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            // Handle incoming data if needed
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void LabelWebServer::handleConsole(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", CONSOLE_HTML);
}

void LabelWebServer::logToConsole(const String& message) {
    if (_instance) {
        // Add to buffer for history
        _instance->_serialBuffer.addMessage(message);

        // Broadcast to connected clients
        if (_instance->_ws.count() > 0) {
            _instance->_ws.textAll(message);
        }
    }
}

void LabelWebServer::setupRoutes() {
    // WebSocket handler
    _ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                       void *arg, uint8_t *data, size_t len) {
        onWsEvent(server, client, type, arg, data, len);
    });
    _server.addHandler(&_ws);

    // Console page
    _server.on("/console", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleConsole(request);
    });

    // Web UI
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRoot(request);
    });

    // REST API endpoints
    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleStatus(request);
    });

    _server.on("/api/feed", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleFeed(request);
    });

    // Printer diagnostics endpoints
    _server.on("/api/printer/check", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handlePrinterCheck(request);
    });

    _server.on("/api/printer/reinit", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handlePrinterReinit(request);
    });

    // Logs endpoint
    _server.on("/api/logs", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleLogs(request);
    });

    // Print text endpoint with body handling
    _server.on("/api/print", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            // Response handled in body callback
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                // First chunk - parse JSON
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, data, len);

                if (error) {
                    sendJsonResponse(request, 400, "Invalid JSON", false);
                    return;
                }

                if (doc["text"].isNull()) {
                    sendJsonResponse(request, 400, "Missing 'text' field", false);
                    return;
                }

                String text = doc["text"].as<String>();
                int fontSize = doc["fontSize"] | 12;
                String align = doc["align"] | "center";

                if (_printer->printText(text, fontSize, align)) {
                    sendJsonResponse(request, 200, "Label printed successfully");
                } else {
                    sendJsonResponse(request, 500, "Failed to print label", false);
                }
            }
        });

    // Print QR code endpoint with body handling
    _server.on("/api/print/qr", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            // Response handled in body callback
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                // First chunk - parse JSON
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, data, len);

                if (error) {
                    sendJsonResponse(request, 400, "Invalid JSON", false);
                    return;
                }

                if (doc["data"].isNull()) {
                    sendJsonResponse(request, 400, "Missing 'data' field", false);
                    return;
                }

                String qrData = doc["data"].as<String>();
                int size = doc["size"] | 3;

                if (_printer->printQRCode(qrData, size)) {
                    sendJsonResponse(request, 200, "QR code printed successfully");
                } else {
                    sendJsonResponse(request, 500, "Failed to print QR code", false);
                }
            }
        });

    // Print barcode endpoint with body handling
    _server.on("/api/print/barcode", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            // Response handled in body callback
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                // First chunk - parse JSON
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, data, len);

                if (error) {
                    sendJsonResponse(request, 400, "Invalid JSON", false);
                    return;
                }

                if (doc["data"].isNull()) {
                    sendJsonResponse(request, 400, "Missing 'data' field", false);
                    return;
                }

                String barcodeData = doc["data"].as<String>();
                String type = doc["type"] | "CODE128";

                if (_printer->printBarcode(barcodeData, type)) {
                    sendJsonResponse(request, 200, "Barcode printed successfully");
                } else {
                    sendJsonResponse(request, 500, "Failed to print barcode", false);
                }
            }
        });

    // OTA Update endpoint - serves upload page
    _server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
        const char* updateHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Firmware Update - Dymolabel</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 16px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            padding: 40px;
            max-width: 500px;
            width: 100%;
        }
        h1 { color: #667eea; margin-bottom: 10px; }
        p { color: #666; margin-bottom: 30px; }
        input[type="file"] {
            width: 100%;
            padding: 15px;
            border: 2px dashed #667eea;
            border-radius: 8px;
            margin-bottom: 20px;
            cursor: pointer;
        }
        button {
            width: 100%;
            padding: 15px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s;
        }
        button:hover { transform: translateY(-2px); }
        button:disabled { opacity: 0.5; cursor: not-allowed; }
        #progress {
            width: 100%;
            height: 30px;
            margin-top: 20px;
            display: none;
        }
        #status {
            margin-top: 15px;
            padding: 15px;
            border-radius: 8px;
            display: none;
        }
        .success { background: #d4edda; color: #155724; }
        .error { background: #f8d7da; color: #721c24; }
        .info { background: #d1ecf1; color: #0c5460; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔄 Firmware Update</h1>
        <p>Select a firmware.bin file to update</p>
        <form id="uploadForm">
            <input type="file" id="file" accept=".bin" required>
            <button type="submit" id="uploadBtn">Upload Firmware</button>
        </form>
        <progress id="progress" value="0" max="100"></progress>
        <div id="status"></div>
    </div>
    <script>
        const form = document.getElementById('uploadForm');
        const fileInput = document.getElementById('file');
        const uploadBtn = document.getElementById('uploadBtn');
        const progress = document.getElementById('progress');
        const status = document.getElementById('status');

        form.addEventListener('submit', async (e) => {
            e.preventDefault();
            const file = fileInput.files[0];
            if (!file) return;

            uploadBtn.disabled = true;
            progress.style.display = 'block';
            showStatus('Uploading firmware...', 'info');

            const formData = new FormData();
            formData.append('firmware', file);

            try {
                const xhr = new XMLHttpRequest();
                xhr.upload.addEventListener('progress', (e) => {
                    if (e.lengthComputable) {
                        const percent = (e.loaded / e.total) * 100;
                        progress.value = percent;
                    }
                });

                xhr.addEventListener('load', () => {
                    if (xhr.status === 200) {
                        showStatus('✓ Update successful! Rebooting...', 'success');
                        setTimeout(() => {
                            window.location.href = '/';
                        }, 5000);
                    } else {
                        showStatus('✗ Update failed: ' + xhr.responseText, 'error');
                        uploadBtn.disabled = false;
                    }
                });

                xhr.addEventListener('error', () => {
                    showStatus('✗ Upload failed', 'error');
                    uploadBtn.disabled = false;
                });

                xhr.open('POST', '/update');
                xhr.send(formData);
            } catch (error) {
                showStatus('✗ Error: ' + error.message, 'error');
                uploadBtn.disabled = false;
            }
        });

        function showStatus(msg, type) {
            status.textContent = msg;
            status.className = type;
            status.style.display = 'block';
        }
    </script>
</body>
</html>
)rawliteral";
        request->send(200, "text/html", updateHTML);
    });

    // OTA Update endpoint - handles firmware upload
    _server.on("/update", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            // Handler called after upload completes
            bool shouldReboot = !Update.hasError();
            AsyncWebServerResponse *response = request->beginResponse(200, "text/plain",
                shouldReboot ? "OK" : "FAIL");
            response->addHeader("Connection", "close");
            request->send(response);
            if (shouldReboot) {
                delay(100);
                ESP.restart();
            }
        },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            // File upload handler
            if (!index) {
                ConsoleLog.printf("[OTA] Update start: %s\n", filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            }

            if (len) {
                if (Update.write(data, len) != len) {
                    Update.printError(Serial);
                }
            }

            if (final) {
                if (Update.end(true)) {
                    ConsoleLog.printf("[OTA] Update success: %u bytes\n", index + len);
                } else {
                    Update.printError(Serial);
                }
            }
        });

    // 404 handler
    _server.onNotFound([this](AsyncWebServerRequest *request) {
        handleNotFound(request);
    });
}

void LabelWebServer::handleRoot(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", INDEX_HTML);
}

void LabelWebServer::handleNotFound(AsyncWebServerRequest *request) {
    sendJsonResponse(request, 404, "Endpoint not found", false);
}

void LabelWebServer::handleStatus(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["status"] = _printer->getStatus();
    doc["ready"] = _printer->isReady();
    doc["printing"] = _printer->isPrinting();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void LabelWebServer::handleFeed(AsyncWebServerRequest *request) {
    if (_printer->feedLabel()) {
        sendJsonResponse(request, 200, "Label fed successfully");
    } else {
        sendJsonResponse(request, 500, "Failed to feed label", false);
    }
}

void LabelWebServer::handlePrinterCheck(AsyncWebServerRequest *request) {
    JsonDocument doc;

    doc["connected"] = _printer->isReady();
    doc["status"] = _printer->getStatus();
    doc["printing"] = _printer->isPrinting();
    doc["tapeWidth"] = _printer->getTapeWidth();

    // USB diagnostics
    #ifdef USE_ESP_IDF_USB_HOST
    doc["usbHostEnabled"] = true;
    #else
    doc["usbHostEnabled"] = false;
    #endif

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void LabelWebServer::handlePrinterReinit(AsyncWebServerRequest *request) {
    ConsoleLog.println("[API] Attempting to reinitialize printer...");

    // Reset and try to initialize again
    _printer->reset();
    delay(500);

    bool success = _printer->begin();

    if (success) {
        ConsoleLog.println("[API] ✓ Printer reinitialized successfully");
        sendJsonResponse(request, 200, "Printer reinitialized successfully");
    } else {
        ConsoleLog.println("[API] ✗ Printer reinitialization failed");
        sendJsonResponse(request, 500, "Printer reinitialization failed", false);
    }
}

void LabelWebServer::handleLogs(AsyncWebServerRequest *request) {
    int limit = 50; // Default to last 50 logs

    if (request->hasParam("limit")) {
        limit = request->getParam("limit")->value().toInt();
        if (limit < 1) limit = 1;
        if (limit > SERIAL_BUFFER_SIZE) limit = SERIAL_BUFFER_SIZE;
    }

    String logsJson = _serialBuffer.getLogsAsJson(limit);
    request->send(200, "application/json", logsJson);
}

void LabelWebServer::sendJsonResponse(AsyncWebServerRequest *request, int code, const String& message, bool success) {
    JsonDocument doc;
    doc["success"] = success;
    doc["message"] = message;

    String response;
    serializeJson(doc, response);
    request->send(code, "application/json", response);
}
