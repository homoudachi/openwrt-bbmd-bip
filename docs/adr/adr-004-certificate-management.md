# ADR-004: Certificate Management Strategy

**Status**: Accepted  
**Date**: 2026-05-16  
**Deciders**: Architecture review

## Context

BACnet/SC requires X.509 certificates for TLS 1.3 mutual authentication:
- CA certificate (trusted by all nodes and hubs)
- Hub certificate (presented by the hub during TLS handshake)
- Node certificates (each connecting node presents one)

bacnet-stack's BSC module has certificate support but with limitations:
- `Operational_Certificate_File` and `Issuer_Certificate_Files` are **readonly**
- No CSR (Certificate Signing Request) support
- No automatic certificate generation

## Decision

### Phase 1 (MVP): Manual Certificate Management

1. **Certificate storage**: `/etc/bbmd/certs/` directory
2. **Certificate generation**: Shell script (`cert-gen.sh`) using `openssl` CLI
   - Generates CA certificate and key
   - Generates hub certificate signed by CA
   - Generates node certificates signed by CA
3. **Configuration**: Certificate paths in UCI config (`/etc/config/bbmd`)
4. **Runtime**: bacnet-stack loads certs from filesystem at startup

### Phase 2 (Future): Automated Certificate Management

1. Integration with OpenWrt's `acme` package for Let's Encrypt
2. Automatic cert renewal with daemon reload
3. CSR generation via ubus API

## Certificate Generation Helper (`cert-gen.sh`)

```bash
#!/bin/sh
# cert-gen.sh - Generate BACnet/SC certificates for BBMD
#
# Usage: cert-gen.sh ca      # Generate CA certificate
#        cert-gen.sh hub     # Generate hub certificate
#        cert-gen.sh node <name>  # Generate node certificate

BBMD_CERT_DIR="/etc/bbmd/certs"
CA_KEY="${BBMD_CERT_DIR}/ca-key.pem"
CA_CERT="${BBMD_CERT_DIR}/ca-cert.pem"
HUB_KEY="${BBMD_CERT_DIR}/hub-key.pem"
HUB_CERT="${BBMD_CERT_DIR}/hub-cert.pem"

generate_ca() {
    openssl req -new -x509 -days 3650 -nodes \
        -keyout "$CA_KEY" -out "$CA_CERT" \
        -subj "/CN=BACnet-SC-CA/O=OpenWrt BBMD"
}

generate_hub() {
    openssl req -new -nodes -keyout "$HUB_KEY" -out /tmp/hub.csr \
        -subj "/CN=BACnet-SC-Hub/O=OpenWrt BBMD"
    openssl x509 -req -days 3650 -in /tmp/hub.csr \
        -CA "$CA_CERT" -CAkey "$CA_KEY" -CAcreateserial \
        -out "$HUB_CERT"
    rm -f /tmp/hub.csr
}

generate_node() {
    local name="$1"
    openssl req -new -nodes \
        -keyout "${BBMD_CERT_DIR}/node-${name}-key.pem" \
        -out "/tmp/node-${name}.csr" \
        -subj "/CN=BACnet-SC-Node-${name}/O=OpenWrt BBMD"
    openssl x509 -req -days 3650 \
        -in "/tmp/node-${name}.csr" \
        -CA "$CA_CERT" -CAkey "$CA_KEY" -CAcreateserial \
        -out "${BBMD_CERT_DIR}/node-${name}-cert.pem"
    rm -f "/tmp/node-${name}.csr"
}
```

## Consequences

### Positive
- Simple, well-understood certificate management
- Compatible with bacnet-stack's read-file approach
- Users can use any external CA if they prefer
- Script approach doesn't require adding certificate logic to the daemon

### Negative
- Manual cert distribution to remote nodes
- No automatic renewal in Phase 1
- bacnet-stack's readonly cert properties limit future flexibility

### Mitigations
- Document manual cert distribution clearly
- Plan Phase 2 for acme/Let's Encrypt integration
- Monitor bacnet-stack upstream for improved cert management
