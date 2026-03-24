import asyncio
import base64
import struct
from io import BytesIO
from .base import BaseCollector, CollectResult
from ..services.thumbnail_service import store_thumbnail


class RTSPCollector(BaseCollector):
    async def collect(self) -> CollectResult:
        username = self.config.get("username", "")
        password = self.config.get("password", "")
        thumb_w = self.config.get("thumb_width", 120)
        thumb_h = self.config.get("thumb_height", 90)

        uri = self.target
        if username and password:
            # Insert credentials into URI
            uri = uri.replace("rtsp://", f"rtsp://{username}:{password}@")

        try:
            import ffmpeg

            # Capture single frame from RTSP
            process = (
                ffmpeg
                .input(uri, rtsp_transport="tcp", stimeout="5000000")
                .output("pipe:", vframes=1, format="image2", vcodec="mjpeg", q=5)
                .overwrite_output()
                .run_async(pipe_stdout=True, pipe_stderr=True)
            )

            stdout, stderr = await asyncio.get_event_loop().run_in_executor(
                None, process.communicate
            )

            if process.returncode != 0 or not stdout:
                return CollectResult(
                    status="offline",
                    value_text="capture_failed",
                    metadata={"error": stderr.decode(errors="ignore")[:200]},
                )

            # Process with Pillow
            from PIL import Image

            img = Image.open(BytesIO(stdout))
            img = img.resize((thumb_w, thumb_h), Image.LANCZOS)

            # Save JPEG for web panel
            jpeg_buf = BytesIO()
            img.save(jpeg_buf, format="JPEG", quality=70)
            jpeg_bytes = jpeg_buf.getvalue()

            # Convert to RGB565 for ESP
            rgb565_bytes = bytearray()
            pixels = img.convert("RGB").load()
            for y in range(thumb_h):
                for x in range(thumb_w):
                    r, g, b = pixels[x, y]
                    rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                    rgb565_bytes.extend(struct.pack("<H", rgb565))

            rgb565_b64 = base64.b64encode(bytes(rgb565_bytes)).decode("ascii")

            # Store thumbnails
            camera_id = str(self.monitor_id)
            store_thumbnail(camera_id, jpeg_bytes, rgb565_b64)

            return CollectResult(
                status="online",
                value_num=len(jpeg_bytes),
                value_text="ok",
                metadata={"thumb_w": thumb_w, "thumb_h": thumb_h},
            )

        except ImportError:
            return CollectResult(
                status="error",
                value_text="ffmpeg-python not installed",
            )
        except Exception as e:
            return CollectResult(
                status="offline",
                value_text="error",
                metadata={"error": str(e)[:200]},
            )
