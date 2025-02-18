#ifndef _CONFIG_TARGET_H_
#define _CONFIG_TARGET_H_

#include "typedef.h"


#define EQ_CONFIG_ID        0x0005
#define EFFECTS_CONFIG_ID  	0x0006
#define AEC_CONFIG_ID   	0x0008


typedef void (*ci_packet_handler_t)(uint8_t *packet, uint16_t size);

struct config_target {
    u16 id;
    ci_packet_handler_t callback;
};

#define REGISTER_CONFIG_TARGET(target) \
        const struct config_target target sec(.config_target)


extern const struct config_target config_target_begin[];
extern const struct config_target config_target_end[];

#define list_for_each_config_target(p) \
    for (p = config_target_begin; p < config_target_end; p++)


#endif
