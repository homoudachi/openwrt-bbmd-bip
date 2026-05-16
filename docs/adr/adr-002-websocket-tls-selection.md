# ADR-002: WebSocket and TLS Library Selection

**Status**: Accepted  
**Date**: 2026-05-16  
**Deciders**: Architecture review

## Context

BACnet/SC requires WebSocket (RFC 6455) with TLS 1.3 for transport. We need:
1. WebSocket server (hub mode) and client (node mode)
2. TLS 1.3 with X.509 certificate mutual authentication
3. Available in OpenWrt package feeds
4. Lightweight enough for embedded routers
5. BSD/MIT licensed preferred

## Options Considered

### WebSocket

| Library | OpenWrt? | License | Notes |
|---|---|---|---|
| **libwebsockets** | Yes (`libs/libwebsockets`) | MIT | v4.4.1, mature, used by bacnet-stack |
| mongoose | No | Dual GPL/Comm | Used in Zephyr backend of bacnet-stack |
| libuwsc | No | MIT | Lightweight but not in OpenWrt |

### TLS

| Library | OpenWrt? | Binary Size | Notes |
|---|---|---|---|
| **OpenSSL** | Yes (`libopenssl`) | ~1.5MB | Most complete, used by libwebsockets |
| **mbedTLS** | Yes (`libmbedtls`) | ~250KB | Smaller, OpenWrt default |
| wolfSSL | Yes (`libwolfssl`) | ~300KB | Lightweight alternative |

## Decision

**WebSocket**: libwebsockets (MIT, v4.4.1+) — already in OpenWrt, used by bacnet-stack BSC

**TLS**: Support both OpenSSL and mbedTLS via OpenWrt package variants:
- `bbmd-openssl` — uses `libopenssl` + `libwebsockets-full`
- `bbmd-mbedtls` — uses `libmbedtls` + `libwebsockets-mbedtls`

Default variant: `bbmd-mbedtls` (smaller, aligns with OpenWrt's default TLS stack)

## Critical Requirement

bacnet-stack BSC requires `LWS_MAX_SMP > 1` (recommended 32). The default OpenWrt libwebsockets package uses `LWS_MAX_SMP=1`. We must either:
1. Build libwebsockets from our package with `-DLWS_MAX_SMP=32`, OR
2. Submit a patch upstream to increase the default

Decision: Build with `-DLWS_MAX_SMP=32` in our CMake configuration.

## Consequences

### Positive
- libwebsockets is battle-tested, BSD-licensed, and in OpenWrt feeds
- TLS library choice gives users flexibility
- mbedTLS variant reduces binary size significantly

### Negative
- Variants increase maintenance surface
- `LWS_MAX_SMP=32` increases libwebsockets binary size slightly

### Mitigations
- Single codebase with compile-time TLS backend selection
- Automated CI matrix for both variants
