# ADR-001: BACnet Protocol Stack Selection

**Status**: Accepted  
**Date**: 2026-05-16  
**Deciders**: Architecture review

## Context

We need a BACnet protocol stack for an OpenWrt router package that supports:
1. BACnet/SC (Secure Connect) — hub and node modes
2. BACnet/IP (for BBMD bridge mode)
3. Embedded-friendly (runs on MIPS/ARM routers with limited RAM/CPU)
4. Actively maintained
5. Open-source license compatible with OpenWrt

## Options Considered

### Option A: bacnet-stack (C, GPL-2.0 + GCC exception)

**bacnet-stack** by Steve Karg is the de facto open-source BACnet C library.
- v1.5.0 (April 2026) has full BACnet/SC support (`src/bacnet/datalink/bsc/`)
- Supports hub, node, and node-switch modes
- Clean WebSocket abstraction layer (`bws_cli_connect`, `bws_srv_start`)
- Uses libwebsockets for WebSocket transport
- Embedded-friendly: ports for Linux, BSD, Zephyr, STM32, PIC
- 60+ BACnet object types, full service coverage
- 561 GitHub stars, active development (20+ contributors)

### Option B: Custom C implementation

- Full control over code size and features
- Massive effort to implement BACnet protocol correctly
- BACnet is a 1300+ page standard — reimplementation is infeasible

### Option C: Rust (rusty-bacnet)

- rusty-bacnet has BACnet/SC hub support
- No Rust toolchain in standard OpenWrt package feeds
- Would require custom toolchain packaging

### Option D: Go

- No Go BACnet implementation has SC support
- Binary size too large for embedded (2-5MB+)

## Decision

**Option A: bacnet-stack (C)**

Integrated as a Git submodule or bundled source within the package.

## Consequences

### Positive
- Full, standards-compliant BACnet implementation
- BACnet/SC hub and node already implemented
- Embedded-proven codebase (used in Zephyr RTOS)
- Active upstream maintenance
- Built-in unit test framework (CUnit)

### Negative
- Requires libwebsockets (in OpenWrt feeds but may need `LWS_MAX_SMP` patch)
- Certificate management in bacnet-stack BSC is readonly (acceptable for v1)
- GPL-2.0 license (FOSS-compatible, permissible for OpenWrt)

### Mitigations
- Bundle bacnet-stack as a submodule pinned to a specific release
- Patch libwebsockets OpenWrt build if needed to set `LWS_MAX_SMP=32`
- Custom cert management layer on top of bacnet-stack's cert handling
