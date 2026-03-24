import json
import time
from ..database import get_db
from ..config import settings


async def store_measurement(monitor_id: int, value_num: float | None, value_text: str, metadata: dict):
    async with get_db() as db:
        await db.execute(
            "INSERT INTO measurements (monitor_id, ts, value_num, value_text, metadata) VALUES (?, ?, ?, ?, ?)",
            (monitor_id, int(time.time()), value_num, value_text, json.dumps(metadata)),
        )
        await db.commit()


async def get_measurements(monitor_id: int, start: int, end: int, resolution: str = "raw") -> list[dict]:
    async with get_db() as db:
        if resolution == "hourly":
            cursor = await db.execute(
                "SELECT hour_ts as ts, avg_val, min_val, max_val, sample_count FROM measurements_hourly WHERE monitor_id = ? AND hour_ts BETWEEN ? AND ? ORDER BY hour_ts",
                (monitor_id, start, end),
            )
            rows = await cursor.fetchall()
            return [
                {"ts": r["ts"], "avg": r["avg_val"], "min": r["min_val"], "max": r["max_val"], "count": r["sample_count"]}
                for r in rows
            ]
        else:
            cursor = await db.execute(
                "SELECT ts, value_num, value_text FROM measurements WHERE monitor_id = ? AND ts BETWEEN ? AND ? ORDER BY ts",
                (monitor_id, start, end),
            )
            rows = await cursor.fetchall()
            return [{"ts": r["ts"], "value": r["value_num"], "text": r["value_text"]} for r in rows]


async def get_summary(monitor_id: int, start: int, end: int) -> dict:
    async with get_db() as db:
        cursor = await db.execute(
            """SELECT
                COUNT(*) as total,
                AVG(value_num) as avg_val,
                MIN(value_num) as min_val,
                MAX(value_num) as max_val,
                SUM(CASE WHEN value_text IN ('up', 'online', 'ok') THEN 1 ELSE 0 END) as up_count
            FROM measurements WHERE monitor_id = ? AND ts BETWEEN ? AND ?""",
            (monitor_id, start, end),
        )
        row = await cursor.fetchone()
        total = row["total"] or 0
        up = row["up_count"] or 0
        return {
            "monitor_id": monitor_id,
            "period_start": start,
            "period_end": end,
            "total_checks": total,
            "avg_value": round(row["avg_val"] or 0, 2),
            "min_value": row["min_val"],
            "max_value": row["max_val"],
            "uptime_pct": round((up / total * 100) if total > 0 else 0, 2),
        }


async def aggregate_hourly():
    """Roll up raw measurements into hourly aggregates."""
    async with get_db() as db:
        now = int(time.time())
        hour_ago = now - 3600

        await db.execute(
            """INSERT OR REPLACE INTO measurements_hourly (monitor_id, hour_ts, avg_val, min_val, max_val, sample_count)
            SELECT monitor_id,
                   (ts / 3600) * 3600 as hour_ts,
                   AVG(value_num),
                   MIN(value_num),
                   MAX(value_num),
                   COUNT(*)
            FROM measurements
            WHERE ts < ? AND value_num IS NOT NULL
            GROUP BY monitor_id, hour_ts""",
            (hour_ago,),
        )
        await db.commit()


async def prune_old_data() -> int:
    """Delete old raw measurements and aged hourly data."""
    async with get_db() as db:
        cutoff_raw = int(time.time()) - (settings.DATA_RETENTION_DAYS * 86400)
        cutoff_hourly = int(time.time()) - (settings.HOURLY_RETENTION_DAYS * 86400)

        cursor = await db.execute("DELETE FROM measurements WHERE ts < ?", (cutoff_raw,))
        raw_deleted = cursor.rowcount
        cursor = await db.execute("DELETE FROM measurements_hourly WHERE hour_ts < ?", (cutoff_hourly,))
        hourly_deleted = cursor.rowcount
        await db.commit()

        return raw_deleted + hourly_deleted
