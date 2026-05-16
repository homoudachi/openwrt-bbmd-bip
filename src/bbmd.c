#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <syslog.h>

#include "bbmd_config.h"

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
    fprintf(stderr, "  -f, --foreground    Run in foreground (do not daemonize)\n");
    fprintf(stderr, "  -c, --config FILE   Config file path (default: /etc/config/bbmd)\n");
    fprintf(stderr, "  -h, --help          Show this help\n");
    fprintf(stderr, "  -v, --version       Show version\n");
}

enum {
    OPT_FOREGROUND = 0x100,
    OPT_CONFIG,
};

static const struct option long_opts[] = {
    {"foreground", no_argument,       NULL, OPT_FOREGROUND},
    {"config",     required_argument, NULL, OPT_CONFIG},
    {"help",       no_argument,       NULL, 'h'},
    {"version",    no_argument,       NULL, 'v'},
    {NULL, 0, NULL, 0}
};

int main(int argc, char **argv)
{
    bool foreground = false;
    const char *config_path = "/etc/config/bbmd";
    bbmd_config_t config;
    bbmd_modes_t modes = {0};
    int opt;

    while ((opt = getopt_long(argc, argv, "fhc:v", long_opts, NULL)) != -1) {
        switch (opt) {
        case 'f':
        case OPT_FOREGROUND:
            foreground = true;
            break;
        case 'c':
        case OPT_CONFIG:
            config_path = optarg;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        case 'v':
            fprintf(stderr, "bbmd version 1.0.0\n");
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    if (bbmd_config_load(&config, config_path) != 0) {
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

    if (!foreground) {
        if (daemon(0, 0) != 0) {
            perror("daemon");
            bbmd_config_free(&config);
            return 1;
        }
    }

    setup_signals();

    openlog("bbmd", LOG_PID | LOG_NDELAY, LOG_DAEMON);
    syslog(LOG_INFO, "bbmd %s starting", "1.0.0");
    syslog(LOG_INFO, "Modes: hub=%d node=%d bridge=%d telemetry=%d",
           modes.hub_mode, modes.node_mode, modes.bridge_mode, modes.telemetry_mode);

    while (running) {
        if (reload_config) {
            reload_config = false;
            syslog(LOG_INFO, "Reloading configuration");
            bbmd_config_free(&config);
            if (bbmd_config_load(&config, config_path) != 0) {
                syslog(LOG_ERR, "Failed to reload configuration");
                running = false;
                break;
            }
            modes.hub_mode = config.hub.enabled;
            modes.node_mode = config.node.enabled;
            modes.bridge_mode = config.bridge.enabled;
            modes.telemetry_mode = config.telemetry.enabled;
            syslog(LOG_INFO, "Configuration reloaded");
        }

        usleep(100000);
    }

    syslog(LOG_INFO, "bbmd shutting down");

    bbmd_config_free(&config);
    closelog();

    return 0;
}
