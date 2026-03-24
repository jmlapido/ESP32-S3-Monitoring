import json
from fastapi import WebSocket, WebSocketDisconnect


class WebConnectionManager:
    def __init__(self):
        self._connections: list[WebSocket] = []

    @property
    def count(self) -> int:
        return len(self._connections)

    async def connect(self, ws: WebSocket):
        await ws.accept()
        self._connections.append(ws)

    def disconnect(self, ws: WebSocket):
        if ws in self._connections:
            self._connections.remove(ws)

    async def broadcast(self, data: dict):
        dead = []
        for ws in self._connections:
            try:
                await ws.send_json(data)
            except Exception:
                dead.append(ws)
        for ws in dead:
            self._connections.remove(ws)


web_manager = WebConnectionManager()


async def web_websocket_endpoint(websocket: WebSocket):
    await web_manager.connect(websocket)
    try:
        while True:
            raw = await websocket.receive_text()
            # Web panel can send commands if needed
            try:
                msg = json.loads(raw)
                msg_type = msg.get("type", "")
                if msg_type == "ping":
                    await websocket.send_json({"type": "pong"})
            except json.JSONDecodeError:
                pass
    except WebSocketDisconnect:
        web_manager.disconnect(websocket)
    except Exception:
        web_manager.disconnect(websocket)
