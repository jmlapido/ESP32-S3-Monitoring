# MonDash - ESP32-S3 Monitoring Dashboard

A full-stack monitoring system that turns an ESP32-S3 touchscreen board into a real-time status display for websites, network devices (WISP), and RTSP cameras. The ESP32 acts as a thin display client while a Python backend on Ubuntu handles all data collection and serves a web control panel.

## Architecture

```
Ubuntu Server (Tailscale/LAN)                ESP32-S3 (WiFi/Tailscale)
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
| Display | ILI9341V 240x320 | SPI (20MHz) | MOSI:11, SCK:12, CS:10, DC:46, BL:45 |
| Touch | FT6336G | I2C (400kHz) | SDA:16, SCL:15, RST:18, INT:17 |
| LED | WS2812B (1x) | RMT | Data:42 |
| Audio | FM8002E | I2S | (unused in this project) |

## Features

### ESP32-S3 Display
- **Dark mode UI** with modern color palette (GitHub dark inspired)
- **Swipeable touch pages**: Overview > Websites > Network > Cameras > Charts
- **16px compact top bar** with connection status and time
- **Floating page dots** — semi-transparent, auto-hide after 2 seconds
- **304px content area** maximized for data display
- **LVGL 9.x** with hardware-accelerated rendering, dual draw buffers in PSRAM
- **Dual-core**: LVGL on core 0, data processing on core 1
- **MJPEG live view** for cameras (Phase 5 — server transcodes H.264 to MJPEG)

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
- **Display config**: Reorder ESP pages, preview 240x320 layout
- **History**: Interactive Chart.js graphs with time range picker and stats
- **Settings**: Server info, ESP connection status, data retention

## Project Structure

```
Esp32-S3_Monitoring/
├── firmware/                    # ESP32-S3 PlatformIO project
│   ├── platformio.ini           # Build config (espressif32@6.9.0)
│   ├── sdkconfig.defaults       # PSRAM, Flash, LVGL, WiFi config
│   ├── partitions.csv           # Custom partition table (~3.5MB app)
│   └── main/
│       ├── main.c               # Entry point, dual-core init
│       ├── hal/                  # Hardware abstraction
│       │   ├── display_hal.c    # ILI9341V SPI + LVGL driver
│       │   ├── touch_hal.c      # FT6336G I2C + LVGL indev
│       │   └── led_hal.c        # WS2812B status LED
│       ├── net/                  # Networking
│       │   ├── wifi_manager.c   # WiFi STA, auto-reconnect
│       │   └── ws_client.c      # WebSocket client + JSON routing
│       ├── data/                 # Data management
│       │   ├── data_store.c     # PSRAM cache with mutex
│       │   └── json_parser.c    # cJSON message parser
│       └── ui/                   # LVGL user interface
│           ├── ui_theme.c       # Dark color palette
│           ├── ui_manager.c     # Top bar, tileview, page dots
│           ├── pages/           # Swipeable page implementations
│           │   ├── page_overview.c
│           │   ├── page_websites.c
│           │   ├── page_network.c
│           │   ├── page_cameras.c
│           │   └── page_chart.c
│           └── widgets/         # Reusable UI components
│               ├── status_card.c
│               └── status_row.c
│
├── server/                      # Python backend (Ubuntu)
│   ├── pyproject.toml           # Dependencies
│   └── app/
│       ├── main.py              # FastAPI app, WebSocket endpoints
│       ├── config.py            # pydantic-settings
│       ├── database.py          # SQLite schema, WAL mode
│       ├── scheduler.py         # APScheduler job management
│       ├── api/                 # REST API routes
│       │   ├── monitors.py      # CRUD for monitors
│       │   ├── display.py       # ESP display config
│       │   ├── history.py       # Time-series data queries
│       │   └── system.py        # Server & ESP status
│       ├── ws/                  # WebSocket handlers
│       │   ├── esp_handler.py   # ESP32 connection (push model)
│       │   └── web_handler.py   # Web panel connections (broadcast)
│       ├── collectors/          # Data collection
│       │   ├── base.py          # Abstract collector pattern
│       │   ├── http_collector.py
│       │   ├── snmp_collector.py
│       │   └── rtsp_collector.py
│       ├── streaming/
│       │   └── mjpeg_proxy.py   # FFmpeg RTSP→MJPEG transcoder
│       └── services/
│           ├── data_service.py      # Measurement storage/aggregation
│           ├── display_service.py   # ESP payload builder
│           └── thumbnail_service.py # Camera thumbnail cache
│
└── web/                         # Svelte + Tailwind SPA
    ├── package.json
    ├── svelte.config.js         # adapter-static (SPA mode)
    ├── vite.config.ts           # Dev proxy to backend
    └── src/
        ├── app.html             # Dark theme shell
        ├── app.css              # Tailwind v4 + custom vars
        ├── lib/
        │   ├── api.ts           # Typed API client
        │   ├── stores.ts        # Svelte stores + WebSocket
        │   └── components/
        │       ├── StatusBadge.svelte
        │       ├── MonitorCard.svelte
        │       ├── Chart.svelte
        │       └── Toast.svelte
        └── routes/
            ├── +layout.svelte   # Sidebar nav, ESP status
            ├── +page.svelte     # Dashboard
            ├── monitors/        # Monitor management
            ├── display/         # ESP layout config
            ├── history/         # Charts & data explorer
            └── settings/        # System settings
```

## Setup

### Prerequisites
- **ESP32-S3**: PlatformIO + VSCode (Windows/Mac/Linux)
- **Ubuntu Server**: Python 3.11+, Node.js 20+, FFmpeg (for RTSP)
- **Network**: Both devices on same network or connected via Tailscale

### 1. Backend Server (Ubuntu)

```bash
# Install system dependencies
apt update && apt install -y python3.11 python3.11-venv python3-pip ffmpeg nodejs npm

# Clone and install
cd /opt
git clone https://github.com/<your-user>/ESP32-S3-Monitoring.git mondash
cd mondash/server
python3.11 -m venv venv
source venv/bin/activate
pip install -e .

# Run
uvicorn app.main:app --host 0.0.0.0 --port 8000
```

The web panel is accessible at `http://<server-ip>:8000/app/`

### 2. Web Panel (Development)

```bash
cd web
npm install
npm run dev  # Dev server at http://localhost:5173 (proxies API to :8000)

# Build for production (output to server/static/)
npm run build
cp -r build/* ../server/static/
```

### 3. ESP32-S3 Firmware

1. Open `firmware/` in VSCode with PlatformIO
2. Run `menuconfig` to set:
   - `MONDASH_WIFI_SSID` — your WiFi network
   - `MONDASH_WIFI_PASSWORD` — your WiFi password
   - `MONDASH_SERVER_URI` — `ws://<server-ip>:8000/ws/esp`
3. Build and flash: `pio run -t upload`
4. Monitor: `pio device monitor`

### 4. Tailscale (Optional)

If using Tailscale for secure remote access:
```bash
# On Ubuntu server
curl -fsSL https://tailscale.com/install.sh | sh
tailscale up

# On ESP32 — use Tailscale IP in MONDASH_SERVER_URI
# e.g., ws://100.64.0.1:8000/ws/esp
```

## Configuration

### Backend Environment Variables

Create `server/.env`:
```env
MONDASH_DATABASE_PATH=mondash.db
MONDASH_HOST=0.0.0.0
MONDASH_PORT=8000
MONDASH_DEFAULT_HTTP_CHECK_INTERVAL=60
MONDASH_DEFAULT_SNMP_POLL_INTERVAL=30
MONDASH_DEFAULT_RTSP_GRAB_INTERVAL=300
MONDASH_DATA_RETENTION_DAYS=7
MONDASH_HOURLY_RETENTION_DAYS=90
```

### ESP32 Kconfig Options

Set via `pio run -t menuconfig` under "MonDash Configuration":
| Option | Default | Description |
|---|---|---|
| `MONDASH_WIFI_SSID` | MyNetwork | WiFi SSID |
| `MONDASH_WIFI_PASSWORD` | password | WiFi password |
| `MONDASH_SERVER_URI` | ws://100.100.100.100:8000/ws/esp | Backend WebSocket URI |
| `MONDASH_MJPEG_BASE_URI` | http://100.100.100.100:8000/stream/ | MJPEG stream base URL |

## WebSocket Protocol

Server pushes JSON messages to ESP:

```json
// Website status update
{"type":"update","page":"websites","ts":1711300000,"data":{
  "sites":[{"name":"Main Site","status":"up","ms":142}]
}}

// Network device update
{"type":"update","page":"network","ts":1711300000,"data":{
  "devices":[{"name":"AP-Tower1","status":"online","clients":23,"bw_mbps":145.2}]
}}

// Chart data
{"type":"chart_data","page":"bandwidth","ts":1711300000,"data":{
  "labels":[0,1,2,3],"series":[120,135,128,142]
}}

// Camera thumbnails (RGB565 base64 for direct display)
{"type":"update","page":"cameras","ts":1711300000,"data":{
  "cameras":[{"name":"Front Gate","status":"online","thumb_rgb565":"<b64>","w":120,"h":90}]
}}
```

## Memory Budget (ESP32-S3)

| Resource | Size | Location |
|---|---|---|
| LVGL draw buffers (2x 240x40) | ~38 KB | PSRAM |
| LVGL objects | ~50-80 KB | Internal SRAM |
| Camera thumbnails (4x 120x90) | ~86 KB | PSRAM |
| MJPEG frame buffer (live view) | ~86 KB | PSRAM (when active) |
| WebSocket RX buffer | ~8 KB | PSRAM |
| Data cache | ~16 KB | PSRAM |
| **Free PSRAM** | **~7.5 MB** | — |

## API Reference

| Method | Endpoint | Description |
|---|---|---|
| GET | `/api/monitors/` | List all monitors |
| POST | `/api/monitors/` | Create monitor |
| GET | `/api/monitors/{id}` | Get monitor details |
| PUT | `/api/monitors/{id}` | Update monitor |
| DELETE | `/api/monitors/{id}` | Delete monitor |
| POST | `/api/monitors/{id}/test` | Run immediate check |
| GET | `/api/display/pages` | Get ESP page config |
| PUT | `/api/display/pages` | Update page layout |
| GET | `/api/history/{id}/data` | Get time-series data |
| GET | `/api/history/{id}/summary` | Get stats summary |
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
| ESP32 Firmware | ESP-IDF 5.x, LVGL 9.1, cJSON, FreeRTOS |
| Backend | Python 3.11, FastAPI, SQLite (WAL), APScheduler |
| SNMP | pysnmp-lextudio |
| RTSP/Streaming | FFmpeg, ffmpeg-python, Pillow |
| Web Panel | Svelte 5, SvelteKit, Tailwind CSS v4, Chart.js |
| Communication | WebSocket (JSON), HTTP REST |

## License

MIT
