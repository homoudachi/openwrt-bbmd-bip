# Handover Prompt — OpenWrt BACnet/SC BBMD Project

**Repo**: https://github.com/homoudachi/openwrt-bbmd-bip (`main` branch, commit `f446955`)
**Working dir**: `/home/matt/opencode/openwrt-bbmd-bip`

## What's Done (Phase 1 Complete)

- **FSD** (`docs/fsd.md`) — Full functional spec: hub, node, telemetry, bridge, cert management, UCI schema
- **5 ADRs** (`docs/adr/`) — bacnet-stack selection, WebSocket/TLS, package structure, cert management, router-as-multi-role
- **Roadmap** (`docs/roadmap.md`) — 8 phases, 50+ tasks
- **8 GitHub issues** (#1-#8, one per phase, labeled `epic,ready-for-agent`)
- **AGENTS.md** with agent skills block + `docs/agents/` config
- **Makefile** + **Config.in** — OpenWrt package with `bbmd-openssl`/`bbmd-mbedtls` variants, CMake build
- **`files/bbmd.init`** — procd init script reading UCI, respawn, reload trigger
- **`files/bbmd.config`** — Default UCI config matching FSD schema
- **`files/cert-gen.sh`** — Certificate generation (CA, hub, node via openssl, ECDSA P-256)
- **`src/CMakeLists.txt`** — CMake build with bacnet-stack submodule, libwebsockets, dual TLS backend
- **`src/bacnet/`** — bacnet-stack v1.5.0 as git submodule (commit `61a7a53be`)
- **`src/bbmd.c`** — Daemon entry point: `-f` foreground, `-c` config path, daemonize, SIGTERM/SIGHUP, syslog, event loop placeholder (`usleep(100ms)`)
- **`src/bbmd_config.c/h`** — UCI config reader via libubox, reads all sections into `bbmd_config_t`, strdup strings, failover hub list
- **`src/bbmd_*.c` stubs** — `bbmd_hub.c`, `bbmd_node.c`, `bbmd_telemetry.c`, `bbmd_bridge.c` (empty, return 0)
- **`.github/workflows/build.yml`** — CI: native build check (openssl+mbedtls), test placeholder, cppcheck lint

## Current State

- Phase 1 issue #1 **closed**
- Issues #2-#8 open, labeled `epic,ready-for-agent`
- All source compiles as stubs (no real functionality yet)
- bacnet-stack submodule loaded, CMake configured correctly for BSC

## Next: Phase 2 — BACnet/SC Hub Mode (Issue #2)

**Goal**: Working SC hub that accepts WebSocket connections with TLS, handles NVCP, routes messages, and does keepalive.

The bacnet-stack BSC API has two approaches:

**Option A** — Direct hub API (lightweight, pure hub):
```c
bsc_hub_function_start(ca_cert, ca_cert_len, cert, cert_len, key, key_len,
    port, iface, &uuid, &vmac, max_bvlc, max_npdu,
    connect_timeout, heartbeat_timeout, disconnect_timeout,
    event_callback, user_arg, &hub_handle);
// then periodically: bsc_socket_maintenance_timer(seconds)
```

**Option B** — Full `dlenv_init()` / `bsc_init()` path (gives BACnet device object):
- Uses `BACNET_SC_ISSUER_1_CERTIFICATE_FILE` etc. env vars for cert paths
- Uses `BACNET_SC_HUB_FUNCTION_BINDING` env var for hub port binding
- Calls `dlenv_init()` → `bsc_init()` → `bsc_node_init()` with `hub_function_enabled=true`
- Main loop: `datalink_receive()` + `npdu_handler()` + periodic `datalink_maintenance_timer()`
- Example: `src/bacnet/apps/sc-hub/main.c`

**Recommendation**: Option B (full stack) because we need BACnet device object for telemetry (Phase 3).

## Key Files for Phase 2

- `src/bbmd_hub.c/h` — Hub orchestrator (overwrite stubs). Must:
  - Load certs from UCI paths (via `bbmd_config_t.hub.tls_cert`, etc.)
  - Set env vars for bacnet-stack cert/file paths
  - Call `Device_Set_Object_Instance_Number()` and `Device_Object_Name_ANSI_Init()`
  - Call `dlenv_init()` with hub binding
  - Run event loop: `datalink_receive()` → `npdu_handler()` + `datalink_maintenance_timer()`
  - Hook into `bbmd_config_t` from Phase 1 config reader

- `src/bbmd.c` — Wire `bbmd_hub_init()` / `bbmd_hub_run()` / `bbmd_hub_stop()` into main event loop, replacing `usleep(100ms)` placeholder

- `src/CMakeLists.txt` — May need to verify bacnet-stack BSC sources are compiled (should already with `BACDL_BSC=ON`)

## Critical bacnet-stack Headers

- `src/bacnet/src/bacnet/datalink/bsc/bsc-node.h` — `BSC_NODE_CONF`, `bsc_node_init/start/stop/send`, `bsc_node_maintenance_timer()`
- `src/bacnet/src/bacnet/datalink/bsc/bsc-hub-function.h` — `bsc_hub_function_start/stop/started/stopped`
- `src/bacnet/src/bacnet/datalink/bsc/bsc-datalink.h` — `bsc_init()`, `bsc_send_pdu()`, `bsc_receive()`, `bsc_maintenance_timer()`
- `src/bacnet/src/bacnet/datalink/bsc/bsc-socket.h` — `bsc_socket_maintenance_timer()`, `BSC_CONTEXT_CFG`
- `src/bacnet/src/bacnet/datalink/bsc/bsc-conf.h` — Buffer/timeout defaults

## Build/Test Commands

```bash
# Native build test
cmake -B build -S src -DBBMD_TLS_BACKEND=openssl && cmake --build build
```

## Load These Skills

- `subagent-driven-development` — For dispatching implementer/reviewer subagents
- `tdd` — Write tests alongside implementation
