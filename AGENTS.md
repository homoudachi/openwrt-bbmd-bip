# AGENTS.md

## Agent skills

### Issue tracker

GitHub Issues on homoudachi/openwrt-bbmd-bip. See `docs/agents/issue-tracker.md`.

### Triage labels

Default label vocabulary (`needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`). See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: one `CONTEXT.md` at repo root + `docs/adr/` for architecture decisions. See `docs/agents/domain.md`.

---

## Project Overview

This is an OpenWrt binary package (`bbmd`) that transforms a Wi-Fi router into a **BACnet/SC (Secure Connect) node/hub** for building automation networks.

- **Language**: C (bacnet-stack + custom code)
- **Build**: CMake via OpenWrt package SDK
- **Dependencies**: bacnet-stack, libwebsockets, OpenSSL/mbedTLS, libubox
- **Target platforms**: mips_24kc, aarch64_cortex-a53, x86_64

## Key Files

- `docs/fsd.md` — Functional Specification Document
- `docs/roadmap.md` — Implementation roadmap
- `docs/adr/` — Architecture Decision Records
- `Makefile` — OpenWrt package build definition
- `src/` — Daemon source code
- `luci/` — LuCI web interface

## Build Commands

```bash
# Build with OpenWrt SDK
make package/bbmd/compile V=s

# Build tests
(cd build && make bbmd_test && ./bbmd_test)

# Lint
# TBD - install clang-tidy or cppcheck
```
