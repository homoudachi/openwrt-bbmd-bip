# Roadmap — OpenWrt BACnet/SC BBMD Node/Hub

## Phase 1: Foundation — Package Skeleton (Week 1-2)

**Goal**: Buildable OpenWrt package with daemon skeleton that compiles and installs on a router.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 1.1 | Create OpenWrt package Makefile with CMake build | `Makefile` + `Config.in` | ⬜ |
| 1.2 | Create procd init script | `files/bbmd.init` | ⬜ |
| 1.3 | Create UCI config schema and defaults | `files/bbmd.config` | ⬜ |
| 1.4 | Create main.c daemon skeleton (arg parsing, signal handling) | `src/bbmd.c` | ⬜ |
| 1.5 | Integrate bacnet-stack as git submodule, configure CMake | `src/CMakeLists.txt` + `src/bacnet/` | ⬜ |
| 1.6 | Implement UCI config reader module | `src/bbmd_config.c/h` | ⬜ |
| 1.7 | CI: GitHub Actions build for x86_64 + mips_24kc + aarch64_cortex-a53 | `.github/workflows/build.yml` | ⬜ |

## Phase 2: BACnet/SC Hub Mode (Week 2-3)

**Goal**: Working SC hub that accepts WebSocket connections, handles keepalive, and routes messages.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 2.1 | Implement WebSocket server startup with TLS (using bacnet-stack BSC) | `src/bbmd_hub.c/h` | ⬜ |
| 2.2 | Implement NVCP HUB_CONNECT handshake processing | Hub registration logic | ⬜ |
| 2.3 | Implement connection table (add/remove/lookup) | Connection table module | ⬜ |
| 2.4 | Implement DATA frame routing (unicast + broadcast) | Message forwarding | ⬜ |
| 2.5 | Implement keepalive handling (timeout detection) | Keepalive module | ⬜ |
| 2.6 | Implement certificate validation on connect | Cert validation | ⬜ |
| 2.7 | Implement connection stats (nodes connected, messages routed) | Stats tracking | ⬜ |
| 2.8 | Integration test: two mock nodes connect, exchange data | Test script + mock node | ⬜ |

## Phase 3: Router Telemetry BACnet Objects (Week 3)

**Goal**: The router appears as a BACnet device with router telemetry objects.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 3.1 | Implement BACnet Device Object (vendor, firmware, etc.) | Device object init | ⬜ |
| 3.2 | Implement telemetry polling engine (CPU, memory, uptime) | `src/bbmd_telemetry.c/h` | ⬜ |
| 3.3 | Implement telemetry BACnet objects (Analog Input, Binary Input) | Object creation + update loop | ⬜ |
| 3.4 | Support BACnet Who-Is/I-Am discovery | Discovery service | ⬜ |
| 3.5 | Support ReadProperty service for telemetry objects | ReadProperty handler | ⬜ |
| 3.6 | Support ReadPropertyMultiple service | ReadPropertyMultiple handler | ⬜ |
| 3.7 | Integration test: BACnet client discovers router and reads CPU/memory | Test with YABE or similar | ⬜ |

## Phase 4: BACnet/SC Node Mode (Week 3-4)

**Goal**: Router can connect to an external BACnet/SC hub as a client node.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 4.1 | Implement WebSocket client connect to external hub | `src/bbmd_node.c/h` | ⬜ |
| 4.2 | Implement HUB_CONNECT frame sending | Host registration | ⬜ |
| 4.3 | Implement primary + failover hub list with exponential backoff | Failover logic | ⬜ |
| 4.4 | Implement keepalive sending (configurable interval) | Periodic keepalive | ⬜ |
| 4.5 | Telemetry objects reachable via external hub connection | Already done in Phase 3 | ⬜ |
| 4.6 | Integration test: router connects to external hub, telemetry visible | Test with external hub | ⬜ |

## Phase 5: BACnet/IP BBMD Bridge (Week 4)

**Goal**: Bridge legacy BACnet/IP devices to the BACnet/SC network.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 5.1 | Implement BACnet/IP UDP listener (port 0xBAC0) | `src/bbmd_bridge.c/h` | ⬜ |
| 5.2 | Implement BVLL frame decode/encode | BVLL handling | ⬜ |
| 5.3 | Implement NPDU routing: BIP → SC and SC → BIP | Route table + forwarding | ⬜ |
| 5.4 | BBMD foreign device registration support | Foreign device table | ⬜ |
| 5.5 | Integration test: BACnet/IP device communicates with SC node | End-to-end test | ⬜ |

## Phase 6: Certificate Management (Week 4-5)

**Goal**: Easy certificate generation and management.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 6.1 | Certificate generation shell script (CA, hub, node) | `files/cert-gen.sh` | ⬜ |
| 6.2 | Package openssl-util dependency for cert-gen | Makefile DEPENDS | ⬜ |
| 6.3 | Documentation: how to generate and deploy certs | `docs/certificates.md` | ⬜ |

## Phase 7: LuCI Web UI (Week 5-6)

**Goal**: Web interface for status monitoring and configuration.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 7.1 | Create luci-app-bbmd package Makefile | `luci/Makefile` | ⬜ |
| 7.2 | Status page: connected nodes, message stats, uptime | `luci/*/bbmd.js` | ⬜ |
| 7.3 | Configuration page: hub/node/cert/telemetry settings | LuCI form sections | ⬜ |
| 7.4 | Menu registration and ACL config | Menu JSON + ACL JSON | ⬜ |
| 7.5 | UCI defaults script for first-run | `luci/*/uci-defaults/` | ⬜ |

## Phase 8: Testing, Documentation, Release (Week 6)

**Goal**: Production-ready release.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 8.1 | Stress test: 50 concurrent SC connections | Test script + results | ⬜ |
| 8.2 | Binary size and memory profiling | Profiling report | ⬜ |
| 8.3 | Install and test on physical router (e.g., GL.iNet) | Test report | ⬜ |
| 8.4 | README, install guide, configuration guide | `README.md`, `docs/` | ⬜ |
| 8.5 | Tag v1.0.0 release | Git tag + GitHub release | ⬜ |
| 8.6 | Submit to openwrt/packages feed | PR to openwrt/packages | ⬜ |

---

## Timeline Summary

| Phase | Weeks | Scope |
|---|---|---|
| 1: Foundation | 1-2 | Package skeleton, build system, CI |
| 2: SC Hub | 2-3 | WebSocket server, TLS, message routing, keepalive |
| 3: Telemetry | 3 | BACnet objects for router stats |
| 4: SC Node | 3-4 | Client mode, failover, keepalive |
| 5: BIP Bridge | 4 | BACnet/IP ↔ SC routing |
| 6: Cert Mgmt | 4-5 | Certificate generation, docs |
| 7: LuCI UI | 5-6 | Web admin interface |
| 8: Testing/Release | 6 | Stress tests, physical tests, release |
