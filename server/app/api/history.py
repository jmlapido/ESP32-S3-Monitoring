from fastapi import APIRouter, Query
from ..services.data_service import get_measurements, get_summary, prune_old_data
import time

router = APIRouter(prefix="/api/history", tags=["history"])


@router.get("/{monitor_id}/data")
async def get_data(
    monitor_id: int,
    start: int = Query(default=0, description="Start timestamp (unix)"),
    end: int = Query(default=0, description="End timestamp (unix)"),
    resolution: str = Query(default="raw", description="raw or hourly"),
):
    if end == 0:
        end = int(time.time())
    if start == 0:
        start = end - 3600  # default last hour

    data = await get_measurements(monitor_id, start, end, resolution)
    return {"monitor_id": monitor_id, "start": start, "end": end, "data": data}


@router.get("/{monitor_id}/summary")
async def get_monitor_summary(
    monitor_id: int,
    start: int = Query(default=0),
    end: int = Query(default=0),
):
    if end == 0:
        end = int(time.time())
    if start == 0:
        start = end - 86400  # default last 24h

    summary = await get_summary(monitor_id, start, end)
    return summary


@router.delete("/prune")
async def prune():
    count = await prune_old_data()
    return {"pruned_rows": count}
