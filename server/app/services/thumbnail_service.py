# In-memory thumbnail cache
_thumbnails: dict[str, dict] = {}


def store_thumbnail(camera_id: str, jpeg_bytes: bytes, rgb565_b64: str):
    _thumbnails[camera_id] = {
        "jpeg": jpeg_bytes,
        "rgb565_b64": rgb565_b64,
    }


def get_thumbnail_jpeg(camera_id: str) -> bytes | None:
    entry = _thumbnails.get(camera_id)
    return entry["jpeg"] if entry else None


def get_thumbnail_rgb565_b64(camera_id: str) -> str | None:
    entry = _thumbnails.get(camera_id)
    return entry["rgb565_b64"] if entry else None
