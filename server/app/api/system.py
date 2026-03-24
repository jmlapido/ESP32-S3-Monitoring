import time
import os
from fastapi import APIRouter
from ..ws.esp_handler import esp_manager
from ..ws.web_handler import web_manager
from ..config import settings

router = APIRouter(prefix="/api/system", tags=["system"])

_start_time = time.time()


@router.get("/status")
async def server_status():
    uptime = int(time.time() - _start_time)
    db_size = 0
    if os.path.exists(settings.DATABASE_PATH):
        db_size = os.path.getsize(settings.DATABASE_PATH)

    return {
        "uptime_seconds": uptime,
        "version": "0.1.0",
        "database_size_bytes": db_size,
        "esp_connected": esp_manager.is_connected,
        "web_clients": web_manager.count,
    }


@router.get("/esp/status")
async def esp_status():
    return {
        "connected": esp_manager.is_connected,
        "last_heartbeat": esp_manager.last_heartbeat,
        "device_info": esp_manager.device_info,
    }


@router.get("/config")
async def get_config():
    return {
        "host": settings.HOST,
        "port": settings.PORT,
        "http_check_interval": settings.DEFAULT_HTTP_CHECK_INTERVAL,
        "snmp_poll_interval": settings.DEFAULT_SNMP_POLL_INTERVAL,
        "rtsp_grab_interval": settings.DEFAULT_RTSP_GRAB_INTERVAL,
        "data_retention_days": settings.DATA_RETENTION_DAYS,
        "hourly_retention_days": settings.HOURLY_RETENTION_DAYS,
    }
