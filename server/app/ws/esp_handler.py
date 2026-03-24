import json
import time
import asyncio
from fastapi import WebSocket, WebSocketDisconnect
from ..services.display_service import build_full_update


class ESPConnectionManager:
    def __init__(self):
        self._ws: WebSocket | None = None
        self._connected = False
        self._last_heartbeat = 0
        self._device_info: dict = {}

    @property
    def is_connected(self) -> bool:
        return self._connected

    @property
    def last_heartbeat(self) -> float:
        return self._last_heartbeat

    @property
    def device_info(self) -> dict:
        return self._device_info

    async def connect(self, ws: WebSocket):
        await ws.accept()
        self._ws = ws
        self._connected = True
        self._last_heartbeat = time.time()

    def disconnect(self):
        self._ws = None
        self._connected = False

    async def send_json(self, data: dict):
        if self._ws and self._connected:
            try:
                await self._ws.send_json(data)
            except Exception:
                self._connected = False

    async def broadcast_update(self, page: str, data: dict):
        msg = {"type": "update", "page": page, "ts": int(time.time()), "data": data}
        await self.send_json(msg)

    async def send_chart_data(self, page: str, labels: list, series: list):
        msg = {
            "type": "chart_data",
            "page": page,
            "ts": int(time.time()),
            "data": {"labels": labels, "series": series},
        }
        await self.send_json(msg)

    async def send_config(self, pages: list[str]):
        await self.send_json({"type": "config", "pages": pages})


esp_manager = ESPConnectionManager()


async def esp_websocket_endpoint(websocket: WebSocket):
    await esp_manager.connect(websocket)

    try:
        # Send initial full data update
        full_data = await build_full_update()
        for msg in full_data:
            await esp_manager.send_json(msg)

        while True:
            raw = await websocket.receive_text()
            esp_manager._last_heartbeat = time.time()

            try:
                msg = json.loads(raw)
                msg_type = msg.get("type", "")

                if msg_type == "hello":
                    esp_manager._device_info = msg
                elif msg_type == "heartbeat":
                    await esp_manager.send_json({"type": "pong"})
                elif msg_type == "page_request":
                    page = msg.get("page", "")
                    # Could trigger specific page data refresh
                    pass
            except json.JSONDecodeError:
                pass

    except WebSocketDisconnect:
        esp_manager.disconnect()
    except Exception:
        esp_manager.disconnect()
