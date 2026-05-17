# Roadmap — OpenWrt BACnet/SC BBMD Node/Hub

## Phase 1: Foundation — Package Skeleton (Week 1-2)

**Goal**: Buildable OpenWrt package with daemon skeleton that compiles and installs on a router.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 1.1 | Create OpenWrt package Makefile with CMake build | `Makefile` + `Config.in` | ✅ |
| 1.2 | Create procd init script | `files/bbmd.init` | ✅ |
| 1.3 | Create UCI config schema and defaults | `files/bbmd.config` | ✅ |
| 1.4 | Create main.c daemon skeleton (arg parsing, signal handling) | `src/bbmd.c` | ✅ |
| 1.5 | Integrate bacnet-stack as git submodule, configure CMake | `src/CMakeLists.txt` + `src/bacnet/` | ✅ |
| 1.6 | Implement UCI config reader module | `src/bbmd_config.c/h` | ✅ |
| 1.7 | CI: GitHub Actions build for x86_64 + mips_24kc + aarch64_cortex-a53 | `.github/workflows/build.yml` | ✅ |

## Phase 2: BACnet/SC Hub Mode (Week 2-3)

**Goal**: Working SC hub that accepts WebSocket connections, handles keepalive, and routes messages.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 2.1 | Implement WebSocket server startup with TLS (using bacnet-stack BSC) | `src/bbmd_hub.c/h` | ✅ |
| 2.2 | Implement NVCP HUB_CONNECT handshake processing | Hub registration logic | ✅ |
| 2.3 | Implement connection table (add/remove/lookup) | Connection table module | ✅ |
| 2.4 | Implement DATA frame routing (unicast + broadcast) | Message forwarding | ✅ |
| 2.5 | Implement keepalive handling (timeout detection) | Keepalive module | ✅ |
| 2.6 | Implement certificate validation on connect | Cert validation | ✅ |
| 2.7 | Implement connection stats (nodes connected, messages routed) | Stats tracking | ✅ |
| 2.8 | Integration test: two mock nodes connect, exchange data | Test script + mock node | ✅ |

## Phase 3: Router Telemetry BACnet Objects (Week 3)

**Goal**: The router appears as a BACnet device with router telemetry objects.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 3.1 | Implement BACnet Device Object (vendor, firmware, etc.) | Device object init | ✅ `Device_Set_Vendor_Identifier`, `Device_Set_Model_Name`, `Device_Set_Firmware_Revision` in `bbmd_hub_init()` |
| 3.2 | Implement telemetry polling engine (CPU, memory, uptime) | `src/bbmd_telemetry.c/h` | ✅ reads `/proc/stat`, `/proc/meminfo`, `/proc/uptime` |
| 3.3 | Implement telemetry BACnet objects (Analog Input) | Object creation + update loop | ✅ 3× Analog Input (CPU%, Memory%, Uptime) via `Analog_Input_Create`, updated via `Analog_Input_Present_Value_Set` |
| 3.4 | Support BACnet Who-Is/I-Am discovery | Discovery service | ✅ `handler_who_is` registered, `Send_I_Am()` on startup |
| 3.5 | Support ReadProperty service for telemetry objects | ReadProperty handler | ✅ `handler_read_property` registered; bacnet-stack auto-serves created objects |
| 3.6 | Support ReadPropertyMultiple service | ReadPropertyMultiple handler | ✅ `handler_read_property_multiple` registered |
| 3.7 | Integration test: BACnet client discovers router and reads CPU/memory | Test with YABE or similar | ⏭️ skipped — needs BACnet client on target hardware |

## Phase 4: BACnet/SC Node Mode (Week 3-4) — **Complete**

**Goal**: Router can connect to an external BACnet/SC hub as a client node.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 4.1 | Implement WebSocket client connect to external hub | `src/bbmd_node.c/h` | ✅ bacnet-stack handles via `dlenv_init()` with node env vars |
| 4.2 | Implement HUB_CONNECT frame sending | Host registration | ✅ bacnet-stack handles internally |
| 4.3 | Implement primary + failover hub list with exponential backoff | Failover logic | ✅ `BACNET_SC_PRIMARY_HUB_URI`, `BACNET_SC_FAILOVER_HUB_URI`, `SC_NETPORT_RECONNECT_TIME=60` |
| 4.4 | Implement keepalive sending (configurable interval) | Periodic keepalive | ✅ bacnet-stack handles internally |
| 4.5 | Telemetry objects reachable via external hub connection | Already done in Phase 3 | ✅ same service handlers as hub mode |
| 4.6 | Integration test: router connects to external hub, telemetry visible | Test with external hub | ⏭️ skipped — needs BACnet client on target hardware |

## Phase 5: BACnet/IP BBMD Bridge (Week 4) — **Complete**

**Goal**: Bridge legacy BACnet/IP devices to the BACnet/SC network.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 5.1 | Implement BACnet/IP UDP listener (port 0xBAC0) | `src/bbmd_bridge.c/h` | ✅ `bip_init()` on `config->bridge.port`, BVLC init |
| 5.2 | Implement BVLL frame decode/encode | BVLL handling | ✅ bacnet-stack handles internally via `bip_receive()`/`bip_send_pdu()` |
| 5.3 | Implement NPDU routing: BIP → SC and SC → BIP | Route table + forwarding | ✅ Two-datalink router (BIP net + BSC net), `bridge_npdu_handler()` with hop_count |
| 5.4 | BBMD foreign device registration support | Foreign device table | ✅ `bvlc_init()` + `bvlc_maintenance_timer()`, bacnet-stack BBMD module |
| 5.5 | Integration test: BACnet/IP device communicates with SC node | End-to-end test | ⏭️ skipped — needs BACnet client on target hardware |

## Phase 6: Certificate Management (Week 4-5)

**Goal**: Easy certificate generation and management.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 6.1 | Certificate generation shell script (CA, hub, node) | `files/cert-gen.sh` | ✅ |
| 6.2 | Package openssl-util dependency for cert-gen | Makefile DEPENDS | ✅ |
| 6.3 | Documentation: how to generate and deploy certs | `docs/certificates.md` | ✅ |

## Phase 7: LuCI Web UI (Week 5-6)

**Goal**: Web interface for status monitoring and configuration.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 7.1 | Create luci-app-bbmd package Makefile | `luci/Makefile` | ✅ Standalone LuCI package; installs htdocs→/www, root→/ |
| 7.2 | Status page | `luci/htdocs/.../status.js` | ✅ PID check, device identity, active modes, system metrics (CPU/mem/uptime from /proc), hub config, connected nodes placeholder |
| 7.3 | Configuration page | `luci/htdocs/.../config.js` | ✅ form.Map with 6 TypedSections, conditional depends, DynamicList for failover hubs |
| 7.4 | Menu + ACL | `menu.d/` + `acl.d/` JSON | ✅ Services→BBMD (BACnet) submenu, view paths, rpcd UCI + /proc permissions |
| 7.5 | UCI defaults | `40_bbmd-luci` | ✅ Idempotent section creation, cert directory, exit 0 |

## Phase 8: Testing, Documentation, Release (Week 6)

**Goal**: Production-ready release.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 8.1 | Stress test: 50 concurrent SC connections | Test script + results | ⏭️ Ready — passwordless sudo available; QEMU image setup complete; LuCI-based testing VM running |
| 8.2 | Binary size and memory profiling | Profiling report | ✅ 1.1MB stripped (x86_64), ~12.6MB runtime, NFR-1 over (<500KB target, MIPS build expected smaller) |
| 8.3 | Install and test on physical router (e.g., GL.iNet) | Test report | ⏭️ Blocked — needs physical router |
| 8.4 | README, install guide, configuration guide | `README.md`, `docs/` | ✅ README.md, docs/install-guide.md, docs/config-guide.md |
| 8.5 | Tag v1.0.0 release | Git tag + GitHub release | ✅ v1.0.0 tagged (commit 9813642); includes -c flag fix, BACNET_FILE_PATH_RESTRICTED=0, MKP macro fix |
| 8.6 | Submit to openwrt/packages feed | PR to openwrt/packages | ⏭️ Blocked — needs external PR approval |

## Phase 10: BIP-Only BBMD Mode (Tailscale Foreign Device)

**Goal**: Standalone BIP/BBMD mode without SC/WebSocket/TLS dependency.  
**Use case**: Remote BACnet client registers as BBMD foreign device over Tailscale VPN, accesses BACnet/IP devices on the router's LAN.

### Architecture

```
Windows PC (Tailscale IP)  ──Tailscale VPN── OpenWrt Router (Tailscale IP + LAN IP)
  BACnet client                     │            bbmd bip mode: UDP 0.0.0.0:47808
  Foreign Device                    │            BBMD foreign device table
  ────────────────                  │            ┌──────────────────┐
  Register-FD ──────────────────────┤            │ bvlc: FD fwd     │
  Who-Is (via Dist-Bcast) ──────────┤            │ bip_receive()    │
                                    │            │ apdu_handler()   │
                                    │            └──────┬───────────┘
                                    │                   │ LAN (192.168.1.x)
                                    │            ┌──────┴───────────┐
                                    │            │ BIP Device       │
                                    │            │ BACnet/IP UDP    │
                                    │            └──────────────────┘
```

| # | Task | Deliverable | Status |
|---|---|---|---|
| 10.1 | Add `bip` UCI config structs and reader | `bbmd_config.h/c` — `bbmd_bip_config_t`, defaults, load/free | ⏳ |
| 10.2 | Implement BIP-only mode module | `src/bbmd_bip.c/h` — BIP datalink, BBMD via bvlc, service handlers, no TLS/SC | ⏳ |
| 10.3 | Integrate BIP mode into main loop + build | `src/bbmd.c` — start/stop/reload/maintenance; `CMakeLists.txt` — add source; mutex with hub/node/bridge | ⏳ |
| 10.4 | Update docs, UCI defaults, LuCI | `docs/config-guide.md`, `files/bbmd.config`, `luci/` uci-defaults + config.js | ⏳ |

### Tailscale / Foreign Device Design

- **Bind to all interfaces**: Don't set `BACNET_IFACE` (or set empty) so `bip_init()` binds `INADDR_ANY`. This makes the BBMD reachable on both `br-lan` (LAN devices) and `tailscale0` (remote foreign device).
- **Foreign device registration**: bacnet-stack's `bvlc` module records `src_addr` from `recvfrom()`. A remote client's Tailscale IP is recorded and forwarded to correctly.
- **Broadcast forwarding**: Local BACnet/IP broadcasts on LAN are forwarded to registered foreign devices via unicast `Forwarded-NPDU`. Foreign device `Distribute-Broadcast-To-Network` messages are forwarded to LAN as broadcasts.
- **No SC dependency**: No TLS certs, no WebSocket, no `BSC_Net` datalink. Pure UDP.
- **Mutual exclusion**: BIP mode cannot run alongside hub, node, or bridge (one BACnet datalink active at a time). Telemetry can coexist.
- **Keepalive**: Foreign devices must re-register periodically. `bvlc_maintenance_timer()` handles FDT expiry.

### Example Configuration

```
config bip 'bip'
    option enabled '1'
    option port '47808'
    # omit lan_interface to bind all interfaces (required for Tailscale)

config globals 'globals'
    option device_instance '1001'
    option device_name 'Router BBMD'
    option network_number '1'

config telemetry 'telemetry'
    option enabled '1'

config hub 'hub'
    option enabled '0'

config bbmd 'bbmd'
    option enabled '0'
```

---

## Phase 9: Bugfix & Virtualization (Week 6+)

**Goal**: Fix config duplication bug, establish QEMU-based testing workflow, profile MIPS binary size.

| # | Task | Deliverable | Status |
|---|---|---|---|
| 9.1 | Fix duplicate config sections in LuCI form (#9) | `luci/root/etc/uci-defaults/40_bbmd-luci` | ✅ Fixed in cf26303 — uci-defaults used `uci get bbmd.globals` (named section) instead of `uci get bbmd.@globals[0]` (type-indexed) |
| 9.2 | Set up QEMU-based LuCI testing VM | QEMU VM script + docs | ✅ OpenWrt 25.12.4 x86_64 booting with LuCI on port 8729; user-facing testing ready |
| 9.3 | Port convention added to AGENTS.md | `AGENTS.md` | ✅ 667c194 |
| 9.4 | Build for MIPS (mips_24kc) and profile binary size | Binary size report | ⏳ Next session |
| 9.5 | Stress testing in QEMU | Test results | ⏳ Next session |

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
| 9: Bugfix & Virtualization | 6+ | Config fix, QEMU testing, MIPS profiling |

---

## Current State — File Map (post-Phase 8)

| File | Purpose |
|---|---|
| `src/bbmd.c` | Daemon main loop: init hub/node/bridge modes, poll telemetry, SIGHUP reload |
| `src/bbmd_hub.c/h` | SC hub mode: WebSocket server, TLS, connection table, message routing |
| `src/bbmd_node.c/h` | SC node mode: mirrors hub pattern with node env vars (no HUB_FUNCTION_BINDING, sets PRIMARY_HUB_URI + FAILOVER_HUB_URI) |
| `src/bbmd_bridge.c/h` | BIP ↔ BSC bridge: two-datalink router, UDP listener + SC hub, NPDU routing, BBMD FD support |
| `src/bbmd_telemetry.c/h` | Telemetry polling: `/proc/stat`, `/proc/meminfo`, `/proc/uptime` → AI objects |
| `src/bbmd_config.c/h` | UCI config reader with defaults |
| `src/CMakeLists.txt` | CMake build (combines bbmd sources + bacnet-stack) |
| `Makefile` + `Config.in` | OpenWrt package definition |
| `files/bbmd.init` | procd init script |
| `files/bbmd.config` | UCI config schema / defaults |
| `files/cert-gen.sh` | X.509 certificate generation script (CA, hub, node) |
| `docs/certificates.md` | Certificate management documentation |
| `.github/workflows/build.yml` | CI: build for x86_64, mips_24kc, aarch64_cortex-a53 |
| `src/bacnet/` | bacnet-stack git submodule (upstream) |
| `luci/Makefile` | LuCI app package Makefile |
| `luci/htdocs/luci-static/resources/view/bbmd/status.js` | Status dashboard: daemon status, device identity, system metrics, connected nodes |
| `luci/htdocs/luci-static/resources/view/bbmd/config.js` | Configuration form: all 6 UCI sections (globals, hub, node, telemetry, bbmd, logging) |
| `luci/root/usr/share/rpcd/acl.d/luci-app-bbmd.json` | ACL permissions for UCI read/write and /proc file access |
| `luci/root/usr/share/luci/menu.d/luci-app-bbmd.json` | Menu registration under Services → BBMD (BACnet) |
| `luci/root/etc/uci-defaults/40_bbmd-luci` | First-boot UCI defaults and cert directory creation |
