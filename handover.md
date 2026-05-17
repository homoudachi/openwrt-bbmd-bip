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

Also available: `triage`, `to-issues`, `diagnose`, `zoom-out`, `improve-codebase-architecture`, `webapp-testing`

---

## Current State

### Phases 1-7: All complete (issues #1-#7 closed)
Core daemon (hub/node/bridge/telemetry/config), LuCI Web UI, certificate management. All closed.

### Phase 8: Complete (issue #8, closed)

| # | Task | Status |
|---|---|---|
| 8.1 | Stress test: 50 concurrent SC connections | ⏳ **Unblocked** — passwordless sudo available; QEMU VM ready for bbmd install + test |
| 8.2 | Binary size and memory profiling | ✅ 1.1MB stripped (x86_64), ~12.6MB runtime |
| 8.3 | Install and test on physical router | ⏭️ Blocked — needs physical router |
| 8.4 | README, install guide, configuration guide | ✅ |
| 8.5 | Tag v1.0.0 release | ✅ v1.0.0 tagged (commit 9813642) |
| 8.6 | Submit to openwrt/packages feed | ⏭️ Blocked — needs external PR |

### Phase 9: Bugfix & Virtualization (issue #9, closed)

| # | Task | Status |
|---|---|---|
| 9.1 | Fix duplicate config sections in LuCI (#9) | ✅ cf26303 — `uci get bbmd.@type[0]` |
| 9.2 | QEMU-based LuCI testing VM | ✅ OpenWrt 25.12.4 x86_64 booting on port 8729 |
| 9.3 | Port convention added to AGENTS.md | ✅ 667c194 |
| 9.4 | Build for MIPS (mips_24kc) and profile binary size | ⏳ Next session |
| 9.5 | Stress testing in QEMU | ⏳ Next session |

### v1.0.0 Release (commit 9813642, tag v1.0.0)

Includes all Phase 1-7 work plus portability fixes, MKP macro fix, bridge default fix.

---

## QEMU Virtualization Setup

OpenWrt 25.12.4 x86_64 VM running for LuCI testing and future bbmd daemon testing:

```
VM image:   /tmp/bbmd-qemu/openwrt.img (512MB, ext4 combined)
Mount:      sudo mount -o loop,offset=$((33792*512)) openwrt.img /tmp/bbmd-qemu/rootfs
Boot:       qemu-system-x86_64 -m 512M -drive file=openwrt.img,format=raw,if=virtio \
              -netdev user,id=net0,hostfwd=tcp::8729-:80 -device virtio-net,netdev=net0 \
              -display none -serial file:/tmp/bbmd-qemu/boot.log -no-reboot \
              -pidfile /tmp/bbmd-qemu/qemu.pid -daemonize
Access:     http://<host-ip>:8729/cgi-bin/luci (login: root, no password)
Network:    static 10.0.2.15/24 on eth0 (matches QEMU user-mode guest IP)
Pages:      /cgi-bin/luci/admin/services/bbmd/status
            /cgi-bin/luci/admin/services/bbmd/config
VM PID:     $(cat /tmp/bbmd-qemu/qemu6.pid)

To kill:    kill $(cat /tmp/bbmd-qemu/qemu6.pid)
To modify:  kill VM → mount → edit files → sync → umount → boot
```

**Critical: OpenWrt uses overlayfs.** On first boot, changes go to overlay (if present), otherwise directly to ext4. Always `sync` before unmounting the loop device. Verify changes persisted by re-mounting and checking file content.

**Config on VM (clean, no duplicates):**
- Network: static 10.0.2.15/24 (must match QEMU user-mode for uhttpd to start promptly)
- BBMD: 6 anonymous sections (globals, hub, node, telemetry, bbmd, logging)
- LuCI files: status.js, config.js, menu.json, ACL.json all installed in `/www/luci-static/resources/view/bbmd/` and `/usr/share/`

---

## Next Session — Priority Tasks

User wants: **more virtualization/testing + MIPS binary size profiling**

### 1. Install bbmd daemon into QEMU VM for live testing

```bash
# Build native binary
export PKG_CONFIG_PATH=/tmp/bbmd-pkgconfig:$PKG_CONFIG_PATH
cmake -B build -S src -DBBMD_TLS_BACKEND=openssl
cmake --build build

# Install into VM
kill $(cat /tmp/bbmd-qemu/qemu6.pid)
sudo mount -o loop,offset=$((33792*512)) /tmp/bbmd-qemu/openwrt.img /tmp/bbmd-qemu/rootfs
sudo cp build/bbmd /tmp/bbmd-qemu/rootfs/usr/bin/
sudo cp files/cert-gen.sh /tmp/bbmd-qemu/rootfs/usr/bin/
# Generate certs, create init script, configure procd
sudo sync && sudo umount /tmp/bbmd-qemu/rootfs
# Reboot VM
```

**Challenges**: the native cmake binary links against stub `.so` libraries. It won't run in the real OpenWrt VM without proper libwebsockets/libubox. Options:
- A. Cross-compile from OpenWrt SDK (proper .ipk)
- B. Static-link or copy host libraries to VM
- C. Use the native build with LD_LIBRARY_PATH hacks in the VM

### 2. MIPS binary size profiling

Build for mips_24kc target:
- Needs OpenWrt SDK for mips_24kc (musl)
- `make package/bbmd/compile V=s` from within the SDK
- Profile stripped binary size vs. x86_64 baseline (1.1MB stripped)
- Target: <500KB per NFR-1

This system has no OpenWrt SDK. Options:
- A. Download OpenWrt SDK for mips_24kc: `https://downloads.openwrt.org/releases/25.12.4/targets/ath79/generic/openwrt-sdk-*.tar.xz`
- B. Use CI artifacts from `.github/workflows/build.yml` (already builds for mips_24kc)
- C. Docker-based cross-compilation

### 3. Stress testing (8.1)

With bbmd installed in QEMU VM, simulate concurrent SC connections. Write test harness script.

### 4. Additional virtualization improvements

- Set up multiple VMs (hub + 2 nodes) on separate QEMU instances with tap networking
- Test bridge BIP ↔ BSC routing between VMs
- LuCI-based config testing workflow

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

## File Map (post-issue #9)

```
src/
├── bbmd.c              ← Main loop: hub/node/bridge lifecycle, telemetry timer, SIGHUP reload
├── bbmd_hub.c/h        ← Hub Mode: WebSocket server, TLS, connection table, message routing
├── bbmd_node.c/h       ← Node Mode: client connect, PRIMARY_HUB_URI + FAILOVER_HUB_URI
├── bbmd_bridge.c/h     ← Bridge Mode: two-datalink router (BIP UDP ↔ BSC hub)
├── bbmd_telemetry.c/h  ← Telemetry: /proc polling → BACnet Analog Input objects
├── bbmd_config.c/h     ← UCI config reader (all sections); optional confdir param
├── CMakeLists.txt      ← Build: bacnet-stack submodule + bbmd sources
├── bacnet/             ← bacnet-stack submodule

luci/
├── Makefile
├── htdocs/luci-static/resources/view/bbmd/
│   ├── status.js       ← Status dashboard
│   └── config.js       ← Configuration form (6 TypedSections)
└── root/
    ├── etc/uci-defaults/40_bbmd-luci ← FIXED: uses @type[0] syntax (cf26303)
    └── usr/share/
        ├── rpcd/acl.d/luci-app-bbmd.json
        └── luci/menu.d/luci-app-bbmd.json

files/
├── cert-gen.sh         ← X.509 certificate generation script
├── bbmd.init           ← procd init script
└── bbmd.config         ← UCI config schema

docs/
├── roadmap.md           ← Updated: Phase 9 added, 8.1 unblocked
├── fsd.md
├── certificates.md
├── install-guide.md
├── config-guide.md
└── adr/
```

---

## Uncommitted Changes

```
 M handover.md             ← This edit
 M src/CMakeLists.txt      ← Local build stubs (not for commit)
```

Recent commits on `main` (pushed):
- `d8ea01c` — docs: update handover for issue #9 fix and LuCI QEMU setup
- `667c194` — docs: add port selection convention to AGENTS.md
- `cf26303` — fix: use @type[0] syntax in uci-defaults to prevent duplicate config sections (#9)

---

## Remaining Issues

All issues closed. Next work:
- Stress testing (8.1) — unblocked, needs bbmd installed in QEMU VM
- MIPS binary size profiling (9.4) — needs OpenWrt SDK or CI access
- Physical router testing (8.3) — needs hardware
- openwrt/packages feed submission (8.6) — needs external PR
