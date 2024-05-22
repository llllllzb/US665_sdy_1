
#ifndef _UNI_RCSP_LIB_API_H_
#define _UNI_RCSP_LIB_API_H_

#include "typedef.h"
#include "uart.h"

#define USE_LITTLE_ENDIAN  0
#define USE_BIG_ENDIAN     1

#define USE_ENDIAN_TYPE    USE_LITTLE_ENDIAN


#define AI_LICENCE_LEN    16

enum {
    TULING = 0,
    DEEPBRAIN,
};

#pragma pack(1)
struct _AI_platform {
    u8 platform;
    u8 license[AI_LICENCE_LEN];
};
#pragma pack()

u16 app_htons(u16 n);
u16 app_ntohs(u16 n);

u32 app_htonl(u32 n);
u32 app_ntohl(u32 n);

void UNI_rcsp_auth_init(int (*send)(void *, u8 *, u16), u8 *link_key, u8 *addr);
void UNI_rcsp_auth_reset(void);
u8 UNI_rcsp_get_auth_flag(void);
void UNI_rcsp_set_auth_flag(u8 auth_flag);
void UNI_rcsp_auth_recieve(u8 *buffer, u16 len);

u8 get_rcsp_version(void);
void get_ai_platform(struct _AI_platform *p, u8 platform);
void smart_auth_res_pass(void);
#endif //_UNI_RCSP_LIB_H_
