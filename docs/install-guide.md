# Install Guide — BBMD for OpenWrt

## Prerequisites

- OpenWrt **21.02+** (or any recent snapshot with procd + libubox support).
- One of these architectures:
  - `mips_24kc` (common MIPS routers)
  - `aarch64_cortex-a53` (ARM64, e.g. GL.iNet GL-MT6000, Raspberry Pi)
  - `x86_64`
  - `mipsel_24kc`

- At least **4 MB free flash** and **16 MB free RAM**.
- Internet access on the router for `opkg` package download (or pre-download the `.ipk`).

## Installation

### 1. Add the bbmd package feed (if not in default feed)

If `bbmd` is not yet part of the official OpenWrt package feed, add the repository URL to `/etc/opkg/customfeeds.conf`:

```
src/gz bbmd https://example.com/path/to/bbmd-repo
```

Then update:

```bash
opkg update
```

### 2. Install the package

```bash
opkg install bbmd-openssl
```

If you prefer the mbedTLS variant:

```bash
opkg install bbmd-mbedtls
```

### 3. Verify package files

After installation, confirm the key files are in place:

```bash
ls -l /usr/sbin/bbmd         # daemon binary
ls -l /usr/sbin/bbmd-cert-gen # certificate generator
ls -l /etc/init.d/bbmd       # procd init script
ls -l /etc/config/bbmd       # UCI configuration (defaults)
```

### Dependencies Installed Automatically

| Package | Purpose |
|---|---|
| `libwebsockets-full` | WebSocket client/server library (TLS) |
| `libopenssl` (or `libmbedtls`) | TLS 1.3 cryptography |
| `libubox` | UCI config parsing, uloop event loop |
| `libblobmsg-json` | JSON blob handling |
| `libubus` | ubus inter-process communication |
| `openssl-util` | OpenSSL CLI tools (for `bbmd-cert-gen`) |

## Post-Install: Certificate Setup

BACnet/SC requires X.509 certificates for TLS 1.3 mutual authentication. Use the included `bbmd-cert-gen` script:

```bash
# 1. Generate CA (self-signed root of trust)
bbmd-cert-gen ca

# 2. Generate hub certificate
bbmd-cert-gen hub
```

For node mode (connecting to a remote hub), also generate a node certificate:

```bash
bbmd-cert-gen node myrouter
```

All certificates are stored in `/etc/bbmd/certs/`.

See the [Certificate Management Guide](certificates.md) for advanced topics: deploying certs to remote nodes, using an external CA, and file permissions.

## Enabling and Starting the Service

```bash
# Enable auto-start on boot
/etc/init.d/bbmd enable

# Start now
/etc/init.d/bbmd start

# Check status
/etc/init.d/bbmd status
```

## Verifying Installation

### Daemon Process

```bash
ps | grep bbmd
```

You should see `/usr/sbin/bbmd` running with the active mode flags.

### Log Output

```bash
logread -e bbmd
```

Expected startup messages include the device instance, active modes, and listening ports.

### LuCI Web UI

If `luci-app-bbmd` is installed, navigate to **Services → BBMD (BACnet)** in the LuCI web interface. The status page shows:

- Daemon running/stopped
- Device identity (instance, name, firmware)
- Active modes (hub/node/bridge/telemetry)
- System stats (CPU, memory, uptime)

### BACnet Discovery

From any BACnet tool on the LAN (e.g. YABE, BACnet Explorer), send a **Who-Is** broadcast. The BBMD responds with **I-Am** containing the configured device instance.

## Troubleshooting

### Daemon won't start

Check the logs:

```bash
logread -e bbmd
```

Common causes:

- **Missing certificates**: Run `bbmd-cert-gen ca` and `bbmd-cert-gen hub` before starting.
- **Port conflict**: Ensure port 443 (hub) or 47808 (bridge) is not in use by another service.
- **Invalid UCI config**: Run `uci show bbmd` and verify the config. Use `uci revert bbmd` to reset to defaults.

### Node won't connect to hub

- Verify the hub is reachable from the node's network.
- Ensure the node's CA certificate matches the hub's CA (same root).
- Check that the node's certificate is signed by the correct CA.
- Confirm the `primary_hub` URI uses the `wss://` scheme and correct path (e.g. `/bacnet-sc`).

### LuCI page shows "Stopped" but daemon is running

The LuCI status page checks for `/var/run/bbmd.pid`. If the daemon was started manually without the init script, this file may not exist. Restart via the init script:

```bash
/etc/init.d/bbmd restart
```

### Re-installing or Upgrading

```bash
opkg update
opkg install bbmd
/etc/init.d/bbmd restart
```

UCI configuration is preserved during upgrade (marked as conffile in the package).
