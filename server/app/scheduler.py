import json
import logging
from apscheduler.schedulers.asyncio import AsyncIOScheduler
from apscheduler.triggers.interval import IntervalTrigger
from .database import get_db
from .collectors.http_collector import HTTPCollector
from .collectors.snmp_collector import SNMPCollector
from .collectors.rtsp_collector import RTSPCollector
from .services.data_service import aggregate_hourly, prune_old_data
from .services.display_service import (
    build_websites_payload,
    build_network_payload,
    build_cameras_payload,
    build_chart_payload,
)
from .ws.esp_handler import esp_manager
from .ws.web_handler import web_manager

logger = logging.getLogger("mondash.scheduler")

_scheduler: AsyncIOScheduler | None = None

COLLECTOR_MAP = {
    "http": HTTPCollector,
    "snmp": SNMPCollector,
    "rtsp": RTSPCollector,
}


async def _run_monitor(monitor_id: int):
    """Execute a single monitor check and broadcast results. Reads config fresh from DB."""
    async with get_db() as db:
        cursor = await db.execute(
            "SELECT type, target, config FROM monitors WHERE id = ? AND enabled = 1",
            (monitor_id,),
        )
        row = await cursor.fetchone()
    if not row:
        return

    monitor_type = row["type"]
    target = row["target"]
    config = json.loads(row["config"]) if row["config"] else {}

    cls = COLLECTOR_MAP.get(monitor_type)
    if not cls:
        return

    collector = cls(monitor_id, target, config)
    result = await collector.run()

    # Build and broadcast appropriate page update
    if monitor_type == "http":
        data = await build_websites_payload()
        await esp_manager.broadcast_update("websites", data)
        await web_manager.broadcast({"type": "update", "page": "websites", "data": data})
    elif monitor_type == "snmp":
        data = await build_network_payload()
        await esp_manager.broadcast_update("network", data)
        await web_manager.broadcast({"type": "update", "page": "network", "data": data})
    elif monitor_type == "rtsp":
        data = await build_cameras_payload()
        await esp_manager.broadcast_update("cameras", data)
        await web_manager.broadcast({"type": "update", "page": "cameras", "data": data})


async def _maintenance():
    """Hourly aggregation and pruning."""
    await aggregate_hourly()
    await prune_old_data()
    logger.info("Maintenance complete: aggregated hourly data, pruned old records")


async def schedule_all_monitors():
    """Load all enabled monitors and schedule them."""
    global _scheduler
    if not _scheduler:
        return

    # Remove existing monitor jobs
    for job in _scheduler.get_jobs():
        if job.id.startswith("monitor_"):
            _scheduler.remove_job(job.id)

    async with get_db() as db:
        cursor = await db.execute(
            "SELECT id, type, name, target, config, poll_interval_sec FROM monitors WHERE enabled = 1"
        )
        rows = await cursor.fetchall()

    for row in rows:
        job_id = f"monitor_{row['id']}"
        _scheduler.add_job(
            _run_monitor,
            trigger=IntervalTrigger(seconds=row["poll_interval_sec"]),
            id=job_id,
            args=[row["id"]],
            replace_existing=True,
            max_instances=1,
        )
        logger.info(f"Scheduled {row['type']} monitor '{row['name']}' every {row['poll_interval_sec']}s")


async def init_scheduler():
    global _scheduler
    _scheduler = AsyncIOScheduler()

    # Maintenance job: run hourly
    _scheduler.add_job(
        _maintenance,
        trigger=IntervalTrigger(hours=1),
        id="maintenance",
        replace_existing=True,
    )

    _scheduler.start()
    logger.info("Scheduler started")

    # Schedule all monitors
    await schedule_all_monitors()


async def shutdown_scheduler():
    global _scheduler
    if _scheduler:
        _scheduler.shutdown(wait=False)
        _scheduler = None
        logger.info("Scheduler stopped")
