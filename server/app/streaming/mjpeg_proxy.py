import asyncio
import subprocess
from fastapi import APIRouter
from fastapi.responses import StreamingResponse, JSONResponse
from ..database import get_db

router = APIRouter(prefix="/stream", tags=["streaming"])

# Active FFmpeg processes per camera
_streams: dict[str, subprocess.Popen] = {}


def _start_ffmpeg(rtsp_uri: str, width: int = 240, height: int = 180, fps: int = 5) -> subprocess.Popen:
    """Start FFmpeg process that transcodes RTSP H.264 to MJPEG on stdout."""
    cmd = [
        "ffmpeg",
        "-rtsp_transport", "tcp",
        "-stimeout", "5000000",
        "-i", rtsp_uri,
        "-f", "mjpeg",
        "-q:v", "5",
        "-r", str(fps),
        "-s", f"{width}x{height}",
        "-an",
        "pipe:1",
    ]
    return subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)


async def _mjpeg_generator(camera_id: str):
    """Yields MJPEG frames as multipart HTTP response."""
    proc = _streams.get(camera_id)
    if not proc or proc.poll() is not None:
        return

    boundary = b"--frame\r\n"

    try:
        while True:
            # Read JPEG SOI marker (0xFFD8)
            header = b""
            buf = b""
            chunk = await asyncio.get_event_loop().run_in_executor(None, proc.stdout.read, 2)
            if not chunk or len(chunk) < 2:
                break

            if chunk[0:2] != b"\xff\xd8":
                # Scan for JPEG start
                buf = chunk
                while True:
                    byte = await asyncio.get_event_loop().run_in_executor(None, proc.stdout.read, 1)
                    if not byte:
                        return
                    buf += byte
                    idx = buf.find(b"\xff\xd8")
                    if idx >= 0:
                        chunk = buf[idx:]
                        break
                    if len(buf) > 100000:
                        return

            # Read until JPEG EOI (0xFFD9)
            frame = bytearray(chunk)
            while True:
                data = await asyncio.get_event_loop().run_in_executor(None, proc.stdout.read, 4096)
                if not data:
                    return
                frame.extend(data)
                if b"\xff\xd9" in data:
                    # Trim to EOI
                    eoi = frame.find(b"\xff\xd9")
                    frame = frame[:eoi + 2]
                    break

            yield (
                b"--frame\r\n"
                b"Content-Type: image/jpeg\r\n"
                b"Content-Length: " + str(len(frame)).encode() + b"\r\n\r\n"
                + bytes(frame) + b"\r\n"
            )
    finally:
        pass


@router.get("/{camera_id}")
async def stream_camera(camera_id: str):
    """MJPEG stream endpoint for a camera."""
    # Look up camera monitor
    async with get_db() as db:
        cursor = await db.execute(
            "SELECT id, target, config FROM monitors WHERE id = ? AND type = 'rtsp' AND enabled = 1",
            (camera_id,),
        )
        row = await cursor.fetchone()

    if not row:
        return JSONResponse({"error": "Camera not found"}, status_code=404)

    import json
    config = json.loads(row["config"]) if row["config"] else {}
    uri = row["target"]
    username = config.get("username", "")
    password = config.get("password", "")
    if username and password:
        uri = uri.replace("rtsp://", f"rtsp://{username}:{password}@")

    width = config.get("stream_width", 240)
    height = config.get("stream_height", 180)
    fps = config.get("stream_fps", 5)

    # Stop existing stream if any
    if camera_id in _streams:
        old = _streams[camera_id]
        if old.poll() is None:
            old.kill()

    proc = _start_ffmpeg(uri, width, height, fps)
    _streams[camera_id] = proc

    return StreamingResponse(
        _mjpeg_generator(camera_id),
        media_type="multipart/x-mixed-replace; boundary=frame",
    )


@router.delete("/{camera_id}")
async def stop_stream(camera_id: str):
    """Stop an active MJPEG stream."""
    if camera_id in _streams:
        proc = _streams.pop(camera_id)
        if proc.poll() is None:
            proc.kill()
        return {"stopped": True}
    return {"stopped": False, "message": "No active stream"}
