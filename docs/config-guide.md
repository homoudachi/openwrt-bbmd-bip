# Configuration Guide — BBMD for OpenWrt

## UCI Configuration File

BBMD is configured via OpenWrt's UCI system. The configuration file is:

```
/etc/config/bbmd
```

You can edit it directly with `vi` or `nano`, use `uci` commands, or use the LuCI web interface (**Services → BBMD (BACnet) → Configuration**).

## Sections Reference

### `globals` — Device Identity

BACnet device identity and network settings.

| Option | Type | Default | Description |
|---|---|---|---|
| `device_instance` | integer | `4194303` | Unique BACnet device instance number (0–4194303). Must be unique on the BACnet internetwork. |
| `device_name` | string | `OpenWrt Router` | Human-readable name for the BACnet device. |
| `vendor_id` | integer | `0` | ASHRAE-assigned vendor ID. `0` = unregistered. |
| `model_name` | string | `OpenWrt BBMD` | Model name reported in the Device object. |
| `firmware_rev` | string | `1.0.0` | Firmware version string reported in the Device object. |
| `network_number` | integer | `1` | BACnet network number for this router/gateway. Must be unique on the BACnet internetwork if bridging multiple networks. |

```
config globals 'globals'
    option device_instance '4194303'
    option device_name 'My Router'
    option vendor_id '0'
    option model_name 'OpenWrt BBMD'
    option firmware_rev '1.0.0'
    option network_number '1'
```

### `hub` — BACnet/SC Hub Mode

Run as a BACnet/SC hub, accepting WebSocket connections from SC nodes.

| Option | Type | Default | Description |
|---|---|---|---|
| `enabled` | boolean | `1` | Enable hub mode. Hub and node mode are mutually exclusive. |
| `port` | integer | `443` | TCP port for the WebSocket server. Typically 443 (HTTPS) or 47808. |
| `tls_cert` | string | `/etc/bbmd/certs/hub-cert.pem` | Path to the hub's TLS certificate (PEM). |
| `tls_key` | string | `/etc/bbmd/certs/hub-key.pem` | Path to the hub's TLS private key (PEM). |
| `ca_cert` | string | `/etc/bbmd/certs/ca-cert.pem` | Path to the CA certificate (PEM). Used to verify connecting node certificates. |
| `max_nodes` | integer | `50` | Maximum concurrent connected SC nodes. |
| `keepalive_interval` | integer | `30` | Seconds between hub keepalive responses. |
| `keepalive_timeout` | integer | `90` | Seconds without a node keepalive before considering the connection dead. |

```
config hub 'hub'
    option enabled '1'
    option port '443'
    option tls_cert '/etc/bbmd/certs/hub-cert.pem'
    option tls_key '/etc/bbmd/certs/hub-key.pem'
    option ca_cert '/etc/bbmd/certs/ca-cert.pem'
    option max_nodes '50'
    option keepalive_interval '30'
    option keepalive_timeout '90'
```

### `node` — BACnet/SC Node Mode

Connect to a remote BACnet/SC hub as a client node.

| Option | Type | Default | Description |
|---|---|---|---|
| `enabled` | boolean | `0` | Enable node mode. Hub and node mode are mutually exclusive. |
| `primary_hub` | string | — | WebSocket URI of the primary hub, e.g. `wss://hub.example.com/bacnet-sc`. |
| `failover_hub` | list | — | One or more failover hub URIs. The node tries each in order with exponential backoff. |
| `tls_cert` | string | `/etc/bbmd/certs/node-cert.pem` | Path to this node's TLS certificate (PEM). |
| `tls_key` | string | `/etc/bbmd/certs/node-key.pem` | Path to this node's TLS private key (PEM). |
| `ca_cert` | string | `/etc/bbmd/certs/ca-cert.pem` | Path to the CA certificate (PEM). Used to verify the hub's certificate. |
| `retry_interval` | integer | `5` | Initial retry interval in seconds after connection failure. |
| `max_retry_interval` | integer | `60` | Maximum retry interval in seconds (exponential backoff cap). |

```
config node 'node'
    option enabled '1'
    option primary_hub 'wss://hub.example.com/bacnet-sc'
    list failover_hub 'wss://hub2.example.com/bacnet-sc'
    list failover_hub 'wss://hub3.example.com/bacnet-sc'
    option tls_cert '/etc/bbmd/certs/node-cert.pem'
    option tls_key '/etc/bbmd/certs/node-key.pem'
    option ca_cert '/etc/bbmd/certs/ca-cert.pem'
    option retry_interval '5'
    option max_retry_interval '60'
```

### `telemetry` — Router Telemetry BACnet Objects

Expose router system stats as BACnet Analog Input objects.

| Option | Type | Default | Description |
|---|---|---|---|
| `enabled` | boolean | `1` | Enable telemetry BACnet objects. |
| `cpu_interval` | integer | `10` | CPU load polling interval in seconds (reads `/proc/stat`). |
| `memory_interval` | integer | `10` | Memory usage polling interval in seconds (reads `/proc/meminfo`). |
| `network_interval` | integer | `30` | Network interface stats polling interval in seconds (reads `/proc/net/dev`). |
| `uptime_interval` | integer | `60` | System uptime polling interval in seconds (reads `/proc/uptime`). |

```
config telemetry 'telemetry'
    option enabled '1'
    option cpu_interval '10'
    option memory_interval '10'
    option network_interval '30'
    option uptime_interval '60'
```

### `bbmd` — BACnet/IP to BACnet/SC Bridge (BBMD)

Bridge legacy BACnet/IP devices to the BACnet/SC network. Also known as the bridge or BBMD mode.

| Option | Type | Default | Description |
|---|---|---|---|
| `enabled` | boolean | `1` | Enable bridge mode. Cannot run simultaneously with hub or node mode. |
| `port` | integer | `47808` | UDP port for BACnet/IP traffic (standard: 0xBAC0 = 47808). |
| `lan_interface` | string | `br-lan` | LAN interface to bind the BACnet/IP UDP socket to. Typically `br-lan` for the built-in switch. |

```
config bbmd 'bbmd'
    option enabled '1'
    option port '47808'
    option lan_interface 'br-lan'
```

### `bip` — BIP-Only BBMD Mode

Run as a standalone BACnet/IP BBMD without BACnet/SC. No TLS certificates required. Suitable for Tailscale VPN / foreign-device setups where remote clients register as BBMD foreign devices and access BACnet/IP devices on the router's LAN.

| Option | Type | Default | Description |
|---|---|---|---|
| `enabled` | boolean | `0` | Enable BIP-only BBMD mode. Cannot run simultaneously with hub, node, or bridge mode. |
| `port` | integer | `47808` | UDP port for BACnet/IP traffic (standard: 0xBAC0 = 47808). |
| `lan_interface` | string | (all) | LAN interface to bind the BACnet/IP UDP socket to. Leave empty or omit to bind all interfaces — required for Tailscale VPN foreign device support. |

```
config bip 'bip'
    option enabled '1'
    option port '47808'
    # lan_interface omitted: binds to all interfaces (0.0.0.0)
    # for Tailscale foreign device support
```

### `logging` — Log Level

| Option | Type | Default | Description |
|---|---|---|---|
| `level` | string | `info` | Log verbosity. One of: `error`, `warn`, `info`, `debug`. |

```
config logging 'logging'
    option level 'info'
```

## Mode Restrictions

- **Hub + Node**: Cannot run simultaneously. The BACnet/SC datalink is a singleton — the daemon can be either a hub (accepting connections) or a node (connecting to a hub), but not both.
- **Bridge + Hub/Node**: The BBMD bridge is exclusive with both hub and node mode. Each mode initialises a separate BACnet datalink, and the bridge's two-datalink router structure conflicts with the SC-only hub/node setup.
- **BIP mode**: BIP mode is also exclusive with hub, node, and bridge mode.
- **Bridge + Telemetry**: These can run together. Telemetry only depends on `/proc` files and the BACnet Device Object, not on the datalink type.
- **BIP + Telemetry**: BIP mode can coexist with telemetry.

The daemon validates mode exclusivity on startup and logs an error if conflicting modes are enabled.

## Example Configurations

### Hub-Only Router

A standalone BACnet/SC hub accepting connections from remote SC nodes:

```
config globals 'globals'
    option device_instance '1001'
    option device_name 'Building A Hub'
    option network_number '1'

config hub 'hub'
    option enabled '1'
    option port '443'
    option tls_cert '/etc/bbmd/certs/hub-cert.pem'
    option tls_key '/etc/bbmd/certs/hub-key.pem'
    option ca_cert '/etc/bbmd/certs/ca-cert.pem'
    option max_nodes '50'
    option keepalive_interval '30'
    option keepalive_timeout '90'

config node 'node'
    option enabled '0'

config telemetry 'telemetry'
    option enabled '1'

config bbmd 'bbmd'
    option enabled '0'

config logging 'logging'
    option level 'info'
```

### Node Connecting to Remote Hub

The router acts as a telemetry node, pushing stats to an upstream hub:

```
config globals 'globals'
    option device_instance '2001'
    option device_name 'Floor 3 Router'
    option network_number '2'

config hub 'hub'
    option enabled '0'

config node 'node'
    option enabled '1'
    option primary_hub 'wss://central-hub.example.com/bacnet-sc'
    list failover_hub 'wss://backup-hub.example.com/bacnet-sc'
    option tls_cert '/etc/bbmd/certs/node-floor3-cert.pem'
    option tls_key '/etc/bbmd/certs/node-floor3-key.pem'
    option ca_cert '/etc/bbmd/certs/ca-cert.pem'

config telemetry 'telemetry'
    option enabled '1'

config bbmd 'bbmd'
    option enabled '0'

config logging 'logging'
    option level 'warn'
```

### Bridge Between BACnet/IP and BACnet/SC

Connect legacy BACnet/IP devices (HVAC, lighting) on the LAN to a remote BACnet/SC hub:

```
config globals 'globals'
    option device_instance '3001'
    option device_name 'BIP-BSC Gateway'
    option network_number '3'

config hub 'hub'
    option enabled '0'

config node 'node'
    option enabled '0'

config telemetry 'telemetry'
    option enabled '0'

config bbmd 'bbmd'
    option enabled '1'
    option port '47808'
    option lan_interface 'br-lan'

config logging 'logging'
    option level 'info'
```

### Full Hub with Telemetry

Hub accepting SC nodes, exposing router telemetry:

```
config globals 'globals'
    option device_instance '4001'
    option device_name 'Rooftop Hub'
    option vendor_id '123'
    option model_name 'BBMD v1'
    option firmware_rev '1.0.0'
    option network_number '10'

config hub 'hub'
    option enabled '1'
    option port '443'
    option tls_cert '/etc/bbmd/certs/hub-cert.pem'
    option tls_key '/etc/bbmd/certs/hub-key.pem'
    option ca_cert '/etc/bbmd/certs/ca-cert.pem'
    option max_nodes '50'

config node 'node'
    option enabled '0'

config telemetry 'telemetry'
    option enabled '1'
    option cpu_interval '5'
    option memory_interval '10'
    option network_interval '15'
    option uptime_interval '30'

config bbmd 'bbmd'
    option enabled '0'

config logging 'logging'
    option level 'info'
```

### BIP-Only BBMD with Tailscale (No Certs Required)

A standalone BACnet/IP BBMD. Remote clients register as foreign devices over Tailscale VPN:

```
config globals 'globals'
    option device_instance '1001'
    option device_name 'Router BBMD'
    option network_number '1'

config hub 'hub'
    option enabled '0'

config node 'node'
    option enabled '0'

config bbmd 'bbmd'
    option enabled '0'

config bip 'bip'
    option enabled '1'
    option port '47808'

config telemetry 'telemetry'
    option enabled '1'

config logging 'logging'
    option level 'info'
```

## Certificate Paths

Certificates are stored in `/etc/bbmd/certs/` by default. For each mode, the daemon loads:

| Mode | Cert Path | Key Path | CA Path |
|---|---|---|---|
| Hub | `/etc/bbmd/certs/hub-cert.pem` | `/etc/bbmd/certs/hub-key.pem` | `/etc/bbmd/certs/ca-cert.pem` |
| Node | `/etc/bbmd/certs/node-cert.pem` | `/etc/bbmd/certs/node-key.pem` | `/etc/bbmd/certs/ca-cert.pem` |

Paths are configurable via UCI. Use `bbmd-cert-gen` to generate certificates. See the [Certificate Management Guide](certificates.md) for details.

## Applying Changes

After editing `/etc/config/bbmd`:

```bash
# Reload config and restart daemon
/etc/init.d/bbmd reload
```

Or if you need a full restart:

```bash
/etc/init.d/bbmd restart
```

Changes are applied immediately via procd's reload trigger (`SIGHUP`). The daemon re-reads the UCI config file in place.

You can also apply changes via the LuCI web interface at **Services → BBMD (BACnet) → Configuration**, then click **Save & Apply**.

## Network Architecture Considerations

- **Hub port**: Port 443 is the default SC hub port. If another service (e.g. HTTPS web server) uses port 443, change the hub port to something else (e.g. 47808).
- **BACnet/IP port**: UDP port 47808 (0xBAC0) is the standard BACnet/IP port. The bridge binds to this port. Ensure no other BACnet/IP service on the router uses the same port.
- **LAN interface**: The bridge binds to `br-lan` by default. If your BACnet/IP devices are on a different interface (e.g. `eth0.2` for a VLAN), adjust `lan_interface` accordingly.
- **Firewall rules**: If the hub or bridge needs to be reachable from the WAN side, add appropriate firewall rules to allow inbound TCP (hub) or UDP (bridge) on the configured ports. For LAN-only operation, no firewall changes are needed.
- **BACnet network numbers**: If bridging multiple BACnet networks, each network segment must have a unique `network_number` in the BACnet internetwork.
- **Subprotocol path**: BACnet/SC nodes connect to the WebSocket path `/bacnet-sc`. This is handled automatically by bacnet-stack and is not configurable.
