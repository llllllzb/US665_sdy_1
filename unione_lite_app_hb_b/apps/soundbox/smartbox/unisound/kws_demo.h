#ifndef KWS_DEMO_H_
#define KWS_DEMO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "uni_iot.h"

int alg_unisound_init(void);
int alg_unisound_deinit();
int alg_unisound_asr_proc(void *mic);
void enable_asr_timeout(void);
void disable_asr_timeout(void);

#ifdef __cplusplus
}
#endif

#endif
