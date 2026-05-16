# Certificate Management — BACnet/SC BBMD

## 1. Overview

BACnet/SC (Secure Connect) requires TLS 1.3 mutual authentication between hubs and nodes. Every connection uses X.509 certificates:

- **CA certificate** — Self-signed root of trust; signs all hub and node certificates.
- **Hub certificate** — Presented by the BBMD hub to connecting nodes during TLS handshake.
- **Node certificate** — Presented by each connecting node. Must be signed by the same CA the hub trusts.

The daemon reads certificates from `/etc/bbmd/certs/` (configurable via UCI).

## 2. Using `bbmd-cert-gen`

After installing the `bbmd` package, the certificate generation helper is available at `/usr/sbin/bbmd-cert-gen`.

All commands store output in `/etc/bbmd/certs/`. The script prefers ECDSA P-256 keys and falls back to RSA-2048 if P-256 is unavailable.

### 2.1 Generate CA

```bash
bbmd-cert-gen ca
```

Creates:
- `/etc/bbmd/certs/ca-key.pem` (private key, mode 600)
- `/etc/bbmd/certs/ca-cert.pem` (self-signed certificate, mode 644)

### 2.2 Generate Hub Certificate

```bash
bbmd-cert-gen hub
```

Requires CA key and certificate to exist. Creates:
- `/etc/bbmd/certs/hub-key.pem` (private key, mode 600)
- `/etc/bbmd/certs/hub-cert.pem` (signed by CA, mode 644)

### 2.3 Generate Node Certificate

```bash
bbmd-cert-gen node <name>
```

Requires CA key and certificate to exist. Creates:
- `/etc/bbmd/certs/node-<name>-key.pem` (private key, mode 600)
- `/etc/bbmd/certs/node-<name>-cert.pem` (signed by CA, mode 644)

## 3. Deploying Certificates to Nodes

1. Generate certificates on the hub (or a secure management host).
2. Copy the appropriate node certificate and key to each remote router:

```bash
scp /etc/bbmd/certs/node-myrouter-key.pem root@remote-router:/etc/bbmd/certs/
scp /etc/bbmd/certs/node-myrouter-cert.pem root@remote-router:/etc/bbmd/certs/
scp /etc/bbmd/certs/ca-cert.pem root@remote-router:/etc/bbmd/certs/
```

3. Configure the UCI paths on the remote node (`/etc/config/bbmd`):

```
config node 'node'
    option enabled '1'
    option primary_hub 'wss://hub-address/bacnet-sc'
    option tls_cert '/etc/bbmd/certs/node-myrouter-cert.pem'
    option tls_key '/etc/bbmd/certs/node-myrouter-key.pem'
    option ca_cert '/etc/bbmd/certs/ca-cert.pem'
```

## 4. Deploying Certificates from an External CA

If your organisation uses a public or internal CA:

1. Obtain a signed certificate and private key for the hub and each node.
2. Copy the CA certificate (from your CA), the hub/node certificate, and the private key to `/etc/bbmd/certs/` on each device.
3. Update UCI config paths accordingly.

```bash
# Example: using certs from an enterprise CA
cp my-ca-cert.pem /etc/bbmd/certs/ca-cert.pem
cp my-hub-cert.pem /etc/bbmd/certs/hub-cert.pem
cp my-hub-key.pem  /etc/bbmd/certs/hub-key.pem
```

Ensure the Common Name (CN) or Subject Alternative Name (SAN) in the hub certificate matches the hub's hostname or IP address that connecting nodes will use.

## 5. File Permissions

| File | Permission | Reason |
|---|---|---|
| Private keys (`*-key.pem`) | `600` (owner read/write only) | Must not be readable by other users |
| Certificate files (`*-cert.pem`, `ca-cert.pem`) | `644` (world-readable) | Public certificates used for peer verification |
| Certificate directory (`/etc/bbmd/certs/`) | `750` (owner rwx, group rx) | Prevents unprivileged processes from listing keys |
