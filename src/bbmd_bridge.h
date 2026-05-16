#ifndef BBMD_BRIDGE_H
#define BBMD_BRIDGE_H

#include "bbmd_config.h"

int bbmd_bridge_init(bbmd_config_t *config);
int bbmd_bridge_run(void);
void bbmd_bridge_maintenance(unsigned int seconds);
void bbmd_bridge_stop(void);

#endif
