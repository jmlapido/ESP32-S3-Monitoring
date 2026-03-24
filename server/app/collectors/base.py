from dataclasses import dataclass, field
from abc import ABC, abstractmethod
import time
from ..services.data_service import store_measurement
from ..ws.esp_handler import esp_manager
from ..ws.web_handler import web_manager


@dataclass
class CollectResult:
    status: str = "unknown"  # up, down, online, offline, error
    value_num: float | None = None
    value_text: str = ""
    metadata: dict = field(default_factory=dict)


class BaseCollector(ABC):
    def __init__(self, monitor_id: int, target: str, config: dict):
        self.monitor_id = monitor_id
        self.target = target
        self.config = config

    @abstractmethod
    async def collect(self) -> CollectResult:
        """Run the collection and return a result."""
        ...

    async def run(self):
        """Collect, store, and broadcast the result."""
        result = await self.collect()

        # Store in database
        await store_measurement(
            self.monitor_id,
            result.value_num,
            result.value_text,
            result.metadata,
        )

        return result
