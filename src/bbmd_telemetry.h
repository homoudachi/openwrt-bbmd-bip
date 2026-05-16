#ifndef BBMD_TELEMETRY_H
#define BBMD_TELEMETRY_H

#include "bbmd_config.h"

int  bbmd_telemetry_init(bbmd_config_t *config);
int  bbmd_telemetry_update(void);
void bbmd_telemetry_cleanup(void);

#endif
