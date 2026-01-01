# DYMO LabelManager PnP USB Protocol

## Overview

This document describes the USB protocol for DYMO LabelManager PnP label printers, based on research from the [labelle](https://github.com/labelle-org/labelle) and [dymoprint](https://github.com/DavidM42/dymoprint-web-print) projects.

## USB Device Identification

### Device IDs

```
Vendor ID:  0x0922 (DYMO)
Product ID: 0x1001 (bootloader mode - requires mode switch)
Product ID: 0x1002 (printer mode - after mode switch)
```

### Mode Switch

The device initially appears as `0x1001` and requires a USB mode switch command to transition to operational mode (`0x1002`):

```
Mode Switch Command: 0x1B 0x5A 0x01
```

This is typically handled by `usb_modeswitch` on Linux or similar utilities on other platforms.

## Protocol Constants

### Control Bytes

- **ESC (0x1B)**: Escape character for command sequences
- **SYN (0x16)**: Synchronization byte for raster data lines

### Maximum Values

- **Max bytes per line**: 8 (for 12mm tape = 64 pixels)
- **Max tape width**: 19mm (128 pixels)
- **Min tape width**: 6mm (48 pixels)

## Tape Specifications

### Supported Tape Widths

| Tape Width | Pixel Height | Bytes Per Line | Notes |
|------------|--------------|----------------|-------|
| 6mm        | 48 pixels    | 4 bytes        | Narrow labels |
| 9mm        | 64 pixels    | 6 bytes        | Standard labels |
| 12mm       | 64 pixels    | 8 bytes        | **Default** |
| 19mm       | 128 pixels   | 13 bytes       | Wide labels |

### Calculation Formula

From labelle source code:

```python
bytes_per_line = (tape_width_mm * 8) / 12
pixel_height = bytes_per_line * 8
```

## Command Protocol

All commands start with the ESC character (0x1B) followed by a command letter and optional parameters.

### Command Reference

#### 1. Status Request

**Command:** ESC + 'A'

**Format:**
```
0x1B 0x41
```

**Response:** 8 bytes status information

**Purpose:** Check printer status and verify connection

---

#### 2. Dot Tab (Vertical Offset)

**Command:** ESC + 'B' + value

**Format:**
```
0x1B 0x42 [0x00-0x08]
```

**Parameters:**
- `value`: Vertical offset (0-8 pixels)

**Purpose:** Adjust vertical positioning of printed content

---

#### 3. Tape Color

**Command:** ESC + 'C' + value

**Format:**
```
0x1B 0x43 [value]
```

**Parameters:**
- `value`: Color code (typically 0 for default)

**Purpose:** Set tape color/type

---

#### 4. Bytes Per Line

**Command:** ESC + 'D' + value

**Format:**
```
0x1B 0x44 [value]
```

**Parameters:**
- `value`: Number of bytes per raster line (4, 6, 8, or 13)

**Purpose:** Configure tape width

---

#### 5. Cut/Form Feed

**Command:** ESC + 'E'

**Format:**
```
0x1B 0x45
```

**Purpose:** Cut the label tape or advance to cut position

---

#### 6. Raster Line Data

**Command:** SYN + pixel_bytes

**Format:**
```
0x16 [byte0] [byte1] ... [byteN]
```

**Parameters:**
- Each byte represents 8 pixels (MSB = leftmost pixel)
- Number of bytes = tape width in bytes per line

**Purpose:** Send one line of raster image data

---

#### 7. Skip Lines (Margin)

**Command:** Multiple SYN bytes

**Format:**
```
0x16 0x16 0x16 ... (N times)
```

**Purpose:** Create blank space (margins) by skipping lines

---

## Print Sequence

The complete workflow for printing a label:

```cpp
// 1. Initialize printer
send([0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]);  // Init command

// 2. Set tape color
send([0x1B, 'C', 0x00]);  // Color 0 (default)

// 3. Configure bytes per line (for 12mm tape)
send([0x1B, 'D', 0x08]);  // 8 bytes = 64 pixels

// 4. Set dot tab if needed
send([0x1B, 'B', 0x00]);  // No vertical offset

// 5. Send raster data line by line
for (each line in image) {
    send([0x16, byte0, byte1, ..., byte7]);  // SYN + 8 bytes
}

// 6. Add trailing margin (typically 56-112 blank lines)
send([0x16] * 56);  // 56 SYN bytes

// 7. Request status
send([0x1B, 'A']);  // Status check

// 8. Read 8-byte status response
read(8 bytes);
```

## Raster Data Format

### Pixel Encoding

- **1 bit per pixel**: 0 = white, 1 = black
- **8 pixels per byte**: MSB (bit 7) is leftmost pixel
- **Big-endian bit order**

### Example: Single Byte

```
Byte value: 0xAA (10101010 binary)

Pixels:  █ ░ █ ░ █ ░ █ ░
         1 0 1 0 1 0 1 0
```

### Raster Line Example (12mm tape, 8 bytes)

```
Line data: [0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00]

Result: ████████████████                ████████████████
        64 pixels total (8 bytes × 8 bits)
```

### Multi-Line Image

Images are sent row by row from top to bottom. Each row is prefixed with SYN (0x16).

## Physical Dimensions

From labelle source code:

- **Print head to cutter distance**: 8.1mm
- **Print head height**: 8.2mm
- **Default margin**: 56-112 lines (provides space for manual cutting)

## USB Communication

### Endpoints

- **OUT endpoint**: Send commands and data to printer
- **IN endpoint**: Receive status responses

### Transfer Type

- **Interrupt transfers** for HID devices
- **Bulk transfers** for non-HID mode (varies by model)

### Typical Packet Size

- **64 bytes** maximum per transfer

## Implementation Notes

### Initialization Sequence

```cpp
1. Send 8 zero bytes for initialization
2. Set tape color to 0
3. Configure bytes per line based on tape width
4. Optionally set dot tab for vertical adjustment
5. Request status to verify connection
```

### Printing Workflow

```cpp
1. Check printer status (ready/busy)
2. Convert image to raster format
3. Send dot tab command if needed
4. Send each raster line with SYN prefix
5. Send trailing margin (blank lines)
6. Request final status
```

### Error Handling

- **No response to status**: Printer disconnected or busy
- **Partial data sent**: USB communication error
- **Invalid tape width**: Configuration mismatch

## Example Code

### C++ (ESP32 Implementation)

```cpp
// Initialize printer
uint8_t init[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
usbWrite(init, 8);

// Set tape to 12mm (8 bytes per line)
uint8_t tapeCmd[] = {0x1B, 'D', 0x08};
usbWrite(tapeCmd, 3);

// Send one line of data (all black pixels)
uint8_t line[] = {0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
usbWrite(line, 9);

// Cut label
uint8_t cut[] = {0x1B, 'E'};
usbWrite(cut, 2);
```

### Python (dymoprint style)

```python
ESC = 0x1b
SYN = 0x16

# Initialize
dev.write([0x00] * 8)

# Set bytes per line
dev.write([ESC, ord('D'), 8])

# Send raster line
line_data = [0xFF] * 8  # All black pixels
dev.write([SYN] + line_data)

# Cut
dev.write([ESC, ord('E')])
```

## References

- **labelle**: https://github.com/labelle-org/labelle
- **dymoprint**: https://github.com/DavidM42/dymoprint-web-print
- **DYMO LabelWriter Technical Reference**: Official DYMO documentation (referenced in labelle)

## Credits

Protocol information extracted from:
- labelle project (Zachary Yedidia, Sebastian Bronner, and contributors)
- dymoprint project (David M42 and contributors)
- Original dymoprint by Sebastian Bronner

## License

This documentation is based on open-source projects (labelle, dymoprint) which are licensed under GPL/Apache licenses. The protocol itself is not copyrighted.
