#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <uci.h>

#include "bbmd_config.h"

#define UCI_PKG "bbmd"

static void str_set(char **dst, const char *src)
{
    free(*dst);
    *dst = src ? strdup(src) : NULL;
}

static void defaults_init(bbmd_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    cfg->globals.device_instance = 4194303;
    str_set(&cfg->globals.device_name, "OpenWrt Router");
    cfg->globals.network_number = 1;
    cfg->globals.protocol_revision = 24;

    cfg->hub.enabled = true;
    cfg->hub.port = 443;
    cfg->hub.max_nodes = 50;
    cfg->hub.keepalive_interval = 30;
    cfg->hub.keepalive_timeout = 90;

    cfg->node.enabled = false;
    cfg->node.retry_interval = 5;
    cfg->node.max_retry_interval = 60;

    cfg->telemetry.enabled = true;
    cfg->telemetry.cpu_interval = 10;
    cfg->telemetry.memory_interval = 10;
    cfg->telemetry.network_interval = 30;
    cfg->telemetry.uptime_interval = 60;

    cfg->bridge.enabled = true;
    cfg->bridge.port = 47808;
    str_set(&cfg->bridge.lan_interface, "br-lan");

    str_set(&cfg->logging.level, "info");
}

static int read_str(struct uci_context *ctx, const char *path, char **dst)
{
    struct uci_ptr ptr = {0};
    if (uci_lookup_ptr(ctx, &ptr, (char *)path, false) != 0 || !ptr.o ||
        !ptr.o->v.string) {
        return -1;
    }
    str_set(dst, ptr.o->v.string);
    return 0;
}

static int read_u32(struct uci_context *ctx, const char *path, uint32_t *dst)
{
    struct uci_ptr ptr = {0};
    if (uci_lookup_ptr(ctx, &ptr, (char *)path, false) != 0 || !ptr.o ||
        !ptr.o->v.string) {
        return -1;
    }
    *dst = (uint32_t)strtoul(ptr.o->v.string, NULL, 10);
    return 0;
}

static int read_u16(struct uci_context *ctx, const char *path, uint16_t *dst)
{
    struct uci_ptr ptr = {0};
    if (uci_lookup_ptr(ctx, &ptr, (char *)path, false) != 0 || !ptr.o ||
        !ptr.o->v.string) {
        return -1;
    }
    *dst = (uint16_t)strtoul(ptr.o->v.string, NULL, 10);
    return 0;
}

static int read_bool(struct uci_context *ctx, const char *path, bool *dst)
{
    struct uci_ptr ptr = {0};
    if (uci_lookup_ptr(ctx, &ptr, (char *)path, false) != 0 || !ptr.o ||
        !ptr.o->v.string) {
        return -1;
    }
    const char *s = ptr.o->v.string;
    *dst = (strcmp(s, "1") == 0 || strcasecmp(s, "true") == 0 ||
            strcasecmp(s, "yes") == 0 || strcasecmp(s, "on") == 0);
    return 0;
}

static int read_str_list(struct uci_context *ctx, const char *path,
                         char ***list_out, int *count_out)
{
    struct uci_ptr ptr = {0};
    struct uci_element *e;
    int n = 0, cap = 0;
    char **arr = NULL;

    if (uci_lookup_ptr(ctx, &ptr, (char *)path, false) != 0 || !ptr.o)
        return -1;
    if (ptr.o->type != UCI_TYPE_LIST)
        return -1;

    uci_foreach_element(&ptr.o->v.list, e) {
        if (n >= cap) {
            cap = cap ? cap * 2 : 4;
            char **tmp = realloc(arr, (size_t)cap * sizeof(char *));
            if (!tmp)
                goto fail;
            arr = tmp;
        }
        arr[n] = strdup(e->name);
        if (!arr[n])
            goto fail;
        n++;
    }

    *list_out = arr;
    *count_out = n;
    return 0;

fail:
    for (int i = 0; i < n; i++)
        free(arr[i]);
    free(arr);
    return -1;
}

int bbmd_config_load(bbmd_config_t *config)
{
    struct uci_context *ctx;
    char path[128];

    defaults_init(config);

    ctx = uci_alloc_context();
    if (!ctx)
        return 0;

#define MKP(section, option) \
    snprintf(path, sizeof(path), "%s.%s.%s", UCI_PKG, section, option); \
    path

    read_u32(ctx, MKP("globals", "device_instance"),
             &config->globals.device_instance);
    read_str(ctx, MKP("globals", "device_name"),
             &config->globals.device_name);
    read_u16(ctx, MKP("globals", "vendor_id"),
             &config->globals.vendor_id);
    read_str(ctx, MKP("globals", "model_name"),
             &config->globals.model_name);
    read_str(ctx, MKP("globals", "firmware_rev"),
             &config->globals.firmware_rev);
    read_u16(ctx, MKP("globals", "network_number"),
             &config->globals.network_number);

    read_bool(ctx, MKP("hub", "enabled"),
              &config->hub.enabled);
    read_u16(ctx, MKP("hub", "port"),
             &config->hub.port);
    read_str(ctx, MKP("hub", "tls_cert"),
             &config->hub.tls_cert);
    read_str(ctx, MKP("hub", "tls_key"),
             &config->hub.tls_key);
    read_str(ctx, MKP("hub", "ca_cert"),
             &config->hub.ca_cert);
    read_u16(ctx, MKP("hub", "max_nodes"),
             &config->hub.max_nodes);
    read_u16(ctx, MKP("hub", "keepalive_interval"),
             &config->hub.keepalive_interval);
    read_u16(ctx, MKP("hub", "keepalive_timeout"),
             &config->hub.keepalive_timeout);

    read_bool(ctx, MKP("node", "enabled"),
              &config->node.enabled);
    read_str(ctx, MKP("node", "primary_hub"),
             &config->node.primary_hub);
    read_str(ctx, MKP("node", "tls_cert"),
             &config->node.tls_cert);
    read_str(ctx, MKP("node", "tls_key"),
             &config->node.tls_key);
    read_str(ctx, MKP("node", "ca_cert"),
             &config->node.ca_cert);
    read_u16(ctx, MKP("node", "retry_interval"),
             &config->node.retry_interval);
    read_u16(ctx, MKP("node", "max_retry_interval"),
             &config->node.max_retry_interval);

    read_str_list(ctx, MKP("node", "failover_hub"),
                  &config->node.failover_hubs,
                  &config->node.failover_count);

    read_bool(ctx, MKP("telemetry", "enabled"),
              &config->telemetry.enabled);
    read_u16(ctx, MKP("telemetry", "cpu_interval"),
             &config->telemetry.cpu_interval);
    read_u16(ctx, MKP("telemetry", "memory_interval"),
             &config->telemetry.memory_interval);
    read_u16(ctx, MKP("telemetry", "network_interval"),
             &config->telemetry.network_interval);
    read_u16(ctx, MKP("telemetry", "uptime_interval"),
             &config->telemetry.uptime_interval);

    read_bool(ctx, MKP("bbmd", "enabled"),
              &config->bridge.enabled);
    read_u16(ctx, MKP("bbmd", "port"),
             &config->bridge.port);
    read_str(ctx, MKP("bbmd", "lan_interface"),
             &config->bridge.lan_interface);

    read_str(ctx, MKP("logging", "level"),
             &config->logging.level);

#undef MKP

    uci_free_context(ctx);
    return 0;
}

void bbmd_config_free(bbmd_config_t *config)
{
    if (!config)
        return;

    free(config->globals.device_name);
    free(config->globals.model_name);
    free(config->globals.firmware_rev);

    free(config->hub.tls_cert);
    free(config->hub.tls_key);
    free(config->hub.ca_cert);

    free(config->node.primary_hub);
    for (int i = 0; i < config->node.failover_count; i++)
        free(config->node.failover_hubs[i]);
    free(config->node.failover_hubs);
    free(config->node.tls_cert);
    free(config->node.tls_key);
    free(config->node.ca_cert);

    free(config->bridge.lan_interface);

    free(config->logging.level);

    memset(config, 0, sizeof(*config));
}

const char *bbmd_config_str(const bbmd_config_t *config)
{
    static char buf[512];

    snprintf(buf, sizeof(buf),
        "bbmd_config(dev_inst=%" PRIu32 " dev_name=%s hub=%s port=%u "
        "bip_port=%u log=%s)",
        config->globals.device_instance,
        config->globals.device_name ? config->globals.device_name : "(null)",
        config->hub.enabled ? "on" : "off",
        config->hub.port,
        config->bridge.port,
        config->logging.level ? config->logging.level : "(null)");

    return buf;
}
