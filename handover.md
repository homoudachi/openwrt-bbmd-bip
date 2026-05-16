# Handover Prompt — OpenWrt BACnet/SC BBMD Project

**Repo**: `homoudachi/openwrt-bbmd-bip` (`main` branch)
**Working dir**: `/home/matt/opencode/openwrt-bbmd-bip`
**Release**: https://github.com/homoudachi/openwrt-bbmd-bip/releases/tag/v1.0.0
**Watch the issues**: `gh issue list --repo homoudachi/openwrt-bbmd-bip`

---

## Orchestrator Instructions

This is an **orchestrator-level prompt**. Your job is not to write code directly — use the **subagent-driven-development** skill to dispatch tasks to subagents (implementer → spec reviewer → code quality reviewer per task). Use **tdd** where appropriate.

**Process per issue:**
1. Read the issue body and its task list
2. For each independent task, dispatch an implementer subagent
3. After implementer reports DONE, dispatch spec reviewer subagent
4. After spec review passes, dispatch code quality reviewer subagent
5. Verify build compiles: `cmake --build build`
6. Fix any issues found by reviewers
7. When all tasks for an issue are complete, close the issue
8. **Update docs** (roadmap.md, CONTEXT.md) to reflect completion
9. **Write this handover prompt** before the end of your run

**GitHub is the source of truth.** Before starting any work, `gh issue view <N>` the relevant issue. Close issues when done with a summary comment.

---

## Skills to Load (in order)

```
/skill subagent-driven-development
/skill tdd
```

Also available: `triage`, `to-issues`, `diagnose`, `zoom-out`, `improve-codebase-architecture`

---

## Current State

### Phase 1-6: All complete (issues #1-#6 closed)
See previous handover for details. Core: OpenWrt package skeleton, BACnet/SC Hub, telemetry, SC Node, BIP Bridge, cert management.

### Phase 7: LuCI Web UI (issue #7, closed)
Status dashboard + config form + menu/ACL + UCI defaults.

### Phase 8: Complete (issue #8, closed)

| # | Task | Status |
|---|---|---|
| 8.1 | Stress test: 50 concurrent SC connections | ⏭️ **Unblocked** — passwordless sudo now available |
| 8.2 | Binary size and memory profiling | ✅ 1.1MB stripped (x86_64), ~12.6MB runtime |
| 8.3 | Install and test on physical router | ⏭️ Blocked — needs physical router |
| 8.4 | README, install guide, configuration guide | ✅ |
| 8.5 | Tag v1.0.0 release | ✅ v1.0.0 tagged (commit 9813642) |
| 8.6 | Submit to openwrt/packages feed | ⏭️ Blocked — needs external PR |

### Phase 9: Bugfix (issue #9, closed)

| # | Task | Status |
|---|---|---|
| 9.1 | Fix duplicate config sections in LuCI form | ✅ Fixed in cf26303 — uci-defaults used `uci get bbmd.globals` (named section) instead of `uci get bbmd.@globals[0]` (type-indexed) |

### v1.0.0 Release (commit 9813642, tag v1.0.0)

Includes all Phase 1-7 work plus:
- Portability: `-c, --config DIR` flag wired (sets UCI_CONFDIR)
- Portability: `BACNET_FILE_PATH_RESTRICTED=0` (allows absolute cert paths)
- Bugfix: MKP macro comma operator wrapping
- Bugfix: Bridge default changed from `true` to `false`

### LuCI Testing VM

QEMU VM running OpenWrt 25.12.4 x86_64 for LuCI testing:
```bash
# VM: PID in /tmp/bbmd-qemu/qemu3.log, boot log at /tmp/bbmd-qemu/boot3.log
# Port forward: host:8729 → guest:80
# Access: http://<host-ip>:8729/cgi-bin/luci (login: root, no password)
# BBMD pages: /admin/services/bbmd/status and /admin/services/bbmd/config
```

---

## Next Steps (in priority order)

### Stress testing (8.1) — now unblocked

Passwordless sudo is available. Steps:
```bash
# 1. Build bbmd binary (native cmake with stubs)
cmake -B build -S src -DBBMD_TLS_BACKEND=openssl && cmake --build build

# 2. Install into QEMU rootfs
sudo mount -o loop,offset=$((33792*512)) /tmp/bbmd-qemu/openwrt.img /tmp/bbmd-qemu/rootfs
sudo cp build/bbmd /tmp/bbmd-qemu/rootfs/usr/bin/
# Copy certs, write init script, etc.
sudo umount /tmp/bbmd-qemu/rootfs

# 3. Boot and run stress tests
```

### openwrt/packages submission (8.6)

Submit PR to `openwrt/packages` feed adding `admin/bbmd/`. Requirements:
- Conforms to OpenWrt package policy
- Build tested on CI (x86_64, mips_24kc, aarch64_cortex-a53)
- Maintainer info filled in Makefile

---

## Build Commands

```bash
# Stub pkgconfig setup (required — this system has no libwebsockets/libubox dev)
export PKG_CONFIG_PATH=/tmp/bbmd-pkgconfig:$PKG_CONFIG_PATH

# Native build test
cmake -B build -S src -DBBMD_TLS_BACKEND=openssl
cmake --build build

# Usage (with forwarded -c flag)
./build/bbmd -f -c /path/to/config/dir
```

**Local build hacks (not for commit):**
- `src/CMakeLists.txt` has stub include/link paths for libwebsockets/uci (`/tmp/bbmd-uci-stub`, `/tmp/bbmd-lws-stub`)
- These are present in the working copy but excluded from commits via selective staging

---

## File Map (post-Phase 8, v1.0.0)

```
src/
├── bbmd.c              ← Main loop: hub/node/bridge lifecycle, telemetry timer, SIGHUP reload; 3-way mode exclusivity; -c flag support
├── bbmd_hub.c/h        ← Hub Mode: env vars + HUB_FUNCTION_BINDING, Device Object, Send_I_Am, event loop
├── bbmd_node.c/h       ← Node Mode: mirrors hub; PRIMARY_HUB_URI + FAILOVER_HUB_URI, no HUB_FUNCTION_BINDING
├── bbmd_bridge.c/h     ← Bridge Mode: two-datalink router (BIP UDP + BSC hub), NPDU routing, BBMD FD, DNET table
├── bbmd_telemetry.c/h  ← Telemetry: /proc polling → BACnet Analog Input objects
├── bbmd_config.c/h     ← UCI config reader (all sections parsed); accepts optional confdir param
├── CMakeLists.txt      ← Build: bacnet-stack submodule, compile defs inc. BACNET_FILE_PATH_RESTRICTED=0, BACDL_BIP+BACDL_BSC ON
├── bacnet/             ← bacnet-stack submodule

luci/
├── Makefile            ← LuCI app package (standalone, no luci.mk)
├── htdocs/luci-static/resources/view/bbmd/
│   ├── status.js       ← Status dashboard: PID check, device identity, modes, CPU/mem/uptime
│   └── config.js       ← Configuration form: 6 TypedSections with conditional depends
└── root/
    ├── etc/uci-defaults/40_bbmd-luci ← First-boot UCI defaults + cert dir
    └── usr/share/
        ├── rpcd/acl.d/luci-app-bbmd.json  ← ACL: UCI read/write + /proc file access
        └── luci/menu.d/luci-app-bbmd.json ← Menu: Services → BBMD (BACnet)

files/
└── cert-gen.sh         ← X.509 certificate generation script (CA, hub, node)

docs/
├── certificates.md      ← Certificate management documentation
├── install-guide.md     ← Installation guide (prerequisites, opkg, certs, troubleshooting)
├── config-guide.md      ← UCI configuration reference (6 sections, 4 examples, mode restrictions)
├── fsd.md               ← Functional spec
└── roadmap.md           ← Implementation roadmap (all phases marked)
```

**Docs updated:**
```
CONTEXT.md              ← Phase 8 complete, portability section added
docs/roadmap.md         ← Phase 8.5 marked complete, file map updated
docs/agents/domain.md   ← Phase 8 status note
docs/fsd.md             ← Phase 8 status (from previous handover)
```

---

## Uncommitted Changes

```
 M handover.md             ← Handover status updates (this edit)
 M src/CMakeLists.txt      ← Local build stubs only (not for commit)
```

Recent commits on `main` (not yet pushed):
- `667c194` — docs: add port selection convention to AGENTS.md
- `cf26303` — fix: use @type[0] syntax in uci-defaults to prevent duplicate config sections (#9)

---

## Remaining Issues

Issue #9 closed (duplicate config fix). Remaining work:
- Stress testing (8.1) — now unblocked (sudo available)
- Physical router testing (8.3) — needs hardware
- openwrt/packages feed submission (8.6) — needs external PR
