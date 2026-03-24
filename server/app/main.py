import uvicorn
from contextlib import asynccontextmanager
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.responses import RedirectResponse
from pathlib import Path

from .config import settings
from .database import init_db
from .scheduler import init_scheduler, shutdown_scheduler
from .api.monitors import router as monitors_router
from .api.display import router as display_router
from .api.history import router as history_router
from .api.system import router as system_router
from .ws.esp_handler import esp_websocket_endpoint
from .ws.web_handler import web_websocket_endpoint
from .streaming.mjpeg_proxy import router as stream_router


@asynccontextmanager
async def lifespan(app: FastAPI):
    await init_db()
    await init_scheduler()
    yield
    await shutdown_scheduler()


app = FastAPI(
    title="MonDash",
    version="0.1.0",
    description="ESP32-S3 Monitoring Dashboard Backend",
    lifespan=lifespan,
)

# CORS
app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.CORS_ORIGINS,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# API routers
app.include_router(monitors_router)
app.include_router(display_router)
app.include_router(history_router)
app.include_router(system_router)
app.include_router(stream_router)

# WebSocket endpoints
app.websocket("/ws/esp")(esp_websocket_endpoint)
app.websocket("/ws/web")(web_websocket_endpoint)

# Static files for web panel
# SvelteKit builds assets to /_app/immutable/... (absolute paths)
# so we must serve both /_app (assets) and /app (html entry)
static_dir = Path(__file__).parent.parent / "static"
if static_dir.exists():
    assets_dir = static_dir / "_app"
    if assets_dir.exists():
        app.mount("/_app", StaticFiles(directory=str(assets_dir)), name="sveltekit-assets")
    app.mount("/app", StaticFiles(directory=str(static_dir), html=True), name="web-panel")


@app.get("/")
async def root():
    return RedirectResponse(url="/app/")


@app.get("/api")
async def api_root():
    return RedirectResponse(url="/docs")


@app.get("/health")
async def health():
    return {"status": "ok", "service": "mondash"}


def run():
    uvicorn.run(
        "app.main:app",
        host=settings.HOST,
        port=settings.PORT,
        reload=False,
    )


if __name__ == "__main__":
    run()
