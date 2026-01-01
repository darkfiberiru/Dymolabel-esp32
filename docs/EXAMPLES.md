# Usage Examples

This guide provides practical examples for using the Dymolabel ESP32 system.

## Web UI Examples

### Basic Text Labels

1. Open the web interface at `http://dymolabel.local`
2. Select the **Text** tab
3. Enter your text in the content field
4. Choose font size and alignment
5. Click **Print Label**

**Use Cases:**

- Asset tags: "Asset #12345"
- Name labels: "John Doe"
- File labels: "2026 Tax Documents"
- Warning labels: "Fragile - Handle with Care"

### QR Codes

1. Select the **QR Code** tab
2. Enter the data to encode:
   - URLs: `https://example.com`
   - WiFi credentials: `WIFI:T:WPA;S:NetworkName;P:Password;;`
   - Contact info: `BEGIN:VCARD\nFN:John Doe\nEND:VCARD`
3. Choose size (larger for scanning from distance)
4. Click **Print QR Code**

**Use Cases:**

- Equipment inventory with links to manuals
- Quick WiFi network access
- Contact information sharing
- Product tracking codes

### Barcodes

1. Select the **Barcode** tab
2. Enter numeric or alphanumeric data
3. Choose barcode type:
   - CODE128: General purpose, alphanumeric
   - CODE39: Legacy systems
   - EAN13: Retail products (13 digits)
   - UPC: US retail (12 digits)
4. Click **Print Barcode**

**Use Cases:**

- Product SKUs
- Library book labels
- Inventory tracking
- Shipping labels

## API Examples

### Automated Inventory System

Print labels when new items are added to inventory:

```python
import requests
import json

def print_inventory_label(item_name, sku, quantity):
    """Print a multi-line inventory label"""

    # Print item name
    requests.post('http://dymolabel.local/api/print', json={
        'text': item_name,
        'fontSize': 16,
        'align': 'center'
    })

    # Print SKU barcode
    requests.post('http://dymolabel.local/api/print/barcode', json={
        'data': sku,
        'type': 'CODE128'
    })

    # Print quantity
    requests.post('http://dymolabel.local/api/print', json={
        'text': f'Qty: {quantity}',
        'fontSize': 12,
        'align': 'center'
    })

    # Feed label
    requests.post('http://dymolabel.local/api/feed')

# Example usage
print_inventory_label('Widget Pro', '12345678', 50)
```

### Asset Tagging Script

Batch print asset labels from a CSV file:

```python
import csv
import requests
import time

def print_asset_labels(csv_file):
    """Print asset labels from CSV file"""

    with open(csv_file, 'r') as file:
        reader = csv.DictReader(file)

        for row in reader:
            asset_id = row['AssetID']
            description = row['Description']

            # Print asset ID
            requests.post('http://dymolabel.local/api/print', json={
                'text': f'Asset: {asset_id}',
                'fontSize': 14,
                'align': 'left'
            })

            # Print description
            requests.post('http://dymolabel.local/api/print', json={
                'text': description,
                'fontSize': 10,
                'align': 'left'
            })

            # Print QR code linking to asset database
            requests.post('http://dymolabel.local/api/print/qr', json={
                'data': f'https://assets.company.com/{asset_id}',
                'size': 3
            })

            # Feed and wait between labels
            requests.post('http://dymolabel.local/api/feed')
            time.sleep(2)

# Example: assets.csv contains AssetID,Description
# A001,Dell Laptop
# A002,HP Monitor
print_asset_labels('assets.csv')
```

### WiFi QR Code Generator

Generate WiFi QR codes for easy network access:

```bash
#!/bin/bash

# WiFi credentials
SSID="YourNetworkName"
PASSWORD="YourPassword"
SECURITY="WPA"  # WPA, WEP, or blank for no password

# Generate WiFi QR code format
WIFI_STRING="WIFI:T:${SECURITY};S:${SSID};P:${PASSWORD};;"

# Print label
curl -X POST http://dymolabel.local/api/print \
  -H "Content-Type: application/json" \
  -d "{\"text\":\"WiFi: ${SSID}\",\"fontSize\":12}"

curl -X POST http://dymolabel.local/api/print/qr \
  -H "Content-Type: application/json" \
  -d "{\"data\":\"${WIFI_STRING}\",\"size\":4}"

curl -X POST http://dymolabel.local/api/feed
```

### Home Assistant Integration

Print labels from Home Assistant automations:

```yaml
# configuration.yaml
rest_command:
  print_label:
    url: http://dymolabel.local/api/print
    method: POST
    content_type: 'application/json'
    payload: '{"text":"{{ text }}","fontSize":{{ fontSize }},"align":"{{ align }}"}'

  print_qr:
    url: http://dymolabel.local/api/print/qr
    method: POST
    content_type: 'application/json'
    payload: '{"data":"{{ data }}","size":{{ size }}}'

# Example automation
automation:
  - alias: "Print Package Delivery Label"
    trigger:
      - platform: state
        entity_id: binary_sensor.front_door
        to: 'on'
    action:
      - service: rest_command.print_label
        data:
          text: "Package Delivered"
          fontSize: 16
          align: "center"
```

### Node-RED Flow

Create a Node-RED flow to print labels:

```json
[
    {
        "id": "print_label",
        "type": "http request",
        "method": "POST",
        "url": "http://dymolabel.local/api/print",
        "name": "Print Label",
        "headers": {
            "content-type": "application/json"
        }
    },
    {
        "id": "inject_text",
        "type": "inject",
        "payload": "{\"text\":\"Hello from Node-RED\",\"fontSize\":14}",
        "payloadType": "json",
        "wires": [["print_label"]]
    }
]
```

### Webhook Integration

Print labels when webhooks are received:

```python
from flask import Flask, request
import requests

app = Flask(__name__)

@app.route('/webhook/print', methods=['POST'])
def print_webhook():
    """Receive webhook and print label"""

    data = request.json
    text = data.get('text', 'No text provided')

    # Print the label
    response = requests.post('http://dymolabel.local/api/print', json={
        'text': text,
        'fontSize': 12,
        'align': 'center'
    })

    return response.json()

if __name__ == '__main__':
    app.run(port=5000)
```

## Advanced Examples

### Multi-Line Labels

```python
def print_multiline_label(lines):
    """Print multiple lines on a single label"""

    for i, line in enumerate(lines):
        # Vary font size for hierarchy
        font_size = 16 if i == 0 else 12

        requests.post('http://dymolabel.local/api/print', json={
            'text': line,
            'fontSize': font_size,
            'align': 'center'
        })

    requests.post('http://dymolabel.local/api/feed')

# Example
print_multiline_label([
    'Server Room A',
    'Rack 3 - Slot 5',
    'Dell PowerEdge R740'
])
```

### Cable Labeling

```python
def print_cable_label(location_a, location_b, cable_type):
    """Print labels for both ends of a cable"""

    # Print first end
    requests.post('http://dymolabel.local/api/print', json={
        'text': f'{location_a} → {location_b}',
        'fontSize': 10
    })
    requests.post('http://dymolabel.local/api/print', json={
        'text': cable_type,
        'fontSize': 8
    })
    requests.post('http://dymolabel.local/api/feed')

    # Print second end
    requests.post('http://dymolabel.local/api/print', json={
        'text': f'{location_b} → {location_a}',
        'fontSize': 10
    })
    requests.post('http://dymolabel.local/api/print', json={
        'text': cable_type,
        'fontSize': 8
    })
    requests.post('http://dymolabel.local/api/feed')

# Example
print_cable_label('Router Port 1', 'Switch Port 24', 'Cat6 Ethernet')
```

### Expiration Date Labels

```python
from datetime import datetime, timedelta

def print_expiration_label(item_name, days_until_expiry):
    """Print label with expiration date"""

    expiry_date = datetime.now() + timedelta(days=days_until_expiry)

    requests.post('http://dymolabel.local/api/print', json={
        'text': item_name,
        'fontSize': 14,
        'align': 'center'
    })

    requests.post('http://dymolabel.local/api/print', json={
        'text': f'Expires: {expiry_date.strftime("%Y-%m-%d")}',
        'fontSize': 12,
        'align': 'center'
    })

    requests.post('http://dymolabel.local/api/feed')

# Example
print_expiration_label('Milk', 7)
```

## Tips and Best Practices

1. **Check Status First**: Always check printer status before bulk printing
2. **Add Delays**: Wait 1-2 seconds between labels in batch operations
3. **Test Sizes**: Test font sizes and QR code sizes before bulk printing
4. **Use Feed**: Always feed the label after printing for clean separation
5. **Error Handling**: Implement retry logic for network issues
6. **Label Width**: Ensure content fits within the label width (12mm = 64px)
7. **QR Code Testing**: Test QR codes with different scanners before mass printing
8. **Barcode Validation**: Validate barcode data format before printing

## Troubleshooting

### Labels Not Printing

```python
import requests

# Check printer status
response = requests.get('http://dymolabel.local/api/status')
status = response.json()

if status['status'] == 'disconnected':
    print("Printer is not connected")
elif status['status'] == 'printing':
    print("Printer is busy, wait and retry")
else:
    print("Printer is ready")
```

### Testing Connection

```bash
# Test basic connectivity
curl http://dymolabel.local/api/status

# Test print functionality
curl -X POST http://dymolabel.local/api/print \
  -H "Content-Type: application/json" \
  -d '{"text":"Test","fontSize":12}'
```
