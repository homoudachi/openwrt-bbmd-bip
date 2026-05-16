#ifndef BBMD_CONFIG_H
#define BBMD_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t device_instance;
    char *device_name;
    uint16_t vendor_id;
    char *model_name;
    char *firmware_rev;
    uint16_t network_number;
    uint8_t protocol_revision;
} bbmd_globals_t;

typedef struct {
    bool enabled;
    uint16_t port;
    char *tls_cert;
    char *tls_key;
    char *ca_cert;
    uint16_t max_nodes;
    uint16_t keepalive_interval;
    uint16_t keepalive_timeout;
} bbmd_hub_config_t;

typedef struct {
    bool enabled;
    char *primary_hub;
    char **failover_hubs;
    int failover_count;
    char *tls_cert;
    char *tls_key;
    char *ca_cert;
    uint16_t retry_interval;
    uint16_t max_retry_interval;
} bbmd_node_config_t;

typedef struct {
    bool enabled;
    uint16_t cpu_interval;
    uint16_t memory_interval;
    uint16_t network_interval;
    uint16_t uptime_interval;
} bbmd_telemetry_config_t;

typedef struct {
    bool enabled;
    uint16_t port;
    char *lan_interface;
} bbmd_bridge_config_t;

typedef struct {
    char *level;
} bbmd_logging_config_t;

typedef struct {
    bbmd_globals_t globals;
    bbmd_hub_config_t hub;
    bbmd_node_config_t node;
    bbmd_telemetry_config_t telemetry;
    bbmd_bridge_config_t bridge;
    bbmd_logging_config_t logging;
} bbmd_config_t;

int bbmd_config_load(bbmd_config_t *config);
void bbmd_config_free(bbmd_config_t *config);
const char *bbmd_config_str(const bbmd_config_t *config);

#endif
