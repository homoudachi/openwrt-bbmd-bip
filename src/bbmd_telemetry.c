#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <syslog.h>

#include "bacnet/bacenum.h"
#include "bacnet/basic/object/ai.h"

#include "bbmd_config.h"
#include "bbmd_telemetry.h"

static bool telemetry_initialized = false;

int bbmd_telemetry_init(bbmd_config_t *config)
{
    if (telemetry_initialized) {
        return 0;
    }

    if (!config || !config->telemetry.enabled) {
        return 0;
    }

    if (Analog_Input_Create(0) == BACNET_MAX_INSTANCE) {
        syslog(LOG_ERR, "telemetry: failed to create AI 0 (CPU)");
        return -1;
    }
    Analog_Input_Units_Set(0, UNITS_PERCENT);

    if (Analog_Input_Create(1) == BACNET_MAX_INSTANCE) {
        syslog(LOG_ERR, "telemetry: failed to create AI 1 (Memory)");
        return -1;
    }
    Analog_Input_Units_Set(1, UNITS_PERCENT);

    if (Analog_Input_Create(2) == BACNET_MAX_INSTANCE) {
        syslog(LOG_ERR, "telemetry: failed to create AI 2 (Uptime)");
        return -1;
    }
    Analog_Input_Units_Set(2, UNITS_SECONDS);

    telemetry_initialized = true;
    return 0;
}

int bbmd_telemetry_update(void)
{
    static uint64_t prev_total = 0, prev_idle = 0;
    FILE *fp;
    unsigned long user, nice, system, idle, iowait, irq, softirq, steal;
    int count;
    uint64_t total, idle_total;
    float cpu_pct, mem_pct, uptime_secs;

    if (!telemetry_initialized) {
        return 0;
    }

    cpu_pct = 0.0f;
    fp = fopen("/proc/stat", "r");
    if (fp) {
        count = fscanf(fp, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
                       &user, &nice, &system, &idle, &iowait,
                       &irq, &softirq, &steal);
        fclose(fp);

        if (count >= 4) {
            total = (uint64_t)user + nice + system + idle
                  + iowait + irq + softirq + steal;
            idle_total = (uint64_t)idle + iowait;

            if (prev_total > 0 && total > prev_total) {
                uint64_t total_delta = total - prev_total;
                uint64_t idle_delta = idle_total - prev_idle;
                cpu_pct = 100.0f * (1.0f
                         - (float)idle_delta / (float)total_delta);
                if (cpu_pct < 0.0f) cpu_pct = 0.0f;
                if (cpu_pct > 100.0f) cpu_pct = 100.0f;
            }

            prev_total = total;
            prev_idle = idle_total;
        }
    } else {
        syslog(LOG_WARNING, "telemetry: cannot open /proc/stat");
    }

    mem_pct = 0.0f;
    fp = fopen("/proc/meminfo", "r");
    if (fp) {
        unsigned long mem_total = 0, mem_avail = 0;
        char line[256];

        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "MemTotal: %lu kB", &mem_total) >= 1)
                continue;
            if (sscanf(line, "MemAvailable: %lu kB", &mem_avail) >= 1)
                continue;
        }
        fclose(fp);

        if (mem_total > 0) {
            mem_pct = 100.0f * (1.0f - (float)mem_avail / (float)mem_total);
            if (mem_pct < 0.0f) mem_pct = 0.0f;
            if (mem_pct > 100.0f) mem_pct = 100.0f;
        }
    } else {
        syslog(LOG_WARNING, "telemetry: cannot open /proc/meminfo");
    }

    uptime_secs = 0.0f;
    fp = fopen("/proc/uptime", "r");
    if (fp) {
        double uptime;
        if (fscanf(fp, "%lf", &uptime) == 1) {
            uptime_secs = (float)uptime;
        }
        fclose(fp);
    } else {
        syslog(LOG_WARNING, "telemetry: cannot open /proc/uptime");
    }

    Analog_Input_Present_Value_Set(0, cpu_pct);
    Analog_Input_Present_Value_Set(1, mem_pct);
    Analog_Input_Present_Value_Set(2, uptime_secs);

    return 0;
}

void bbmd_telemetry_cleanup(void)
{
    telemetry_initialized = false;
}
