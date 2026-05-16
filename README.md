# BBMD — BACnet/SC Hub, Node & BBMD Bridge for OpenWrt

[![Build](https://img.shields.io/badge/build-passing-brightgreen)]()
[![License](https://img.shields.io/badge/license-GPLv2-blue)]()
[![OpenWrt](https://img.shields.io/badge/platform-OpenWrt-orange)]()

Turn a commodity Wi-Fi router into a **BACnet/SC (Secure Connect) hub/node/bridge** for building automation networks. The first BACnet package ever for OpenWrt.

## Features

- **Hub Mode** — Accept up to 50 BACnet/SC node connections over WebSocket with TLS 1.3 mutual authentication, message routing, keepalive, and connection management.
- **Node Mode** — Connect to a remote BACnet/SC hub as a client node, with primary + failover hub list and automatic reconnection with exponential backoff.
- **Bridge Mode (BBMD)** — Route NPDU frames between BACnet/IP (UDP :47808) and BACnet/SC, with BBMD foreign device registration support.
- **Telemetry** — Expose router CPU load, memory usage, uptime, and network stats as BACnet Analog Input objects readable by any BACnet client.
- **LuCI Web UI** — Status dashboard and full configuration form accessible from the OpenWrt web interface.
- **Certificate Management** — Built-in `bbmd-cert-gen` script for generating CA, hub, and node X.509 certificates (ECDSA P-256 or RSA-2048).

## Architecture

```
┌──────────────────────────────────────────────────────────┐
│                    OpenWrt Router                        │
│                                                          │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────────┐  │
│  │ SC Hub      │  │ SC Node      │  │ BIP ↔ BSC      │  │
│  │ wss://:443  │  │ (Telemetry)  │  │ Bridge (BBMD)  │  │
│  │ msg routing │  │ CPU/RAM/Up   │  │ UDP :47808     │  │
│  │ 50 nodes    │  │ BACnet objs  │  │ FD support     │  │
│  └──────┬──────┘  └──────┬───────┘  └───────┬────────┘  │
│         │                │                    │          │
│         └────────────────┼────────────────────┘          │
│                          │                               │
│              ┌───────────▼─────────────┐                 │
│              │   BACnet Network Layer  │                 │
│              └─────────────────────────┘                 │
│                                                          │
│  Configuration: UCI (/etc/config/bbmd) + LuCI Web UI    │
└──────────────────────────────────────────────────────────┘

  LAN: BACnet/IP devices          WAN: Remote SC nodes
  UDP :47808                       wss://router/bacnet-sc
```

## Quick Start

```bash
# Install on an OpenWrt router
opkg update && opkg install bbmd-openssl

# Generate certificates
bbmd-cert-gen ca
bbmd-cert-gen hub

# Enable and start the daemon
/etc/init.d/bbmd enable
/etc/init.d/bbmd start
```

## Documentation

| Document | Description |
|---|---|
| [Install Guide](docs/install-guide.md) | Prerequisites, package installation, service setup |
| [Configuration Guide](docs/config-guide.md) | Full UCI schema, examples, mode restrictions |
| [Certificate Management](docs/certificates.md) | Certificate generation and deployment |
| [Functional Spec](docs/fsd.md) | Complete requirements and design |
| [Roadmap](docs/roadmap.md) | Implementation status and plans |

## BACnet Objects

When telemetry is enabled, the daemon exposes these BACnet objects:

| Object | Instance | Value | Update Rate |
|---|---|---|---|
| Device | 0 | Router identity, vendor, firmware | Static |
| Analog Input | 0 | CPU usage % | 10 s |
| Analog Input | 1 | Memory usage % | 10 s |
| Analog Input | 2 | System uptime (s) | 60 s |

## Building from Source

```bash
# Using OpenWrt SDK
make package/bbmd/compile V=s

# Two TLS variants (select with menuconfig):
#   bbmd-openssl   — OpenSSL backend (default)
#   bbmd-mbedtls   — mbedTLS backend
```

Dependencies: `libwebsockets-full`, `libopenssl` (or `libmbedtls`), `libubox`, `libblobmsg-json`, `libubus`, `openssl-util`.

## Supported Platforms

| Architecture | Status |
|---|---|
| mips_24kc (common MIPS routers) | Primary |
| aarch64_cortex-a53 (ARM64, e.g. GL.iNet) | Primary |
| x86_64 | Supported |
| mipsel_24kc | Supported |

## License

GNU General Public License v2.0 or later. See [LICENSE](LICENSE).

## Contributing

See the [issue tracker](https://github.com/homoudachi/openwrt-bbmd-bip/issues) for open tasks. Pull requests welcome.
