import json
from fastapi import APIRouter
from pydantic import BaseModel
from ..database import get_db
from ..services.display_service import build_full_update

router = APIRouter(prefix="/api/display", tags=["display"])


class PageConfig(BaseModel):
    page_index: int
    page_type: str
    title: str
    config: dict = {}


@router.get("/pages")
async def list_pages():
    async with get_db() as db:
        cursor = await db.execute(
            "SELECT * FROM display_config ORDER BY page_index"
        )
        rows = await cursor.fetchall()
        return [
            {
                "id": r["id"],
                "page_index": r["page_index"],
                "page_type": r["page_type"],
                "title": r["title"],
                "config": json.loads(r["config"]) if r["config"] else {},
            }
            for r in rows
        ]


@router.put("/pages")
async def update_pages(pages: list[PageConfig]):
    async with get_db() as db:
        await db.execute("DELETE FROM display_config")
        for p in pages:
            await db.execute(
                "INSERT INTO display_config (page_index, page_type, title, config) VALUES (?, ?, ?, ?)",
                (p.page_index, p.page_type, p.title, json.dumps(p.config)),
            )
        await db.commit()

    # Notify ESP of config change
    from ..ws.esp_handler import esp_manager
    page_names = [p.title.lower() for p in pages]
    await esp_manager.send_config(page_names)

    return {"updated": True, "count": len(pages)}


@router.get("/preview")
async def preview():
    """Returns the data exactly as the ESP would receive it."""
    return await build_full_update()
