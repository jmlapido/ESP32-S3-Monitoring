import asyncio
from .base import BaseCollector, CollectResult

# Common OID presets
UBIQUITI_PRESETS = {
    "if_status": "1.3.6.1.2.1.2.2.1.8.1",       # ifOperStatus
    "wireless_clients": "1.3.6.1.4.1.41112.1.4.5.1.15.1",  # ubnt wireless stations
    "tx_bytes": "1.3.6.1.2.1.2.2.1.16.1",         # ifOutOctets
    "rx_bytes": "1.3.6.1.2.1.2.2.1.10.1",         # ifInOctets
    "sys_name": "1.3.6.1.2.1.1.5.0",
}

MIKROTIK_PRESETS = {
    "if_status": "1.3.6.1.2.1.2.2.1.8.1",
    "wireless_clients": "1.3.6.1.4.1.14988.1.1.1.3.1.6.1",  # mtxrWlApClientCount
    "tx_bytes": "1.3.6.1.2.1.2.2.1.16.1",
    "rx_bytes": "1.3.6.1.2.1.2.2.1.10.1",
    "sys_name": "1.3.6.1.2.1.1.5.0",
}


class SNMPCollector(BaseCollector):
    async def collect(self) -> CollectResult:
        community = self.config.get("community", "public")
        vendor = self.config.get("vendor", "generic")
        oids = self.config.get("oids", {})
        port = self.config.get("port", 161)

        # Select presets based on vendor
        if vendor == "ubiquiti" and not oids:
            oids = UBIQUITI_PRESETS
        elif vendor == "mikrotik" and not oids:
            oids = MIKROTIK_PRESETS
        elif not oids:
            oids = {"sys_name": "1.3.6.1.2.1.1.5.0", "if_status": "1.3.6.1.2.1.2.2.1.8.1"}

        try:
            from pysnmp.hlapi.v3arch.asyncio import (
                get_cmd,
                SnmpEngine,
                CommunityData,
                UdpTransportTarget,
                ContextData,
                ObjectType,
                ObjectIdentity,
            )

            results = {}
            engine = SnmpEngine()

            for name, oid in oids.items():
                error_indication, error_status, error_index, var_binds = await get_cmd(
                    engine,
                    CommunityData(community),
                    await UdpTransportTarget.create((self.target, port)),
                    ContextData(),
                    ObjectType(ObjectIdentity(oid)),
                )

                if error_indication:
                    results[name] = f"error: {error_indication}"
                elif error_status:
                    results[name] = f"error: {error_status.prettyPrint()}"
                else:
                    for var_bind in var_binds:
                        results[name] = str(var_bind[1])

            # Determine status
            if_status = results.get("if_status", "")
            is_online = if_status in ("1", "up(1)")
            clients = 0
            try:
                clients = int(results.get("wireless_clients", "0"))
            except (ValueError, TypeError):
                pass

            return CollectResult(
                status="online" if is_online else "offline",
                value_num=float(clients),
                value_text=f"clients={clients}",
                metadata=results,
            )

        except ImportError:
            return CollectResult(
                status="error",
                value_text="pysnmp not installed",
                metadata={"error": "pysnmp-lextudio not available"},
            )
        except Exception as e:
            return CollectResult(
                status="offline",
                value_num=None,
                value_text="error",
                metadata={"error": str(e)[:200]},
            )
