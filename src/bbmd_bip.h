#ifndef BBMD_BIP_H
#define BBMD_BIP_H

#include "bbmd_config.h"

int  bbmd_bip_init(bbmd_config_t *config);
int  bbmd_bip_run(void);
void bbmd_bip_maintenance(unsigned int seconds);
void bbmd_bip_stop(void);

#endif
