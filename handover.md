# Handover — openwrt-bbmd-bip (2026-05-17)

**Repo**: `homoudachi/openwrt-bbmd-bip` (`main`, commit `8d625e1`)
**Working dir**: `C:\Users\Matt\Documents\Projects\opencode\openwrt-bbmd-bip`
**Target router**: OpenWrt 25.12.2, ath79/generic, mips_24kc, `Generic` device

---

## What was built

### Phase 10: BIP-only BBMD mode (no SC, no TLS)

A standalone BACnet/IP BBMD mode for the **Tailscale foreign device** use case:

```
Windows PC (Tailscale IP)       OpenWrt Router (Tailscale IP + LAN IP)
  BACnet client                       bbmd bip mode → UDP 0.0.0.0:47808
  Registers as foreign device         BBMD foreign device table
       │                                    │
       └──── Tailscale VPN (WireGuard) ─────┘
                                            │ LAN (192.168.1.x)
                                     ┌──────┴──────┐
                                     │  BIP Device  │
                                     │  BACnet/IP   │
                                     └─────────────┘
```

**Key design**: Binds UDP to `0.0.0.0` (all interfaces including `tailscale0`) when `lan_interface` is omitted from UCI config. The `bvlc` module records foreign device source addresses from `recvfrom()`, which correctly captures the Tailscale IP. Standard BBMD forwarding handles the rest.

### New UCI config section

```
config bip 'bip'
    option enabled '1'
    option port '47808'
    # lan_interface omitted = bind all interfaces (required for Tailscale)
```

### File manifest (12 files, 504 insertions)

| File | Action | Purpose |
|------|--------|---------|
| `src/bbmd_bip.c` | **New** 170L | BIP-only BBMD: BIP datalink, bvlc BBMD, service handlers |
| `src/bbmd_bip.h` | **New** 11L | API: init/run/maintenance/stop |
| `src/bbmd_config.h` | +7L | `bbmd_bip_config_t` struct (enabled, port, lan_interface) |
| `src/bbmd_config.c` | +13L | UCI reader for `bip` section, defaults (disabled, 47808, NULL iface) |
| `src/bbmd.c` | +66L | BIP mode in main loop: start/stop/reload/mutex checks |
| `src/CMakeLists.txt` | +1L | Add `bbmd_bip.c` to build |
| `docs/roadmap.md` | +64L | Phase 10 section with Tailscale topology diagram |
| `docs/config-guide.md` | +50L | BIP config reference, Tailscale example config |
| `files/bbmd.config` | +6L | UCI defaults |
| `luci/.../config.js` | +20L | BIP configuration form |
| `luci/.../status.js` | +4L | BIP status display |
| `luci/.../40_bbmd-luci` | +7L | First-boot UCI defaults |
| `.github/workflows/release.yml` | **New** 92L | OpenWrt SDK cross-compilation |
| `handover.md` / `HANDOVER.md` | Updated | This file |

### Mode restrictions

| Mode | BIP | Hub | Node | Bridge | Telemetry |
|------|-----|-----|------|--------|-----------|
| BIP | — | No | No | No | Yes |
| Hub | No | — | No | No | Yes |
| Node | No | No | — | No | Yes |
| Bridge | No | No | No | — | Yes |

---

## NEXT SESSION: Verify release build

### The problem

A `.github/workflows/release.yml` workflow was created to cross-compile `.ipk` for the router. It was triggered twice but **failed both times**. Three bugs were diagnosed and fixed in commits `d7c8ebb` and `ada082a`:

| Bug | Symptom | Root cause | Fix |
|-----|---------|-----------|-----|
| SDK not found | "Failed to find SDK" | Greedy regex `[^<>\s]+` consumed `.tar.zst` suffix | Non-greedy `grep -o 'openwrt-sdk-[^"]*\.tar\.zst'` |
| OpenWrt 25.12 format | `.tar.zst` (zstd) not `.tar.xz` (xz) | Newer OpenWrt releases use zstd | Added `tar --zstd` branch |
| Recursive dependency | Kconfig error + TTY open | Missing `make defconfig` after feeds install | Added `make defconfig` before compile |
| PKG_SOURCE_URL | Variable defined after include | Line appended at EOF | Inserted after PKG_SOURCE with sed |

### To trigger

```bash
gh workflow run release.yml \
  -f openwrt_version=25.12.2 \
  -f target=ath79/generic \
  -f variant=openssl
```

Then monitor:
```bash
gh run list --workflow=release.yml --limit 1 --json status,conclusion,url
```

### Remaining risks

1. **Feed timeout**: `./scripts/feeds update -a` downloads ~400MB of package feeds. May hit timeout.
2. **Disk space**: SDK (~250MB) + compiled deps (libwebsockets, openssl, libubox = ~4GB) → runner has ~14GB.
3. **Dep build failures**: Some feed packages may fail to compile on `ubuntu-24.04` runner.
4. **Makefile sed fragility**: The `sed` commands modifying the Makefile may break if upstream Makefile changes.

### Fallback: build locally

If CI builds keep failing, build with OpenWrt SDK directly:

```bash
# On any Linux machine (or WSL/Docker on Windows):
wget https://downloads.openwrt.org/releases/25.12.2/targets/ath79/generic/openwrt-sdk-25.12.2-ath79-generic_gcc-14.3.0_musl.Linux-x86_64.tar.zst
tar --zstd -xf openwrt-sdk-*.tar.zst
cd openwrt-sdk-*
# Copy package files
mkdir -p package/bbmd
cp /path/to/openwrt-bbmd-bip/Makefile package/bbmd/
cp /path/to/openwrt-bbmd-bip/Config.in package/bbmd/
cp -r /path/to/openwrt-bbmd-bip/files/* package/bbmd/
# Create source tarball
cd /path/to/openwrt-bbmd-bip
git archive --format=tar.gz --prefix="bbmd-1.0.0/" -o sdk/dl/bbmd-1.0.0.tar.gz HEAD
cd /path/to/sdk
# Same Makefile edits as the CI does:
sed -i 's|PKG_SOURCE_PROTO:=git|PKG_SOURCE:=bbmd-1.0.0.tar.gz|' package/bbmd/Makefile
sed -i '/PKG_SOURCE_URL:=https:\/\/github.com/d' package/bbmd/Makefile
sed -i '/PKG_SOURCE_DATE/d' package/bbmd/Makefile
sed -i '/PKG_SOURCE_VERSION/d' package/bbmd/Makefile
sed -i '/PKG_SOURCE:=bbmd-1.0.0.tar.gz/a PKG_SOURCE_URL:=file://$(CURDIR)/dl/' package/bbmd/Makefile
# Build
./scripts/feeds update -a
./scripts/feeds install -a
make defconfig
make package/bbmd-openssl/compile V=s
# Result: bin/packages/mips_24kc/base/bbmd-openssl_*.ipk
```

### If the .ipk builds successfully

```bash
# On the router:
opkg install bbmd-openssl_*.ipk

# Configure for BIP-only mode with Tailscale:
uci set bbmd.hub.enabled=0
uci set bbmd.bbmd.enabled=0
uci set bbmd.bip=bip
uci set bbmd.bip.enabled=1
uci set bbmd.bip.port=47808
uci commit bbmd
/etc/init.d/bbmd restart

# On Windows PC (BACnet client):
# Register as foreign device using BBMD's Tailscale IP:
bacwi --foreign <router-tailscale-ip>:47808
```

---

## Deploying on the router

Once the `.ipk` is built:

1. **Install Tailscale** on the router if not already:
   ```bash
   opkg update && opkg install tailscale
   tailscale up
   ```

2. **Install bbmd**:
   ```bash
   opkg install bbmd-openssl_*.ipk
   ```

3. **Configure BIP mode** (via LuCI: Services → BBMD → Configuration → BIP BBMD, or UCI):
   ```bash
   uci set bbmd.hub.enabled=0
   uci set bbmd.bbmd.enabled=0
   uci set bbmd.bip=bip
   uci set bbmd.bip.enabled=1
   uci set bbmd.bip.port=47808
   # Omit lan_interface for Tailscale (binds all interfaces)
   uci commit bbmd
   /etc/init.d/bbmd restart
   ```

4. **Verify**:
   ```bash
   ps | grep bbmd          # should show bbmd running
   netstat -uln | grep 47808  # should show UDP on 0.0.0.0:47808
   logread | grep bbmd     # should show "BIP-only BBMD mode started"
   ```

5. **On Windows PC**, use a BACnet client (YABE, bacnet-stack tools) to register as foreign device and discover the BIP device via the Tailscale IP.

---

## Open issues on GitHub

| # | Title | Status |
|---|-------|--------|
| [#33](https://github.com/homoudachi/openwrt-bbmd-bip/issues/33) | Config structs + BIP module | ✅ Closed |
| [#34](https://github.com/homoudachi/openwrt-bbmd-bip/issues/34) | Main loop integration | ✅ Closed |
| [#35](https://github.com/homoudachi/openwrt-bbmd-bip/issues/35) | Docs + LuCI + UCI defaults | ✅ Closed |
| [#36](https://github.com/homoudachi/openwrt-bbmd-bip/issues/36) | Fix CI release workflow | 🔄 Open — needs build verification |

---

## Key URLs

- **Release workflow**: https://github.com/homoudachi/openwrt-bbmd-bip/actions/workflows/release.yml
- **Latest commit**: https://github.com/homoudachi/openwrt-bbmd-bip/commit/8d625e1
- **OpenWrt SDK download**: https://downloads.openwrt.org/releases/25.12.2/targets/ath79/generic/
  - File: `openwrt-sdk-25.12.2-ath79-generic_gcc-14.3.0_musl.Linux-x86_64.tar.zst` (226 MB)
