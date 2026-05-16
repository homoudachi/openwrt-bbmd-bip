# ADR-005: Router as Hub + Node + Telemetry Device

**Status**: Accepted  
**Date**: 2026-05-16  
**Deciders**: Architecture review

## Context

The OpenWrt router should serve three roles simultaneously within a single daemon:
1. **BACnet/SC Hub**: WebSocket server accepting connections from BACnet/SC nodes
2. **BACnet/SC Node**: Expose router telemetry (CPU, memory, network) as BACnet objects reachable by other BACnet devices on the SC network
3. **BACnet/IP BBMD Bridge**: Optionally bridge legacy BACnet/IP devices to the SC network

## Decision

### Single Daemon, Multiple Roles

One daemon (`bbmd`) hosting all roles via bacnet-stack's multi-interface architecture:

```
┌─────────────── bbmd daemon ───────────────┐
│                                            │
│  ┌──────────────────────────────────┐     │
│  │      BACnet Stack Core           │     │
│  │  (Device Object, Services)       │     │
│  └──────────┬───────────────────────┘     │
│             │                               │
│  ┌──────────┼───────────────────────┐     │
│  │   Datalink Layer                │     │
│  │  ┌──────────┐ ┌──────────┐ ┌───┐│     │
│  │  │ SC Hub   │ │ SC Node  │ │BIP││     │
│  │  │ (server) │ │ (client) │ │UDP││     │
│  │  └──────────┘ └──────────┘ └───┘│     │
│  └──────────────────────────────────┘     │
│                                            │
│  ┌──────────────────────────────────┐     │
│  │  Telemetry Engine                │     │
│  │  (reads /proc, sysinfo, netlink) │     │
│  └──────────────────────────────────┘     │
└────────────────────────────────────────────┘
```

### Architecture Rationale

1. **Single daemon** reduces resource usage (one process vs three)
2. **bacnet-stack supports multiple datalinks** natively — it's designed for this
3. **Shared BACnet device object** — the router has one Device Object regardless of how many interfaces it has
4. **Telemetry as BACnet objects** — standard Analog Input, Binary Input objects are updated by a telemetry polling thread

### Startup Mode Selection

The daemon starts based on UCI configuration:
- If `hub.enabled=1`: start SC hub (WebSocket server)
- If `node.enabled=1`: connect to external hub as node
- If `bbmd.enabled=1`: start BACnet/IP BBMD bridge
- If `telemetry.enabled=1`: start telemetry polling
- Multiple modes can run simultaneously

### Telemetry Implementation

```c
// bbmd_telemetry.h
typedef struct {
    float cpu_load;          // /proc/loadavg 1-min
    int mem_used_pct;        // /proc/meminfo
    int mem_total_mb;
    int mem_avail_mb;
    unsigned long uptime;    // /proc/uptime
    uint64_t lan_rx_bytes;   // /proc/net/dev
    uint64_t lan_tx_bytes;
    uint64_t wan_rx_bytes;
    uint64_t wan_tx_bytes;
    int wan_link;            // netlink/ubus
    int connected_nodes;     // hub internal
} bbmd_telemetry_t;
```

Update intervals:
- CPU/Memory: 10 seconds
- Network stats: 30 seconds
- Uptime/node count: 60 seconds

Data sources:
- `/proc/loadavg`, `/proc/meminfo`, `/proc/uptime` (always available)
- `/proc/net/dev` or netlink for network stats
- ubus call to netifd for WAN link status
- Internal hub connection table for connected node count

## Consequences

### Positive
- Single process, single binary — efficient
- All BACnet communication goes through one stack instance
- Telemetry automatically available on all connected interfaces
- Configurable — admin can disable individual modes

### Negative
- Slightly more complex startup logic
- Hub crash = all modes down (but procd respawn handles this)

### Mitigations
- Clear mode separation in code (separate .c files per mode)
- procd respawn ensures automatic recovery
- Log level per mode for debugging
