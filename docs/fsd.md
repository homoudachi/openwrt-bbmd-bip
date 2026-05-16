# Functional Specification Document — OpenWrt BACnet/SC BBMD Node/Hub

> **Implementation Status:** Phase 1 (Foundation) ✅ | Phase 2 (SC Hub) ✅ | Phase 3 (Telemetry) ✅ | Phase 4 (SC Node) ✅ | Phase 5 (BIP Bridge) ✅ | Phase 6 (Cert Mgmt) ✅ | Phase 7 (LuCI UI) ✅ | Phase 8 (Testing/Release) 🔄

## 1. Executive Summary

**Product**: `bbmd` — An OpenWrt binary package (`.ipk`) that transforms a Wi-Fi router into a BACnet/SC (Secure Connect) hub/node for building automation networks.

**Purpose**: Enable commodity routers to serve as the central connectivity point (SC Hub) for BACnet/SC device networks, while also acting as a BACnet node exposing router telemetry (CPU, memory, network stats) as BACnet objects. Optionally bridge BACnet/IP (BBMD) devices to the BACnet/SC network.

**Key differentiator**: First BACnet package ever for OpenWrt. Turns a $30 router into a building-automation-grade SC hub.

---

## 2. System Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                    OpenWrt Router (MIPS/ARM)                     │
│                                                                   │
│  ┌─────────────────┐  ┌──────────────────┐  ┌────────────────┐  │
│  │ BACnet/SC Hub   │  │ BACnet/SC Node   │  │ BACnet/IP BBMD │  │
│  │ (WebSocket Srv) │  │ (Router Telemetry)│  │ (UDP Bridge)   │  │
│  │   wss://:443     │  │  CPU/RAM/Uptime   │  │  UDP :47808    │  │
│  └────────┬────────┘  └────────┬─────────┘  └───────┬────────┘  │
│           │                    │                      │           │
│           └────────────────────┼──────────────────────┘           │
│                                │                                   │
│                    ┌───────────▼──────────────┐                   │
│                    │   BACnet Network Layer   │                   │
│                    │   (NPDU routing)          │                   │
│                    └──────────────────────────┘                   │
│                                                                   │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │              Configuration & Management                      │ │
│  │  UCI (/etc/config/bbmd)  +  LuCI Web UI  +  procd init      │ │
│  └─────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────┘

    LAN: BACnet/IP devices          WAN/Internet: BACnet/SC nodes
    ┌─────────────────────┐          ┌──────────────────────────┐
    │ Building automation │          │ Remote BACnet/SC devices │
    │ BACnet/IP UDP:47808 │          │ wss://router-ip/bacnet-sc│
    │ HVAC, lighting, etc │          │ Cloud-connected BMS      │
    └─────────────────────┘          └──────────────────────────┘
```

---

## 3. Functional Requirements

### FR-1: BACnet/SC Hub Mode

| ID | Requirement | Priority |
|---|---|---|
| FR-1.1 | Accept WebSocket connections from BACnet/SC nodes on configurable port (default 443/47808) | P0 |
| FR-1.2 | Perform TLS 1.3 mutual authentication with X.509 certificates | P0 |
| FR-1.3 | Support WebSocket subprotocol `bacnet.sc` | P0 |
| FR-1.4 | Process HUB_CONNECT frames — register nodes in connection table | P0 |
| FR-1.5 | Route DATA (NVCP type 0x00) frames between connected nodes | P0 |
| FR-1.6 | Handle broadcast distribution (forward to all nodes except sender) | P0 |
| FR-1.7 | Process keepalive (NODE_KEEPALIVE) and respond (HUB_KEEPALIVE) | P0 |
| FR-1.8 | Detect dead connections via keepalive timeout (default 90s) | P0 |
| FR-1.9 | Handle graceful disconnect (HUB_DISCONNECT frames) | P1 |
| FR-1.10 | Maintain connection statistics (uptime, messages routed, nodes connected) | P1 |
| FR-1.11 | Support configurable max connected nodes | P1 |
| FR-1.12 | Log connections, disconnections, and errors | P0 |

### FR-2: BACnet/SC Node Mode

| ID | Requirement | Priority |
|---|---|---|
| FR-2.1 | Connect to a remote BACnet/SC hub via WebSocket (wss://) | P1 |
| FR-2.2 | Perform TLS 1.3 mutual authentication | P1 |
| FR-2.3 | Send HUB_CONNECT frame with device instance | P1 |
| FR-2.4 | Support primary hub + failover hub list | P2 |
| FR-2.5 | Automatic failover with exponential backoff | P2 |
| FR-2.6 | Periodic keepalive sending (configurable interval) | P1 |
| FR-2.7 | Expose router telemetry as BACnet objects (see FR-5) | P1 |

### FR-3: BACnet/IP BBMD Bridge/Gateway

| ID | Requirement | Priority |
|---|---|---|
| FR-3.1 | Listen on UDP port 0xBAC0 (47808) for BACnet/IP traffic | P2 |
| FR-3.2 | Decode BVLL frames, extract NPDU | P2 |
| FR-3.3 | Route NPDU from BACnet/IP → BACnet/SC (encapsulate in NVCP) | P2 |
| FR-3.4 | Route NPDU from BACnet/SC → BACnet/IP (encapsulate in BVLL) | P2 |
| FR-3.5 | BBMD foreign device registration support | P3 |
| FR-3.6 | BBMD broadcast distribution support | P3 |

### FR-4: Certificate Management

| ID | Requirement | Priority |
|---|---|---|
| FR-4.1 | Load CA certificate, hub certificate, and hub private key from filesystem | P0 |
| FR-4.2 | Validate connecting node certificates against CA | P0 |
| FR-4.3 | Support certificate file paths in UCI config | P0 |
| FR-4.4 | Certificate generation helper script (generate CA, hub, node certs) | P2 |
| FR-4.5 | Support certificate reload without restart (SIGHUP) | P2 |

### FR-5: Router Telemetry BACnet Objects

The SC node exposes router telemetry as standard BACnet objects:

| Object Type | Instance | Property | Update Rate | Priority |
|---|---|---|---|---|
| Device | 0 | Router identity, firmware, vendor info | Static | P0 |
| Analog Input | 1 | CPU load % (1-min avg) | 10 sec | P1 |
| Analog Input | 2 | Memory used % | 10 sec | P1 |
| Analog Input | 3 | System uptime (seconds) | 60 sec | P1 |
| Analog Input | 4 | Total RAM (MB) | Static | P1 |
| Analog Input | 5 | Available RAM (MB) | 10 sec | P1 |
| Analog Input | 10 | LAN rx bytes/sec | 30 sec | P2 |
| Analog Input | 11 | LAN tx bytes/sec | 30 sec | P2 |
| Analog Input | 12 | WAN rx bytes/sec | 30 sec | P2 |
| Analog Input | 13 | WAN tx bytes/sec | 30 sec | P2 |
| Analog Input | 20 | CPU temperature (if sensor exists) | 30 sec | P3 |
| Binary Input | 1 | WAN link status | On change | P2 |
| Binary Input | 2 | Wi-Fi radio status | On change | P3 |
| Analog Input | 30 | Connected SC node count (hub mode) | 30 sec | P1 |
| Analog Input | 31 | Hub uptime (hub mode) | 60 sec | P1 |

### FR-6: BACnet Services Supported

| Service | Type | Priority |
|---|---|---|
| Who-Is / I-Am | Unconfirmed | P0 |
| ReadProperty | Confirmed | P0 |
| ReadPropertyMultiple | Confirmed | P1 |
| WriteProperty (telemetry config only) | Confirmed | P2 |
| SubscribeCOV | Confirmed | P2 |
| DeviceCommunicationControl | Confirmed | P2 |
| ReinitializeDevice | Confirmed | P2 |

### FR-7: Configuration & Management

| ID | Requirement | Priority |
|---|---|---|
| FR-7.1 | UCI configuration at `/etc/config/bbmd` | P0 |
| FR-7.2 | procd init script for service management (`/etc/init.d/bbmd`) | P0 |
| FR-7.3 | procd reload trigger on UCI changes | P0 |
| FR-7.4 | SIGHUP-based config reload | P0 |
| FR-7.5 | Logging to syslog (via logd) | P0 |
| FR-7.6 | LuCI web UI for status and configuration | P2 |
| FR-7.7 | LuCI status page showing connected nodes, message stats | P2 |

### FR-8: Operation & Reliability

| ID | Requirement | Priority |
|---|---|---|
| FR-8.1 | Auto-start on boot (procd respawn) | P0 |
| FR-8.2 | Crash recovery (automatic restart via procd) | P0 |
| FR-8.3 | Configurable log levels (error, warn, info, debug) | P0 |
| FR-8.4 | Resource limits (nofiles, memory) | P1 |
| FR-8.5 | Graceful shutdown (send HUB_DISCONNECT to connected nodes) | P1 |

---

## 4. Non-Functional Requirements

| ID | Requirement | Target |
|---|---|---|
| NFR-1 | Binary size | < 500KB (stripped) |
| NFR-2 | Runtime memory | < 16MB (hub with 50 nodes) |
| NFR-3 | Max concurrent SC connections | 50 (configurable) |
| NFR-4 | CPU overhead at idle | < 1% on MIPS 24Kc |
| NFR-5 | Message latency | < 10ms through hub |
| NFR-6 | TLS handshake time | < 500ms (P-256 ECDSA) |
| NFR-7 | Package installation size | < 3MB total |

---

## 5. UCI Configuration Schema

```
/etc/config/bbmd

config globals 'globals'
    option device_instance '4194303'     # BACnet Device Instance
    option device_name 'OpenWrt Router'
    option vendor_id '0'
    option model_name 'OpenWrt BBMD'
    option firmware_rev '1.0.0'
    option network_number '1'            # BACnet network number

config hub 'hub'
    option enabled '1'                   # Run as SC hub
    option port '443'                    # WebSocket listen port
    option tls_cert '/etc/bbmd/certs/hub-cert.pem'
    option tls_key '/etc/bbmd/certs/hub-key.pem'
    option ca_cert '/etc/bbmd/certs/ca-cert.pem'
    option max_nodes '50'
    option keepalive_interval '30'
    option keepalive_timeout '90'

config node 'node'
    option enabled '0'                   # Connect to external hub
    option primary_hub 'wss://hub.example.com/bacnet-sc'
    list failover_hub 'wss://hub2.example.com/bacnet-sc'
    option tls_cert '/etc/bbmd/certs/node-cert.pem'
    option tls_key '/etc/bbmd/certs/node-key.pem'
    option ca_cert '/etc/bbmd/certs/ca-cert.pem'

config telemetry 'telemetry'
    option enabled '1'                   # Expose telemetry as BACnet objects
    option cpu_interval '10'
    option memory_interval '10'
    option network_interval '30'
    option uptime_interval '60'

config bbmd 'bbmd'
    option enabled '1'                   # BACnet/IP BBMD bridge
    option port '47808'
    option lan_interface 'br-lan'

config logging 'logging'
    option level 'info'                  # error|warn|info|debug
```

---

## 6. Package Structure

```
bbmd/
├── Makefile                        # OpenWrt package build
├── Config.in                       # Kconfig (TLS backend selection)
├── files/
│   ├── bbmd.init                   # procd init script
│   ├── bbmd.config                 # UCI default config
│   └── cert-gen.sh                 # Certificate generation helper
├── src/                            # Source (from bacnet-stack + custom)
│   ├── CMakeLists.txt
│   ├── main.c                      # Entry point
│   ├── bbmd_hub.c/h                # Hub mode orchestrator
│   ├── bbmd_node.c/h               # Node mode orchestrator
│   ├── bbmd_config.c/h             # UCI config reader
│   ├── bbmd_telemetry.c/h          # Router telemetry BACnet objects
│   ├── bbmd_bridge.c/h             # BACnet/IP ↔ SC bridge
│   └── bacnet/                     # bacnet-stack source (submodule)
```

---

## 7. Build Dependencies

| Dependency | Purpose | OpenWrt Package |
|---|---|---|
| bacnet-stack (v1.5.0+) | BACnet protocol, BSC hub/node | Source (bundled) |
| libwebsockets (v4.4+) | WebSocket client/server | `libwebsockets-full` |
| OpenSSL or mbedTLS | TLS 1.3 | `libopenssl` or `libmbedtls` |
| libubox | UCI, uloop, ubus | `libubox` |
| libblobmsg-json | JSON parsing | `libblobmsg-json` |
| libubus | ubus bus communication | `libubus` |

---

## 8. Supported Platforms

| Target | Architecture | Status |
|---|---|---|
| mips_24kc | MIPS (common routers) | Primary |
| aarch64_cortex-a53 | ARM64 | Primary |
| x86_64 | x86 VMs | Supported |
| mipsel_24kc | MIPSEL | Supported |

---

## 9. Testing Strategy

| Test Type | Scope | Tools |
|---|---|---|
| Unit tests | BACnet protocol, NVCP encoding, config parsing | CUnit (bacnet-stack built-in) |
| Integration tests | Hub connect, message routing, keepalive | Custom test harness |
| OpenWrt CI | Cross-compile for all targets | GitHub Actions + OpenWrt SDK |
| Manual QA | Install on real router (e.g., GL.iNet) | Physical device test |

---

## 10. Glossary

| Term | Definition |
|---|---|
| **BACnet** | Building Automation and Control Networks (ASHRAE 135) |
| **BACnet/SC** | BACnet Secure Connect — WebSocket + TLS 1.3 transport |
| **BBMD** | BACnet Broadcast Management Device — BACnet/IP broadcast router |
| **BIP** | BACnet/IP — legacy UDP-based BACnet transport |
| **NVCP** | Network Visible Communication Protocol over SC |
| **Hub** | Central WebSocket server routing messages between SC nodes |
| **Node** | BACnet/SC client connecting to a hub |
| **NPDU** | Network Protocol Data Unit — BACnet network layer packet |
| **APDU** | Application Protocol Data Unit — BACnet application layer packet |
| **BVLL** | BACnet Virtual Link Layer — BACnet/IP datalink encapsulation |
