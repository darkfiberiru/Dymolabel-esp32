#include "WebServer.h"
#include "config.h"

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

LabelWebServer::LabelWebServer(DymoUSB* printer)
    : _server(WEB_SERVER_PORT), _printer(printer) {
}

bool LabelWebServer::begin() {
    setupRoutes();
    _server.begin();

    #if DEBUG_SERIAL
    Serial.println("[WEB] Web server started on port " + String(WEB_SERVER_PORT));
    #endif

    return true;
}

void LabelWebServer::handleClients() {
    // AsyncWebServer handles clients automatically
    // This method is kept for compatibility
}

void LabelWebServer::setupRoutes() {
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

    // Print endpoints with body handling
    AsyncCallbackJsonWebHandler* printTextHandler = new AsyncCallbackJsonWebHandler("/api/print",
        [this](AsyncWebServerRequest *request, JsonVariant &json) {
            JsonObject jsonObj = json.as<JsonObject>();

            if (!jsonObj.containsKey("text")) {
                sendJsonResponse(request, 400, "Missing 'text' field", false);
                return;
            }

            String text = jsonObj["text"].as<String>();
            int fontSize = jsonObj.containsKey("fontSize") ? jsonObj["fontSize"].as<int>() : 12;
            String align = jsonObj.containsKey("align") ? jsonObj["align"].as<String>() : "center";

            if (_printer->printText(text, fontSize, align)) {
                sendJsonResponse(request, 200, "Label printed successfully");
            } else {
                sendJsonResponse(request, 500, "Failed to print label", false);
            }
        });
    _server.addHandler(printTextHandler);

    AsyncCallbackJsonWebHandler* printQRHandler = new AsyncCallbackJsonWebHandler("/api/print/qr",
        [this](AsyncWebServerRequest *request, JsonVariant &json) {
            JsonObject jsonObj = json.as<JsonObject>();

            if (!jsonObj.containsKey("data")) {
                sendJsonResponse(request, 400, "Missing 'data' field", false);
                return;
            }

            String data = jsonObj["data"].as<String>();
            int size = jsonObj.containsKey("size") ? jsonObj["size"].as<int>() : 3;

            if (_printer->printQRCode(data, size)) {
                sendJsonResponse(request, 200, "QR code printed successfully");
            } else {
                sendJsonResponse(request, 500, "Failed to print QR code", false);
            }
        });
    _server.addHandler(printQRHandler);

    AsyncCallbackJsonWebHandler* printBarcodeHandler = new AsyncCallbackJsonWebHandler("/api/print/barcode",
        [this](AsyncWebServerRequest *request, JsonVariant &json) {
            JsonObject jsonObj = json.as<JsonObject>();

            if (!jsonObj.containsKey("data")) {
                sendJsonResponse(request, 400, "Missing 'data' field", false);
                return;
            }

            String data = jsonObj["data"].as<String>();
            String type = jsonObj.containsKey("type") ? jsonObj["type"].as<String>() : "CODE128";

            if (_printer->printBarcode(data, type)) {
                sendJsonResponse(request, 200, "Barcode printed successfully");
            } else {
                sendJsonResponse(request, 500, "Failed to print barcode", false);
            }
        });
    _server.addHandler(printBarcodeHandler);

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

void LabelWebServer::sendJsonResponse(AsyncWebServerRequest *request, int code, const String& message, bool success) {
    JsonDocument doc;
    doc["success"] = success;
    doc["message"] = message;

    String response;
    serializeJson(doc, response);
    request->send(code, "application/json", response);
}
