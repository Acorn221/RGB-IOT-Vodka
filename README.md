# RGB Vodka

ESP32-based RGB LED controller with HTTP API and WebSocket support.

## Hardware

- ESP32 DevKit
- RGB LED (common cathode)
- Pins: Red=GPIO16, Blue=GPIO17, Green=GPIO18

## Setup

1. Copy `src/creds.h.example` to `src/creds.h` and add your WiFi credentials:
```cpp
#define WIFI_SSID "your_ssid"
#define WIFI_PASSWORD "your_password"
```

2. Build and upload with PlatformIO:
```bash
pio run -t upload
pio device monitor
```

3. Note the IP address printed on boot.

## API

### Set Color Instantly
```bash
curl "http://IP/led?r=255&g=0&b=0"
curl "http://IP/led?color=red"
curl "http://IP/led?color=blue&value=128"
```

### Fade to Color
```bash
curl "http://IP/led/fade?r=255&g=0&b=128&duration=2000"
curl "http://IP/led/fade?color=white&duration=1000"
```

### Built-in Sequences
```bash
curl "http://IP/led/sequence?name=rainbow&speed=500"
curl "http://IP/led/sequence?name=police&speed=300"
curl "http://IP/led/sequence?name=breathe&speed=1000"
curl "http://IP/led/sequence?name=flash&speed=100"
```

Available sequences: `rainbow`, `flash`, `breathe`, `police`

### Custom Sequences
```bash
# Simple sequence
curl -X POST http://IP/led/sequence \
  -H "Content-Type: application/json" \
  -d '{"steps":[{"r":255,"g":0,"b":0,"duration":1000},{"r":0,"g":255,"b":0,"duration":1000}]}'

# With fades
curl -X POST http://IP/led/sequence \
  -H "Content-Type: application/json" \
  -d '{"steps":[
    {"r":255,"g":0,"b":0,"duration":1000,"fade":true},
    {"r":0,"g":255,"b":0,"duration":1000,"fade":true},
    {"r":0,"g":0,"b":255,"duration":1000,"fade":true}
  ]}'

# Looping sequence
curl -X POST http://IP/led/sequence \
  -H "Content-Type: application/json" \
  -d '{"loop":true,"steps":[{"r":255,"g":0,"b":0,"duration":500},{"r":0,"g":0,"b":0,"duration":500}]}'
```

### Queue Management

Add sequences to a queue - they play in order:
```bash
# Queue built-in sequences
curl "http://IP/led/sequence?name=rainbow&speed=500&queue=true"
curl "http://IP/led/sequence?name=police&speed=300&queue=true"

# Queue custom sequence
curl -X POST http://IP/led/sequence \
  -H "Content-Type: application/json" \
  -d '{"queue":true,"steps":[{"r":255,"g":0,"b":0,"duration":2000}]}'

# Clear queue and start fresh
curl -X POST http://IP/led/sequence \
  -H "Content-Type: application/json" \
  -d '{"clear":true,"steps":[{"r":0,"g":255,"b":0,"duration":1000}]}'
```

### Control
```bash
curl "http://IP/led/off"           # Turn off
curl "http://IP/led/stop"          # Stop sequence
curl "http://IP/led/clear"         # Clear queue and stop
curl "http://IP/led/status"        # Get current state
```

## WebSocket

Connect to `ws://IP/ws` for real-time control.

### Commands
```json
{"cmd":"set","r":255,"g":0,"b":0}
{"cmd":"set","color":"red"}
{"cmd":"fade","r":255,"g":0,"b":0,"duration":1000}
{"cmd":"sequence","name":"rainbow","speed":500}
{"cmd":"sequence","name":"rainbow","queue":true}
{"cmd":"custom","steps":[...],"queue":true}
{"cmd":"stop"}
{"cmd":"clear"}
{"cmd":"status"}
```

### Clear + Command
Any command can include `"clear":true` to clear the queue first:
```json
{"clear":true,"cmd":"sequence","name":"rainbow"}
```

## Sequence Step Format

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| r | int | 0 | Red (0-255) |
| g | int | 0 | Green (0-255) |
| b | int | 0 | Blue (0-255) |
| duration | int | 500 | Duration in ms |
| fade | bool | false | Fade from previous color |

## Project Structure

```
src/
├── main.cpp              # Entry point, WiFi setup
├── Config.h              # Pin definitions, constants
├── LedController.h/cpp   # LED control, fades, sequences, queue
├── Routes.h/cpp          # HTTP endpoints
├── WebSocketHandler.h/cpp # WebSocket handler
└── creds.h               # WiFi credentials (not in git)
```

## API Spec

See [openapi.yaml](openapi.yaml) for full OpenAPI 3.0 specification.
