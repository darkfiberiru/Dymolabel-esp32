# API Documentation

The Dymolabel ESP32 provides a RESTful API for programmatic label printing.

## Base URL

```
http://dymolabel.local
```

or

```
http://[ESP32_IP_ADDRESS]
```

## Authentication

Currently, no authentication is required. This should only be used on trusted networks.

## Endpoints

### Print Text Label

Print a text-based label with customizable font size and alignment.

**Endpoint:** `POST /api/print`

**Request Body:**

```json
{
  "text": "Hello World",
  "fontSize": 12,
  "align": "center"
}
```

**Parameters:**

- `text` (required): The text to print
- `fontSize` (optional): Font size in points (8, 12, 16, 20). Default: 12
- `align` (optional): Text alignment ("left", "center", "right"). Default: "center"

**Response:**

```json
{
  "success": true,
  "message": "Label printed successfully"
}
```

**Example:**

```bash
curl -X POST http://dymolabel.local/api/print \
  -H "Content-Type: application/json" \
  -d '{"text":"Hello World","fontSize":16,"align":"center"}'
```

---

### Print QR Code

Generate and print a QR code.

**Endpoint:** `POST /api/print/qr`

**Request Body:**

```json
{
  "data": "https://example.com",
  "size": 3
}
```

**Parameters:**

- `data` (required): The data to encode in the QR code (URL, text, etc.)
- `size` (optional): QR code size multiplier (2, 3, 4). Default: 3

**Response:**

```json
{
  "success": true,
  "message": "QR code printed successfully"
}
```

**Example:**

```bash
curl -X POST http://dymolabel.local/api/print/qr \
  -H "Content-Type: application/json" \
  -d '{"data":"https://github.com","size":3}'
```

---

### Print Barcode

Generate and print a barcode.

**Endpoint:** `POST /api/print/barcode`

**Request Body:**

```json
{
  "data": "1234567890",
  "type": "CODE128"
}
```

**Parameters:**

- `data` (required): The data to encode in the barcode
- `type` (optional): Barcode type. Default: "CODE128"
  - Supported types: CODE128, CODE39, EAN13, UPC

**Response:**

```json
{
  "success": true,
  "message": "Barcode printed successfully"
}
```

**Example:**

```bash
curl -X POST http://dymolabel.local/api/print/barcode \
  -H "Content-Type: application/json" \
  -d '{"data":"1234567890","type":"CODE128"}'
```

---

### Get Printer Status

Check the current status of the printer.

**Endpoint:** `GET /api/status`

**Response:**

```json
{
  "status": "ready",
  "ready": true,
  "printing": false
}
```

**Status Values:**

- `ready`: Printer is connected and ready to print
- `printing`: Currently printing a label
- `disconnected`: Printer is not connected

**Example:**

```bash
curl http://dymolabel.local/api/status
```

---

### Feed Label

Advance the label tape (eject current label).

**Endpoint:** `POST /api/feed`

**Response:**

```json
{
  "success": true,
  "message": "Label fed successfully"
}
```

**Example:**

```bash
curl -X POST http://dymolabel.local/api/feed
```

---

## Error Responses

All endpoints return error responses in the following format:

```json
{
  "success": false,
  "message": "Error description"
}
```

**Common HTTP Status Codes:**

- `200 OK`: Request successful
- `400 Bad Request`: Missing or invalid parameters
- `404 Not Found`: Endpoint not found
- `500 Internal Server Error`: Printer error or internal issue

---

## Integration Examples

### Python

```python
import requests

# Print text label
response = requests.post(
    'http://dymolabel.local/api/print',
    json={'text': 'Hello from Python', 'fontSize': 16}
)
print(response.json())

# Print QR code
response = requests.post(
    'http://dymolabel.local/api/print/qr',
    json={'data': 'https://python.org', 'size': 3}
)
print(response.json())
```

### Node.js

```javascript
const axios = require('axios');

// Print text label
axios.post('http://dymolabel.local/api/print', {
  text: 'Hello from Node.js',
  fontSize: 14,
  align: 'center'
})
.then(response => console.log(response.data))
.catch(error => console.error(error));

// Check status
axios.get('http://dymolabel.local/api/status')
.then(response => console.log(response.data))
.catch(error => console.error(error));
```

### Shell Script

```bash
#!/bin/bash

# Print a label with current date
DATE=$(date +"%Y-%m-%d")
curl -X POST http://dymolabel.local/api/print \
  -H "Content-Type: application/json" \
  -d "{\"text\":\"Date: $DATE\",\"fontSize\":12}"
```

---

## Rate Limiting

There is no rate limiting, but the printer can only process one label at a time. Check the status endpoint before sending multiple print requests.

## CORS

CORS is not currently implemented. The API should only be accessed from the same network as the ESP32.
