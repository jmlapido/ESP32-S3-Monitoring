import json
import time
from ..database import get_db
from .thumbnail_service import get_thumbnail_rgb565_b64


async def _get_latest_status(monitor_type: str) -> list[dict]:
    """Get latest measurement for each monitor of a given type."""
    async with get_db() as db:
        cursor = await db.execute(
            """SELECT m.id, m.name, m.target, m.type, m.config,
                      ms.value_num, ms.value_text, ms.ts, ms.metadata
               FROM monitors m
               LEFT JOIN measurements ms ON ms.id = (
                   SELECT id FROM measurements WHERE monitor_id = m.id ORDER BY ts DESC LIMIT 1
               )
               WHERE m.type = ? AND m.enabled = 1
               ORDER BY m.display_order, m.name""",
            (monitor_type,),
        )
        return [dict(r) for r in await cursor.fetchall()]


async def build_websites_payload() -> dict:
    rows = await _get_latest_status("http")
    sites = []
    for r in rows:
        sites.append({
            "name": r["name"],
            "status": r["value_text"] if r["value_text"] in ("up", "down") else "unknown",
            "ms": int(r["value_num"]) if r["value_num"] else None,
        })
    return {"sites": sites}


async def build_network_payload() -> dict:
    rows = await _get_latest_status("snmp")
    devices = []
    for r in rows:
        meta = json.loads(r["metadata"]) if r["metadata"] else {}
        clients = 0
        try:
            clients = int(meta.get("wireless_clients", r["value_num"] or 0))
        except (ValueError, TypeError):
            pass

        devices.append({
            "name": r["name"],
            "status": r["value_text"] if r["value_text"] in ("online", "offline") else "unknown",
            "clients": clients,
            "bw_mbps": 0,  # bandwidth tracking requires delta calculation over time
        })
    return {"devices": devices}


async def build_cameras_payload() -> dict:
    rows = await _get_latest_status("rtsp")
    cameras = []
    for r in rows:
        meta = json.loads(r["metadata"]) if r["metadata"] else {}
        cam_id = str(r["id"])
        thumb = get_thumbnail_rgb565_b64(cam_id)

        cameras.append({
            "name": r["name"],
            "status": r["value_text"] if r["value_text"] in ("online", "offline", "ok") else "unknown",
            "thumb_rgb565": thumb,
            "w": meta.get("thumb_w", 120),
            "h": meta.get("thumb_h", 90),
        })
    return {"cameras": cameras}


async def build_chart_payload(monitor_id: int | None = None, points: int = 60) -> dict:
    """Build chart data from recent measurements."""
    async with get_db() as db:
        if monitor_id is None:
            # Use first HTTP monitor by default
            cursor = await db.execute(
                "SELECT id FROM monitors WHERE type = 'http' AND enabled = 1 ORDER BY display_order LIMIT 1"
            )
            row = await cursor.fetchone()
            if not row:
                return {"labels": [], "series": []}
            monitor_id = row["id"]

        cursor = await db.execute(
            "SELECT ts, value_num FROM measurements WHERE monitor_id = ? AND value_num IS NOT NULL ORDER BY ts DESC LIMIT ?",
            (monitor_id, points),
        )
        rows = await cursor.fetchall()

    rows = list(reversed(rows))
    labels = list(range(len(rows)))
    series = [r["value_num"] for r in rows]

    return {"labels": labels, "series": series}


async def build_overview_payload() -> dict:
    websites = await build_websites_payload()
    network = await build_network_payload()
    cameras = await build_cameras_payload()

    w_up = sum(1 for s in websites["sites"] if s["status"] == "up")
    d_online = sum(1 for d in network["devices"] if d["status"] == "online")
    c_online = sum(1 for c in cameras["cameras"] if c["status"] in ("online", "ok"))
    total_clients = sum(d.get("clients", 0) for d in network["devices"])
    avg_ms = 0
    ms_vals = [s["ms"] for s in websites["sites"] if s["ms"] is not None]
    if ms_vals:
        avg_ms = sum(ms_vals) / len(ms_vals)

    return {
        "websites_up": w_up,
        "websites_total": len(websites["sites"]),
        "devices_online": d_online,
        "devices_total": len(network["devices"]),
        "cameras_online": c_online,
        "cameras_total": len(cameras["cameras"]),
        "total_clients": total_clients,
        "avg_response_ms": round(avg_ms),
    }


async def build_full_update() -> list[dict]:
    """Build all page updates for ESP initial sync."""
    ts = int(time.time())
    messages = []

    messages.append({"type": "update", "page": "websites", "ts": ts, "data": await build_websites_payload()})
    messages.append({"type": "update", "page": "network", "ts": ts, "data": await build_network_payload()})
    messages.append({"type": "update", "page": "cameras", "ts": ts, "data": await build_cameras_payload()})
    messages.append({"type": "chart_data", "page": "bandwidth", "ts": ts, "data": await build_chart_payload()})

    return messages
