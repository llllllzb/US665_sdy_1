#ifndef _CONFIG_INTERFACE_H_
#define _CONFIG_INTERFACE_H_

#include "config/config_transport.h"
#include "config/config_target.h"

//配置工具 0:旧工具 1:新工具
#define NEW_CONFIG_TOOL     1       //

void config_layer_init(const ci_transport_t *transport, void *config);

void *config_load(int id);

void ci_send_packet(u32 id, u8 *packet, int size);


#endif
