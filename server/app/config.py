from pydantic_settings import BaseSettings
from pathlib import Path


class Settings(BaseSettings):
    DATABASE_PATH: str = "mondash.db"
    HOST: str = "0.0.0.0"
    PORT: int = 8000
    CORS_ORIGINS: list[str] = ["*"]
    DEFAULT_HTTP_CHECK_INTERVAL: int = 60
    DEFAULT_SNMP_POLL_INTERVAL: int = 30
    DEFAULT_RTSP_GRAB_INTERVAL: int = 300
    DATA_RETENTION_DAYS: int = 7
    HOURLY_RETENTION_DAYS: int = 90

    model_config = {"env_file": ".env", "env_prefix": "MONDASH_"}


settings = Settings()
