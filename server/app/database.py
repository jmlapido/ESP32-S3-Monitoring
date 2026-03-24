import aiosqlite
import time
from contextlib import asynccontextmanager
from .config import settings

_db_path = settings.DATABASE_PATH

SCHEMA = """
CREATE TABLE IF NOT EXISTS monitors (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    type TEXT NOT NULL CHECK(type IN ('http', 'snmp', 'rtsp')),
    name TEXT NOT NULL,
    target TEXT NOT NULL,
    config TEXT DEFAULT '{}',
    poll_interval_sec INTEGER DEFAULT 60,
    enabled INTEGER DEFAULT 1,
    page_assignment TEXT DEFAULT 'auto',
    display_order INTEGER DEFAULT 0,
    created_at INTEGER DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER DEFAULT (strftime('%s', 'now'))
);

CREATE TABLE IF NOT EXISTS measurements (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    monitor_id INTEGER NOT NULL REFERENCES monitors(id) ON DELETE CASCADE,
    ts INTEGER NOT NULL,
    value_num REAL,
    value_text TEXT,
    metadata TEXT DEFAULT '{}'
);

CREATE INDEX IF NOT EXISTS idx_meas_monitor_ts ON measurements(monitor_id, ts);

CREATE TABLE IF NOT EXISTS measurements_hourly (
    monitor_id INTEGER NOT NULL,
    hour_ts INTEGER NOT NULL,
    avg_val REAL,
    min_val REAL,
    max_val REAL,
    sample_count INTEGER,
    PRIMARY KEY (monitor_id, hour_ts)
);

CREATE TABLE IF NOT EXISTS display_config (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    page_index INTEGER NOT NULL,
    page_type TEXT NOT NULL,
    title TEXT NOT NULL DEFAULT '',
    config TEXT DEFAULT '{}'
);
"""

DEFAULT_PAGES = [
    (0, "overview", "Overview", "{}"),
    (1, "status_list", "Websites", '{"monitor_type": "http"}'),
    (2, "status_list", "Network", '{"monitor_type": "snmp"}'),
    (3, "camera_grid", "Cameras", '{"monitor_type": "rtsp"}'),
    (4, "chart", "Bandwidth", '{"metric": "bandwidth"}'),
]


async def init_db():
    async with aiosqlite.connect(_db_path) as db:
        await db.execute("PRAGMA journal_mode=WAL")
        await db.execute("PRAGMA foreign_keys=ON")
        await db.executescript(SCHEMA)

        # Insert default display pages if empty
        cursor = await db.execute("SELECT COUNT(*) FROM display_config")
        row = await cursor.fetchone()
        if row[0] == 0:
            await db.executemany(
                "INSERT INTO display_config (page_index, page_type, title, config) VALUES (?, ?, ?, ?)",
                DEFAULT_PAGES,
            )

        await db.commit()


@asynccontextmanager
async def get_db():
    db = await aiosqlite.connect(_db_path)
    db.row_factory = aiosqlite.Row
    await db.execute("PRAGMA foreign_keys=ON")
    try:
        yield db
    finally:
        await db.close()
