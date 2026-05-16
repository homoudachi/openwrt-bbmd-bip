# Project Context — OpenWrt BACnet/SC BBMD

OpenWrt binary package (`bbmd`) that transforms a Wi-Fi router into a BACnet/SC (Secure Connect) node/hub for building automation networks.

- **Language**: C (bacnet-stack + custom code)
- **Build**: CMake via OpenWrt package SDK
- **Dependencies**: bacnet-stack, libwebsockets, OpenSSL/mbedTLS, libubox
- **Target platforms**: mips_24kc, aarch64_cortex-a53, x86_64

## Current Phase

**Phase 8 complete** — All non-blocked tasks finished. v1.0.0 tagged. Remaining blockers: stress test (8.1), physical router install (8.3), openwrt/packages PR (8.6).

## Key Files

| File | Purpose |
|---|---|
| `src/bbmd.c` | Main loop: hub/node/bridge lifecycle, telemetry timer, SIGHUP reload; enforces mode exclusivity |
| `src/bbmd_hub.c/h` | BACnet/SC Hub Mode: env vars, Device Object, service handlers, dlenv_init, event loop |
| `src/bbmd_node.c/h` | BACnet/SC Node Mode: mirrors hub pattern; sets PRIMARY_HUB_URI/FAILOVER_HUB_URI, NO hub binding |
| `src/bbmd_bridge.c/h` | BACnet/IP ↔ BACnet/SC Bridge: two-datalink router, BIP UDP + BSC hub, NPDU routing, BBMD FD support |
| `src/bbmd_telemetry.c/h` | Telemetry polling: /proc/stat, /proc/meminfo, /proc/uptime → BACnet Analog Input objects |
| `src/bbmd_config.c/h` | UCI config reader (globals, hub, node, telemetry, bridge, logging) |
| `src/CMakeLists.txt` | CMake build with bacnet-stack submodule |
| `Makefile` | OpenWrt package build definition |
| `luci/` | LuCI web interface (Phase 7 - complete) |
| `luci/Makefile` | LuCI app package Makefile (standalone, installs htdocs to /www and root to /) |
| `luci/htdocs/luci-static/resources/view/bbmd/status.js` | Status dashboard: daemon PID check, device identity (UCI), active modes, CPU/memory/uptime (/proc) |
| `luci/htdocs/luci-static/resources/view/bbmd/config.js` | Configuration form: 6 TypedSections (globals, hub, node, telemetry, bbmd, logging) with conditional depends |
| `luci/root/usr/share/rpcd/acl.d/luci-app-bbmd.json` | ACL: UCI read/write permissions + /proc file access |
| `luci/root/usr/share/luci/menu.d/luci-app-bbmd.json` | Menu: Services → BBMD (BACnet) with Status and Configuration views |
| `luci/root/etc/uci-defaults/40_bbmd-luci` | First-boot UCI defaults + cert directory creation |
| `files/cert-gen.sh` | Certificate generation script (installed to `/usr/sbin/bbmd-cert-gen`) |
| `docs/certificates.md` | Certificate management documentation |
| `README.md` | Project README: description, features, quick start, BACnet objects |
| `docs/install-guide.md` | Installation guide: prerequisites, opkg install, cert setup, troubleshooting |
| `docs/config-guide.md` | UCI configuration reference: all 6 sections, examples, mode restrictions |

## BACnet Objects Exposed

| Object | Instance | Value | Units |
|---|---|---|---|
| Device | — | vendor_id, model_name, firmware_rev | — |
| Analog Input | 0 | CPU Usage % | UNITS_PERCENT |
| Analog Input | 1 | Memory Usage % | UNITS_PERCENT |
| Analog Input | 2 | Uptime (seconds) | UNITS_SECONDS |

## BACnet Services

All handlers registered in `bbmd_hub.c` and `bbmd_node.c` (independent duplications): Who-Is/I-Am, ReadProperty, ReadPropertyMultiple, WriteProperty, WritePropertyMultiple, ReinitializeDevice, TimeSync, Who-Has.

## Mode Restrictions

- Hub and node mode cannot run simultaneously (BSC datalink is singleton).
- Bridge mode is exclusive with hub mode and node mode.
- Bridge mode can coexist with telemetry mode.

## Portability

- `-c, --config DIR` — custom config directory (default: `/etc/config`). Sets `UCI_CONFDIR` before UCI context creation.
- `BACNET_FILE_PATH_RESTRICTED=0` — allows absolute cert paths (bacnet-stack defaults to restricting these). Set as compile def in CMakeLists.txt.
- `-f, --foreground` — run in foreground (skip daemonize).
