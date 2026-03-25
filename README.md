# MonDash - ESP32-S3 Monitoring Dashboard

A full-stack monitoring system that turns an ESP32-S3 touchscreen board into a real-time status display for websites, network devices (WISP), and RTSP cameras. The ESP32 acts as a thin display client while a Python backend on Ubuntu handles all data collection and serves a web control panel.

## Architecture

```
Ubuntu Server (LAN / Tailscale)             ESP32-S3 (WiFi)
┌────────────────────────────┐              ┌──────────────────────┐
│  FastAPI Backend (:8000)   │  WebSocket   │  LVGL 9 Dark UI      │
│  ├─ REST API (web panel)   │─────────────>│  ├─ Overview tiles   │
│  ├─ WebSocket (ESP push)   │  JSON push   │  ├─ Website status   │
│  ├─ Collectors:            │              │  ├─ Network/SNMP     │
│  │  ├─ HTTP checks         │              │  ├─ Camera grid/live │
│  │  ├─ SNMP polling        │              │  └─ Charts           │
│  │  └─ RTSP thumbnails     │              │  Touch swipe nav     │
│  ├─ MJPEG proxy (FFmpeg)   │              └──────────────────────┘
│  ├─ SQLite (time-series)   │
│  └─ APScheduler            │
├────────────────────────────┤
│  Web Panel (Svelte+TW)     │  ← browser access :8000/app/
│  Dark mode dashboard       │
│  CRUD monitors, charts     │
│  Display layout config     │
└────────────────────────────┘
```

## Hardware

| Component | Chip | Interface | Key Pins |
|---|---|---|---|
| Board | Hosyond ES3C28P (ESP32-S3) | — | 16MB Flash QIO, 8MB PSRAM OPI |
| Display | ILI9341V 240×320 | SPI (20MHz) | MOSI:11, SCK:12, CS:10, DC:46, BL:45 |
| Touch | FT6336G | I2C (400kHz) | SDA:16, SCL:15, RST:18, INT:17 |
| LED | WS2812B (1×) | RMT | Data:42 |
| Audio | FM8002E | I2S | (unused in this project) |

## Features

### ESP32-S3 Display
- **Dark mode UI** with modern color palette (GitHub dark inspired)
- **Swipeable touch pages**: Overview → Websites → Network → Cameras → Charts
- **16px compact top bar** with connection status dot and time
- **Floating page dots** — transparent, auto-hide after 2 seconds
- **304px content area** maximized for data display
- **LVGL 9.x** with hardware-accelerated rendering, dual draw buffers in PSRAM
- **Dual-core FreeRTOS**: LVGL on core 0, data processing on core 1
- **MJPEG live view** for cameras (server transcodes H.264 → MJPEG via FFmpeg)

### Backend Server
- **HTTP monitoring**: Async checks with response time tracking, up/down status
- **SNMP monitoring**: Presets for Ubiquiti and Mikrotik devices, client counts, bandwidth
- **RTSP cameras**: FFmpeg thumbnail capture + live MJPEG proxy streaming
- **WebSocket push**: Real-time updates to ESP and web panel simultaneously
- **SQLite time-series**: WAL mode, hourly aggregation, configurable retention
- **APScheduler**: Per-monitor intervals, maintenance jobs

### Web Control Panel
- **Svelte 5 + Tailwind CSS v4** — dark mode, modern responsive design
- **Dashboard**: Live status cards, website list, device grid
- **Monitor management**: Add/edit/delete HTTP, SNMP, RTSP monitors with instant test
- **Display config**: Reorder ESP pages, preview 240×320 layout
- **History**: Interactive Chart.js graphs with time range picker and stats
- **Settings**: Server info, ESP connection status, data retention

## Deployment (Production)

### Server

| | |
|---|---|
| OS | Ubuntu 25.04 |
| Local IP | `192.168.1.24` |
| Tailscale IP | `100.123.83.86` |
| Web Panel | `http://192.168.1.24:8000/app/` |
| API Docs | `http://192.168.1.24:8000/docs` |
| Service | `systemd` — `mondash.service` |
| Repo location | `/opt/mondash/` |

```bash
# Service management
systemctl status mondash
systemctl restart mondash
journalctl -u mondash -f
```

## Project Structure

```
Esp32-S3_Monitoring/
├── firmware/                    # ESP32-S3 PlatformIO project
│   ├── platformio.ini           # Build config (espressif32@6.9.0, ESP-IDF 5.3.1)
│   ├── sdkconfig.defaults       # PSRAM OPI, Flash QIO 16MB, LVGL, WiFi config
│   ├── partitions.csv           # Custom partition table (~3.5MB app)
│   └── main/
│       ├── main.c               # Entry point, dual-core task init
│       ├── hal/                 # Hardware abstraction layer
│       │   ├── display_hal.c    # ILI9341V SPI + LVGL driver
│       │   ├── touch_hal.c      # FT6336G I2C + LVGL indev
│       │   └── led_hal.c        # WS2812B status LED
│       ├── net/                 # Networking
│       │   ├── wifi_manager.c   # WiFi STA, auto-reconnect
│       │   └── ws_client.c      # WebSocket client + JSON routing
│       ├── data/                # Data management
│       │   ├── data_store.c     # PSRAM cache with mutex
│       │   └── json_parser.c    # cJSON message parser
│       └── ui/                  # LVGL user interface
│           ├── ui_theme.c       # Dark color palette
│           ├── ui_manager.c     # Top bar, tileview, floating page dots
│           ├── pages/
│           │   ├── page_overview.c
│           │   ├── page_websites.c
│           │   ├── page_network.c
│           │   ├── page_cameras.c
│           │   └── page_chart.c
│           └── widgets/
│               ├── status_card.c
│               └── status_row.c
│
├── server/                      # Python backend
│   ├── pyproject.toml           # Dependencies
│   ├── .env                     # Runtime config (not in git)
│   └── app/
│       ├── main.py              # FastAPI app, WebSocket endpoints, static files
│       ├── config.py            # pydantic-settings
│       ├── database.py          # SQLite schema, WAL mode
│       ├── scheduler.py         # APScheduler job management
│       ├── api/                 # REST API routes
│       │   ├── monitors.py      # CRUD for monitors
│       │   ├── display.py       # ESP display config
│       │   ├── history.py       # Time-series data queries
│       │   └── system.py        # Server & ESP status
│       ├── ws/                  # WebSocket handlers
│       │   ├── esp_handler.py   # ESP32 connection manager (push model)
│       │   └── web_handler.py   # Web panel broadcast
│       ├── collectors/          # Data collection
│       │   ├── base.py
│       │   ├── http_collector.py
│       │   ├── snmp_collector.py
│       │   └── rtsp_collector.py
│       ├── streaming/
│       │   └── mjpeg_proxy.py   # FFmpeg RTSP→MJPEG transcoder
│       └── services/
│           ├── data_service.py
│           ├── display_service.py
│           └── thumbnail_service.py
│
└── web/                         # Svelte + Tailwind SPA
    ├── package.json
    ├── svelte.config.js         # adapter-static (SPA mode)
    ├── vite.config.ts           # Dev proxy to backend :8000
    └── src/
        ├── lib/
        │   ├── api.ts           # Typed API client
        │   ├── stores.ts        # Svelte stores + WebSocket auto-reconnect
        │   └── components/
        │       ├── StatusBadge.svelte
        │       ├── MonitorCard.svelte
        │       ├── Chart.svelte
        │       └── Toast.svelte
        └── routes/
            ├── +layout.svelte   # Sidebar nav, ESP status dot
            ├── +page.svelte     # Dashboard
            ├── monitors/
            ├── display/
            ├── history/
            └── settings/
```

## Setup

### Prerequisites
- **ESP32-S3**: PlatformIO + VSCode (Windows)
- **Ubuntu Server**: Python 3.13+, Node.js 20+, FFmpeg
- **Network**: Both devices on same LAN, or server reachable via Tailscale

### 1. Backend Server (Ubuntu)

```bash
# Install system dependencies
apt update && apt install -y ffmpeg git python3-venv python3-pip
curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
apt install -y nodejs

# Clone and install
cd /opt
git clone https://github.com/jmlapido/ESP32-S3-Monitoring.git mondash

# Python venv
cd /opt/mondash/server
python3 -m venv venv
source venv/bin/activate
pip install fastapi "uvicorn[standard]" websockets aiosqlite httpx \
    "pysnmp-lextudio>=6.1.0" apscheduler pillow ffmpeg-python pydantic-settings

# Build web panel
cd /opt/mondash/web
npm install --legacy-peer-deps
npm run build
mkdir -p /opt/mondash/server/static
cp -r build/* /opt/mondash/server/static/

# Configure
cat > /opt/mondash/server/.env << EOF
DATABASE_PATH=/opt/mondash/data/mondash.db
HOST=0.0.0.0
PORT=8000
RETENTION_DAYS=30
CHECK_INTERVAL_DEFAULT=60
EOF
mkdir -p /opt/mondash/data

# Install as systemd service
cat > /etc/systemd/system/mondash.service << EOF
[Unit]
Description=MonDash Backend
After=network.target

[Service]
User=root
WorkingDirectory=/opt/mondash/server
EnvironmentFile=/opt/mondash/server/.env
ExecStart=/opt/mondash/server/venv/bin/uvicorn app.main:app --host 0.0.0.0 --port 8000
Restart=always

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable --now mondash
```

Web panel accessible at `http://<server-ip>:8000/app/`

### 2. ESP32-S3 Firmware (Windows)

> **Important**: ESP-IDF does not support spaces in project paths. Use a directory junction to work around the OneDrive path:
> ```powershell
> New-Item -ItemType Junction -Path C:\mondash -Target "C:\Users\<user>\...\firmware"
> ```

```powershell
# Always build from the junction path
cd C:\mondash

# Set WiFi credentials (edit sdkconfig.defaults or use menuconfig)
# CONFIG_MONDASH_WIFI_SSID="YourSSID"
# CONFIG_MONDASH_WIFI_PASSWORD="YourPassword"
# CONFIG_MONDASH_SERVER_URI="ws://192.168.1.24:8000/ws/esp"

# Build and flash
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload

# Serial monitor
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" device monitor --baud 115200
```

### 3. Tailscale (Remote Access)

```bash
# Server is already on Tailscale as esp32-s3-server (100.123.83.86)
# Web panel accessible remotely at http://100.123.83.86:8000/app/
# ESP32 uses local LAN IP (192.168.1.24) — no Tailscale needed on ESP
```

## Known Hardware Notes

### ILI9341V Display
- Uses ST7789 driver from esp_lcd (compatible, same command set)
- Requires **byte-swapped RGB565** — handled in `flush_cb` via software swap
- Requires **X-axis mirror** (`esp_lcd_panel_mirror(true, false)`) for correct orientation
- SPI at 20MHz; increase to 40MHz only if signal quality is confirmed

### FT6336G Touch
- Uses new `esp_lcd_touch_get_data()` API (replaces deprecated `get_coordinates`)
- I2C at 400kHz on GPIO 15/16

### WS2812B LED
- Uses `led_strip` component via RMT on GPIO 42
- Status colors: Red=no WiFi, Yellow=WiFi/no server, Blue=connecting, Green=connected

## Configuration

### ESP32 Kconfig (sdkconfig.defaults)

| Option | Default | Description |
|---|---|---|
| `CONFIG_MONDASH_WIFI_SSID` | MyNetwork | WiFi SSID |
| `CONFIG_MONDASH_WIFI_PASSWORD` | password | WiFi password |
| `CONFIG_MONDASH_SERVER_URI` | ws://192.168.1.24:8000/ws/esp | Backend WebSocket URI |
| `CONFIG_MONDASH_MJPEG_BASE_URI` | http://192.168.1.24:8000/stream | MJPEG stream base URL |

### Backend (.env)

| Variable | Default | Description |
|---|---|---|
| `DATABASE_PATH` | mondash.db | SQLite database path |
| `HOST` | 0.0.0.0 | Bind address |
| `PORT` | 8000 | HTTP/WS port |
| `RETENTION_DAYS` | 30 | Raw measurement retention |
| `CHECK_INTERVAL_DEFAULT` | 60 | Default poll interval (seconds) |

## WebSocket Protocol

Server pushes JSON to ESP32:

```json
// Website status
{"type":"update","page":"websites","ts":1711300000,"data":{
  "sites":[{"name":"Main Site","status":"up","ms":142}]
}}

// Network devices (SNMP)
{"type":"update","page":"network","ts":1711300000,"data":{
  "devices":[{"name":"AP-Tower1","status":"online","clients":23,"bw_mbps":145.2}]
}}

// Camera thumbnails (RGB565 base64)
{"type":"update","page":"cameras","ts":1711300000,"data":{
  "cameras":[{"name":"Front Gate","status":"online","thumb_rgb565":"<b64>","w":120,"h":90}]
}}

// Bandwidth chart
{"type":"chart_data","page":"bandwidth","ts":1711300000,"data":{
  "labels":[0,1,2,3],"series":[120,135,128,142]
}}
```

## Memory Budget (ESP32-S3)

| Resource | Size | Location |
|---|---|---|
| LVGL draw buffers (2× 240×40) | ~38 KB | PSRAM |
| LVGL objects | ~50–80 KB | Internal SRAM |
| Camera thumbnails (4× 120×90) | ~86 KB | PSRAM |
| MJPEG frame buffer (live view) | ~86 KB | PSRAM (when active) |
| WebSocket RX buffer | ~8 KB | PSRAM |
| Data cache | ~16 KB | PSRAM |
| **Free PSRAM** | **~7.5 MB** | — |

## API Reference

| Method | Endpoint | Description |
|---|---|---|
| GET | `/health` | Health check |
| GET | `/api/monitors/` | List all monitors |
| POST | `/api/monitors/` | Create monitor |
| GET | `/api/monitors/{id}` | Get monitor |
| PUT | `/api/monitors/{id}` | Update monitor |
| DELETE | `/api/monitors/{id}` | Delete monitor |
| POST | `/api/monitors/{id}/test` | Run immediate check |
| GET | `/api/display/pages` | Get ESP page config |
| PUT | `/api/display/pages` | Update page layout |
| GET | `/api/display/preview` | Preview ESP payload |
| GET | `/api/history/{id}/data` | Time-series data |
| GET | `/api/history/{id}/summary` | Stats summary |
| GET | `/api/system/status` | Server status |
| GET | `/api/system/esp/status` | ESP connection info |
| GET | `/stream/{camera_id}` | MJPEG live stream |
| WS | `/ws/esp` | ESP32 WebSocket |
| WS | `/ws/web` | Web panel WebSocket |

## SNMP Vendor Presets

### Ubiquiti (airMAX)
- Interface status: `1.3.6.1.2.1.2.2.1.8.1`
- Wireless clients: `1.3.6.1.4.1.41112.1.4.5.1.15.1`
- TX/RX bytes: `1.3.6.1.2.1.2.2.1.16.1` / `1.3.6.1.2.1.2.2.1.10.1`

### Mikrotik (RouterOS)
- Interface status: `1.3.6.1.2.1.2.2.1.8.1`
- Wireless clients: `1.3.6.1.4.1.14988.1.1.1.3.1.6.1`
- TX/RX bytes: `1.3.6.1.2.1.2.2.1.16.1` / `1.3.6.1.2.1.2.2.1.10.1`

## Tech Stack

| Layer | Technology |
|---|---|
| ESP32 Firmware | ESP-IDF 5.3.1, LVGL 9.1, cJSON, FreeRTOS |
| Build System | PlatformIO (espressif32@6.9.0) |
| Backend | Python 3.13, FastAPI, SQLite WAL, APScheduler |
| SNMP | pysnmp-lextudio 6.x |
| RTSP/Streaming | FFmpeg 7.x, ffmpeg-python, Pillow |
| Web Panel | Svelte 5, SvelteKit, Tailwind CSS v4, Chart.js |
| Communication | WebSocket (JSON push), HTTP REST |
| Remote Access | Tailscale |

## License

MIT
