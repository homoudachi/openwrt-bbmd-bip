#!/bin/sh
# cert-gen.sh - Generate BACnet/SC certificates for BBMD
#
# BACnet/SC requires X.509 certificates for TLS 1.3 mutual authentication.
# This script generates a self-signed CA, a hub cert (signed by CA),
# and node certs (signed by CA) using P-256 ECDSA keys.
#
# Usage:
#   cert-gen.sh ca           Generate CA key and self-signed certificate
#   cert-gen.sh hub          Generate hub key and certificate signed by CA
#   cert-gen.sh node <name>  Generate node key and certificate signed by CA

set -e

BBMD_CERT_DIR="/etc/bbmd/certs"
CA_KEY="${BBMD_CERT_DIR}/ca-key.pem"
CA_CERT="${BBMD_CERT_DIR}/ca-cert.pem"
HUB_KEY="${BBMD_CERT_DIR}/hub-key.pem"
HUB_CERT="${BBMD_CERT_DIR}/hub-cert.pem"

DAYS=3650
OPENSSL_CNF="/etc/ssl/openssl.cnf"

# Try P-256 ECDSA first (preferred for embedded), fall back to RSA-2048
use_ecdsa() {
    openssl ecparam -name prime256v1 -genkey -noout 2>/dev/null
}

ensure_dir() {
    mkdir -p "$BBMD_CERT_DIR"
    chmod 750 "$BBMD_CERT_DIR"
}

generate_ca() {
    ensure_dir
    echo "Generating CA key..."
    if use_ecdsa; then
        openssl ecparam -name prime256v1 -genkey -noout -out "$CA_KEY"
        echo "  ECDSA P-256 key: $CA_KEY"
    else
        openssl genrsa -out "$CA_KEY" 2048
        echo "  RSA-2048 key: $CA_KEY"
    fi
    chmod 600 "$CA_KEY"

    echo "Generating CA certificate (self-signed, ${DAYS} days)..."
    openssl req -new -x509 -days "$DAYS" -nodes \
        -key "$CA_KEY" -out "$CA_CERT" \
        -subj "/CN=BACnet-SC-CA/O=OpenWrt BBMD" \
        -extensions v3_ca
    chmod 644 "$CA_CERT"
    echo "  CA certificate: $CA_CERT"
    echo "Done."
}

generate_hub() {
    ensure_dir
    if [ ! -f "$CA_CERT" ] || [ ! -f "$CA_KEY" ]; then
        echo "Error: CA certificate and key must exist before generating hub cert."
        echo "Run '$0 ca' first."
        exit 1
    fi

    echo "Generating hub key..."
    if use_ecdsa; then
        openssl ecparam -name prime256v1 -genkey -noout -out "$HUB_KEY"
        echo "  ECDSA P-256 key: $HUB_KEY"
    else
        openssl genrsa -out "$HUB_KEY" 2048
        echo "  RSA-2048 key: $HUB_KEY"
    fi
    chmod 600 "$HUB_KEY"

    echo "Generating hub certificate signed by CA..."
    openssl req -new -nodes -key "$HUB_KEY" -out /tmp/hub.csr \
        -subj "/CN=BACnet-SC-Hub/O=OpenWrt BBMD"

    openssl x509 -req -days "$DAYS" -in /tmp/hub.csr \
        -CA "$CA_CERT" -CAkey "$CA_KEY" -CAcreateserial \
        -out "$HUB_CERT"
    chmod 644 "$HUB_CERT"

    rm -f /tmp/hub.csr
    echo "  Hub certificate: $HUB_CERT"
    echo "Done."
}

generate_node() {
    local name="$1"
    if [ -z "$name" ]; then
        echo "Error: node name is required."
        echo "Usage: $0 node <name>"
        exit 1
    fi

    ensure_dir
    if [ ! -f "$CA_CERT" ] || [ ! -f "$CA_KEY" ]; then
        echo "Error: CA certificate and key must exist before generating node cert."
        echo "Run '$0 ca' first."
        exit 1
    fi

    local node_key="${BBMD_CERT_DIR}/node-${name}-key.pem"
    local node_cert="${BBMD_CERT_DIR}/node-${name}-cert.pem"

    echo "Generating node '${name}' key..."
    if use_ecdsa; then
        openssl ecparam -name prime256v1 -genkey -noout -out "$node_key"
        echo "  ECDSA P-256 key: $node_key"
    else
        openssl genrsa -out "$node_key" 2048
        echo "  RSA-2048 key: $node_key"
    fi
    chmod 600 "$node_key"

    echo "Generating node '${name}' certificate signed by CA..."
    openssl req -new -nodes -key "$node_key" -out "/tmp/node-${name}.csr" \
        -subj "/CN=BACnet-SC-Node-${name}/O=OpenWrt BBMD"

    openssl x509 -req -days "$DAYS" -in "/tmp/node-${name}.csr" \
        -CA "$CA_CERT" -CAkey "$CA_KEY" -CAcreateserial \
        -out "$node_cert"
    chmod 644 "$node_cert"

    rm -f "/tmp/node-${name}.csr"
    echo "  Node certificate: $node_cert"
    echo "Done."
}

case "${1:-help}" in
    ca)
        generate_ca
        ;;
    hub)
        generate_hub
        ;;
    node)
        generate_node "$2"
        ;;
    *)
        echo "Usage: $0 {ca|hub|node <name>}"
        echo ""
        echo "  ca           Generate CA key and self-signed certificate"
        echo "  hub          Generate hub key and certificate signed by CA"
        echo "  node <name>  Generate node key and certificate signed by CA"
        echo ""
        echo "All certificates and keys are stored in: $BBMD_CERT_DIR"
        exit 1
        ;;
esac
