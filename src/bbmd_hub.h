#ifndef BBMD_HUB_H
#define BBMD_HUB_H

#include "bbmd_config.h"

int bbmd_hub_init(bbmd_config_t *config);
int bbmd_hub_run(void);
void bbmd_hub_maintenance(unsigned int seconds);
void bbmd_hub_stop(void);

#endif
