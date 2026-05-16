#ifndef BBMD_NODE_H
#define BBMD_NODE_H

#include "bbmd_config.h"

int bbmd_node_init(bbmd_config_t *config);
int bbmd_node_run(void);
void bbmd_node_maintenance(unsigned int seconds);
void bbmd_node_stop(void);

#endif
