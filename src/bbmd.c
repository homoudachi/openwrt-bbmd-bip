#define _GNU_SOURCE

#define BBMD_VERSION "1.0.0"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <syslog.h>

#include "bbmd_config.h"
#include "bbmd_hub.h"
#include "bbmd_node.h"
#include "bbmd_bridge.h"
#include "bbmd_telemetry.h"

typedef struct {
    bool hub_mode;
    bool node_mode;
    bool bridge_mode;
    bool telemetry_mode;
} bbmd_modes_t;

static volatile bool running = true;
static volatile bool reload_config = false;

static void signal_handler(int sig)
{
    switch (sig) {
    case SIGTERM:
    case SIGINT:
        running = false;
        break;
    case SIGHUP:
        reload_config = true;
        break;
    }
}

static void setup_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
}

static void print_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr,
            "  -f, --foreground    Run in foreground (do not daemonize)\n");
    fprintf(stderr,
            "  -c, --config DIR    Config directory"
            " (default: /etc/config)\n");
    fprintf(stderr, "  -h, --help          Show this help\n");
    fprintf(stderr, "  -v, --version       Show version\n");
}

enum {
    OPT_FOREGROUND = 0x100,
    OPT_CONFIG,
};

static const struct option long_opts[] = {
    { "foreground", no_argument, NULL, OPT_FOREGROUND },
    { "config", required_argument, NULL, OPT_CONFIG },
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'v' },
    { NULL, 0, NULL, 0 }
};

static uint64_t get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static int hub_start(bbmd_config_t *config)
{
    if (bbmd_hub_init(config) != 0) {
        syslog(LOG_ERR, "Failed to initialize BACnet/SC hub");
        return -1;
    }
    syslog(LOG_INFO, "BACnet/SC hub started on port %u",
           config->hub.port);
    return 0;
}

static void hub_reload(bbmd_config_t *config)
{
    bbmd_hub_stop();
    if (config->hub.enabled) {
        if (bbmd_hub_init(config) != 0) {
            syslog(LOG_ERR, "Failed to restart BACnet/SC hub");
        } else {
            syslog(LOG_INFO, "BACnet/SC hub restarted on port %u",
                   config->hub.port);
        }
    }
}

static int node_start(bbmd_config_t *config)
{
    if (bbmd_node_init(config) != 0) {
        syslog(LOG_ERR, "Failed to initialize BACnet/SC node");
        return -1;
    }
    syslog(LOG_INFO, "BACnet/SC node started (hub: %s)",
           config->node.primary_hub);
    return 0;
}

static void node_reload(bbmd_config_t *config)
{
    bbmd_node_stop();
    if (config->node.enabled) {
        if (bbmd_node_init(config) != 0) {
            syslog(LOG_ERR, "Failed to restart BACnet/SC node");
        } else {
            syslog(LOG_INFO, "BACnet/SC node restarted (hub: %s)",
                   config->node.primary_hub);
        }
    }
}

static int bridge_start(bbmd_config_t *config)
{
    if (bbmd_bridge_init(config) != 0) {
        syslog(LOG_ERR, "Failed to initialize bridge");
        return -1;
    }
    syslog(LOG_INFO, "BACnet/IP to BACnet/SC bridge started");
    return 0;
}

static void bridge_reload(bbmd_config_t *config)
{
    bbmd_bridge_stop();
    if (config->bridge.enabled) {
        if (bbmd_bridge_init(config) != 0) {
            syslog(LOG_ERR, "Failed to restart bridge");
        } else {
            syslog(LOG_INFO, "BACnet/IP to BACnet/SC bridge restarted");
        }
    }
}

int main(int argc, char **argv)
{
    bool foreground = false;
    const char *confdir = NULL;
    bbmd_config_t config;
    bbmd_modes_t modes = { 0 };
    uint64_t last_maintenance;
    uint64_t last_telemetry;
    int opt;

    while ((opt = getopt_long(argc, argv, "fhc:v", long_opts, NULL)) != -1) {
        switch (opt) {
        case 'f':
        case OPT_FOREGROUND:
            foreground = true;
            break;
        case 'c':
        case OPT_CONFIG:
            confdir = optarg;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        case 'v':
            fprintf(stderr, "bbmd version %s\n", BBMD_VERSION);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    if (bbmd_config_load(&config, confdir) != 0) {
        fprintf(stderr, "Failed to load configuration\n");
        return 1;
    }

    modes.hub_mode = config.hub.enabled;
    modes.node_mode = config.node.enabled;
    modes.bridge_mode = config.bridge.enabled;
    modes.telemetry_mode = config.telemetry.enabled;

    if (!modes.hub_mode && !modes.node_mode && !modes.bridge_mode) {
        fprintf(stderr, "No active modes configured\n");
        bbmd_config_free(&config);
        return 1;
    }

    if (modes.hub_mode && modes.node_mode) {
        fprintf(stderr, "Cannot run as both hub and node simultaneously\n");
        bbmd_config_free(&config);
        return 1;
    }

    if (modes.bridge_mode && modes.hub_mode) {
        fprintf(stderr, "Cannot run as both bridge and hub simultaneously\n");
        bbmd_config_free(&config);
        return 1;
    }

    if (modes.bridge_mode && modes.node_mode) {
        fprintf(stderr, "Cannot run as both bridge and node simultaneously\n");
        bbmd_config_free(&config);
        return 1;
    }

    if (!foreground) {
        if (daemon(0, 0) != 0) {
            perror("daemon");
            bbmd_config_free(&config);
            return 1;
        }
    }

    setup_signals();

    openlog("bbmd", LOG_PID | LOG_NDELAY, LOG_DAEMON);
    syslog(LOG_INFO, "bbmd %s starting", BBMD_VERSION);
    syslog(LOG_INFO, "Modes: hub=%d node=%d bridge=%d telemetry=%d",
           modes.hub_mode, modes.node_mode, modes.bridge_mode,
           modes.telemetry_mode);

    if (modes.hub_mode) {
        if (hub_start(&config) != 0) {
            syslog(LOG_ERR, "Hub start failed, exiting");
            bbmd_config_free(&config);
            closelog();
            return 1;
        }
    }

    if (modes.node_mode) {
        if (node_start(&config) != 0) {
            syslog(LOG_ERR, "Node start failed, exiting");
            bbmd_config_free(&config);
            closelog();
            return 1;
        }
    }

    if (modes.bridge_mode) {
        if (bridge_start(&config) != 0) {
            syslog(LOG_ERR, "Bridge start failed, exiting");
            bbmd_config_free(&config);
            closelog();
            return 1;
        }
    }

    if (modes.telemetry_mode) {
        if (bbmd_telemetry_init(&config) != 0) {
            syslog(LOG_WARNING, "Failed to initialize telemetry");
        } else {
            syslog(LOG_INFO, "Telemetry initialized");
        }
    }

    last_maintenance = get_time_ms();
    last_telemetry = last_maintenance;

    while (running) {
        if (reload_config) {
            reload_config = false;
            syslog(LOG_INFO, "Reloading configuration");
            bbmd_config_free(&config);
            if (bbmd_config_load(&config, confdir) != 0) {
                syslog(LOG_ERR, "Failed to reload configuration");
                running = false;
                break;
            }
            if (modes.hub_mode) {
                hub_reload(&config);
            }
            if (modes.node_mode) {
                node_reload(&config);
            }
            if (modes.bridge_mode) {
                bridge_reload(&config);
            }
            modes.hub_mode = config.hub.enabled;
            modes.node_mode = config.node.enabled;
            modes.bridge_mode = config.bridge.enabled;
            modes.telemetry_mode = config.telemetry.enabled;

            if (config.telemetry.enabled) {
                bbmd_telemetry_init(&config);
                last_telemetry = get_time_ms();
            } else {
                bbmd_telemetry_cleanup();
            }

            syslog(LOG_INFO, "Configuration reloaded");
        }

        if (modes.hub_mode || modes.node_mode || modes.bridge_mode) {
            if (modes.hub_mode) bbmd_hub_run();
            if (modes.node_mode) bbmd_node_run();
            if (modes.bridge_mode) bbmd_bridge_run();

            uint64_t now = get_time_ms();
            if (now - last_maintenance >= 1000) {
                unsigned int elapsed =
                    (unsigned int)((now - last_maintenance) / 1000);
                last_maintenance = now;
                if (modes.hub_mode) bbmd_hub_maintenance(elapsed);
                if (modes.node_mode) bbmd_node_maintenance(elapsed);
                if (modes.bridge_mode) bbmd_bridge_maintenance(elapsed);
            }
        } else {
            usleep(100000);
        }

        if (modes.telemetry_mode) {
            uint64_t now = get_time_ms();
            if (now - last_telemetry >= (uint64_t)config.telemetry.cpu_interval * 1000) {
                last_telemetry = now;
                bbmd_telemetry_update();
            }
        }
    }

    syslog(LOG_INFO, "bbmd shutting down");

    if (modes.hub_mode) {
        bbmd_hub_stop();
    }

    if (modes.node_mode) {
        bbmd_node_stop();
    }

    if (modes.bridge_mode) {
        bbmd_bridge_stop();
    }

    if (modes.telemetry_mode) {
        bbmd_telemetry_cleanup();
    }

    bbmd_config_free(&config);
    closelog();

    return 0;
}
