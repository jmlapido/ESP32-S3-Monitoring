import httpx
import time
from .base import BaseCollector, CollectResult


class HTTPCollector(BaseCollector):
    async def collect(self) -> CollectResult:
        timeout = self.config.get("timeout", 10)
        expected_status = self.config.get("expected_status", 200)
        verify_ssl = self.config.get("verify_ssl", True)

        try:
            async with httpx.AsyncClient(verify=verify_ssl, timeout=timeout) as client:
                start = time.monotonic()
                response = await client.get(self.target)
                elapsed_ms = (time.monotonic() - start) * 1000

                is_up = response.status_code == expected_status

                return CollectResult(
                    status="up" if is_up else "down",
                    value_num=round(elapsed_ms, 1),
                    value_text=f"{response.status_code}",
                    metadata={
                        "status_code": response.status_code,
                        "response_ms": round(elapsed_ms, 1),
                    },
                )

        except httpx.TimeoutException:
            return CollectResult(
                status="down",
                value_num=None,
                value_text="timeout",
                metadata={"error": "timeout"},
            )
        except httpx.ConnectError as e:
            return CollectResult(
                status="down",
                value_num=None,
                value_text="connect_error",
                metadata={"error": str(e)[:200]},
            )
        except Exception as e:
            return CollectResult(
                status="down",
                value_num=None,
                value_text="error",
                metadata={"error": str(e)[:200]},
            )
