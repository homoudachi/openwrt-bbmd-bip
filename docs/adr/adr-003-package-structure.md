# ADR-003: Package Structure and OpenWrt Integration

**Status**: Accepted  
**Date**: 2026-05-16  
**Deciders**: Architecture review

## Context

We need to define how the package integrates with OpenWrt's build system, init system, configuration system, and web UI.

## Decision

### 1. Package Structure

Standard OpenWrt feed layout under `net/bbmd/`:

```
openwrt-bbmd-bip/
├── Makefile                        # Top-level OpenWrt package Makefile
├── Config.in                       # Kconfig menu entries (TLS backend selection)
├── files/
│   ├── bbmd.init                   # procd init script (/etc/init.d/bbmd)
│   ├── bbmd.config                 # Default UCI config (/etc/config/bbmd)
│   └── cert-gen.sh                 # Certificate generation script
├── src/
│   ├── CMakeLists.txt              # CMake build for bbmd + bacnet-stack
│   ├── bbmd.c                      # Main entry point
│   ├── bbmd_hub.c/h                # Hub mode
│   ├── bbmd_node.c/h               # Node mode
│   ├── bbmd_config.c/h             # UCI config reader
│   ├── bbmd_telemetry.c/h          # Router telemetry objects
│   └── bacnet/                     # bacnet-stack (git submodule)
├── luci/                           # Optional LuCI app (separate package)
│   ├── Makefile
│   ├── htdocs/luci-static/resources/view/bbmd/bbmd.js
│   ├── root/usr/share/luci/menu.d/luci-app-bbmd.json
│   └── root/usr/share/rpcd/acl.d/luci-app-bbmd.json
├── test/                           # Integration tests
├── docs/                           # Documentation
│   ├── fsd.md
│   ├── roadmap.md
│   └── adr/
└── .github/workflows/              # CI/CD
    └── build.yml
```

### 2. Build System

CMake-based build using `include $(INCLUDE_DIR)/cmake.mk`:

- bacnet-stack built as a static library
- bbmd sources compiled and linked against bacnet-stack + libwebsockets + OpenSSL/mbedTLS
- TLS backend selection via CMake option `-DBBMD_TLS_BACKEND=openssl|mbedtls`

### 3. Init System

procd init script (`/etc/init.d/bbmd`):
- `start_service()` reads UCI config, builds command line, calls procd
- `service_triggers()` registers `procd_add_reload_trigger bbmd`
- `reload_service()` sends SIGHUP to daemon
- procd `respawn` enabled for crash recovery

### 4. Configuration

UCI at `/etc/config/bbmd`:
- `globals` section for device identity (device_instance, device_name, vendor_id)
- `hub` section for hub mode parameters
- `node` section for client mode parameters
- `telemetry` section for router monitoring objects
- `bbmd` section for BACnet/IP bridge parameters
- `logging` section for log level

### 5. LuCI Web UI (Phase 2)

Separate package `luci-app-bbmd`:
- JavaScript-based LuCI2 view
- Status page: connected nodes, message stats, uptime
- Configuration page: hub/node/cert/telemetry settings
- ACL: read/write access to `uci bbmd`

## Consequences

### Positive
- Follows OpenWrt conventions exactly (package devs will feel at home)
- procd gives us crash recovery for free
- UCI gives us config management for free
- Separate LuCI package means users can skip the web UI if they want

### Negative
- UCI parsing in C requires libubox (small dependency)
- Two-package structure (bbmd + luci-app-bbmd) means two Makefiles

### Mitigations
- libubox is already on every OpenWrt device (it IS OpenWrt's userspace)
- Two packages is standard practice in OpenWrt feeds
