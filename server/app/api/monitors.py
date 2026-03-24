import json
import time
from fastapi import APIRouter, HTTPException
from pydantic import BaseModel
from ..database import get_db

router = APIRouter(prefix="/api/monitors", tags=["monitors"])


class MonitorCreate(BaseModel):
    type: str  # http, snmp, rtsp
    name: str
    target: str
    config: dict = {}
    poll_interval_sec: int = 60
    enabled: bool = True
    page_assignment: str = "auto"
    display_order: int = 0


class MonitorUpdate(BaseModel):
    name: str | None = None
    target: str | None = None
    config: dict | None = None
    poll_interval_sec: int | None = None
    enabled: bool | None = None
    page_assignment: str | None = None
    display_order: int | None = None


class MonitorResponse(BaseModel):
    id: int
    type: str
    name: str
    target: str
    config: dict
    poll_interval_sec: int
    enabled: bool
    page_assignment: str
    display_order: int
    created_at: int
    updated_at: int


def _row_to_monitor(row) -> dict:
    return {
        "id": row["id"],
        "type": row["type"],
        "name": row["name"],
        "target": row["target"],
        "config": json.loads(row["config"]) if row["config"] else {},
        "poll_interval_sec": row["poll_interval_sec"],
        "enabled": bool(row["enabled"]),
        "page_assignment": row["page_assignment"],
        "display_order": row["display_order"],
        "created_at": row["created_at"],
        "updated_at": row["updated_at"],
    }


@router.get("/")
async def list_monitors():
    async with get_db() as db:
        cursor = await db.execute(
            "SELECT * FROM monitors ORDER BY display_order, name"
        )
        rows = await cursor.fetchall()
        return [_row_to_monitor(r) for r in rows]


@router.post("/", status_code=201)
async def create_monitor(m: MonitorCreate):
    if m.type not in ("http", "snmp", "rtsp"):
        raise HTTPException(400, "type must be http, snmp, or rtsp")

    async with get_db() as db:
        cursor = await db.execute(
            """INSERT INTO monitors (type, name, target, config, poll_interval_sec, enabled, page_assignment, display_order)
               VALUES (?, ?, ?, ?, ?, ?, ?, ?)""",
            (m.type, m.name, m.target, json.dumps(m.config),
             m.poll_interval_sec, int(m.enabled), m.page_assignment, m.display_order),
        )
        await db.commit()
        monitor_id = cursor.lastrowid

        cursor = await db.execute("SELECT * FROM monitors WHERE id = ?", (monitor_id,))
        row = await cursor.fetchone()
        return _row_to_monitor(row)


@router.get("/{monitor_id}")
async def get_monitor(monitor_id: int):
    async with get_db() as db:
        cursor = await db.execute("SELECT * FROM monitors WHERE id = ?", (monitor_id,))
        row = await cursor.fetchone()
        if not row:
            raise HTTPException(404, "Monitor not found")
        return _row_to_monitor(row)


@router.put("/{monitor_id}")
async def update_monitor(monitor_id: int, m: MonitorUpdate):
    async with get_db() as db:
        cursor = await db.execute("SELECT * FROM monitors WHERE id = ?", (monitor_id,))
        row = await cursor.fetchone()
        if not row:
            raise HTTPException(404, "Monitor not found")

        updates = []
        values = []
        if m.name is not None:
            updates.append("name = ?")
            values.append(m.name)
        if m.target is not None:
            updates.append("target = ?")
            values.append(m.target)
        if m.config is not None:
            updates.append("config = ?")
            values.append(json.dumps(m.config))
        if m.poll_interval_sec is not None:
            updates.append("poll_interval_sec = ?")
            values.append(m.poll_interval_sec)
        if m.enabled is not None:
            updates.append("enabled = ?")
            values.append(int(m.enabled))
        if m.page_assignment is not None:
            updates.append("page_assignment = ?")
            values.append(m.page_assignment)
        if m.display_order is not None:
            updates.append("display_order = ?")
            values.append(m.display_order)

        if updates:
            updates.append("updated_at = ?")
            values.append(int(time.time()))
            values.append(monitor_id)
            sql = f"UPDATE monitors SET {', '.join(updates)} WHERE id = ?"
            await db.execute(sql, values)
            await db.commit()

        cursor = await db.execute("SELECT * FROM monitors WHERE id = ?", (monitor_id,))
        row = await cursor.fetchone()
        return _row_to_monitor(row)


@router.delete("/{monitor_id}")
async def delete_monitor(monitor_id: int):
    async with get_db() as db:
        cursor = await db.execute("SELECT * FROM monitors WHERE id = ?", (monitor_id,))
        if not await cursor.fetchone():
            raise HTTPException(404, "Monitor not found")
        await db.execute("DELETE FROM monitors WHERE id = ?", (monitor_id,))
        await db.commit()
    return {"deleted": True}


@router.post("/{monitor_id}/test")
async def test_monitor(monitor_id: int):
    async with get_db() as db:
        cursor = await db.execute("SELECT * FROM monitors WHERE id = ?", (monitor_id,))
        row = await cursor.fetchone()
        if not row:
            raise HTTPException(404, "Monitor not found")

        monitor = _row_to_monitor(row)

    # Run collector inline
    from ..collectors.http_collector import HTTPCollector
    from ..collectors.snmp_collector import SNMPCollector
    from ..collectors.rtsp_collector import RTSPCollector

    collectors = {"http": HTTPCollector, "snmp": SNMPCollector, "rtsp": RTSPCollector}
    cls = collectors.get(monitor["type"])
    if not cls:
        raise HTTPException(400, f"Unknown monitor type: {monitor['type']}")

    collector = cls(monitor["id"], monitor["target"], monitor["config"])
    result = await collector.collect()
    return {
        "monitor_id": monitor["id"],
        "status": result.status,
        "value_num": result.value_num,
        "value_text": result.value_text,
        "metadata": result.metadata,
    }
