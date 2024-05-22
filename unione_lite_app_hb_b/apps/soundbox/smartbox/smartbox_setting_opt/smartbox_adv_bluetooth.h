#ifndef __SMARTBOX_ADV_BLUETOOTH__
#define __SMARTBOX_ADV_BLUETOOTH__
#include "typedef.h"
#include "system/event.h"


#define		SDK_TYPE_US660P1				0x0
#define		SDK_TYPE_US662P2				0x1
#define 	SDK_TYPE_US663P1				0x2
#define 	SDK_TYPE_US665P3 			0x3
#define		SDK_TYPE_US667P1 			0x4
#define		SDK_TYPE_US666P1 			0x5
#define		SDK_TYPE_US666P1_TWS			0x6
#define 	SDK_TYPE_US665P3_SOUNDCARD	0x7

#if   (defined CONFIG_CPU_BR21)
#define		RCSP_SDK_TYPE		SDK_TYPE_US662P2
#elif (defined CONFIG_CPU_BR22)
#define		RCSP_SDK_TYPE		SDK_TYPE_US663P1
#elif (defined CONFIG_CPU_BR23 && SOUNDCARD_ENABLE)
#define		RCSP_SDK_TYPE		SDK_TYPE_US665P3_SOUNDCARD
#elif (defined CONFIG_CPU_BR23)
#define		RCSP_SDK_TYPE		SDK_TYPE_US666P1//SDK_TYPE_US665P3
#elif (defined CONFIG_CPU_BR25)
#define		RCSP_SDK_TYPE		SDK_TYPE_US666P1
#elif (defined CONFIG_CPU_BR30)
#define		RCSP_SDK_TYPE		SDK_TYPE_US667P1
#else
#define		RCSP_SDK_TYPE		SDK_TYPE_US663P1
#endif

// enum RCSP_ADV_MSG_T {
//     MSG_UNI_ADV_SETTING_SYNC,
//     MSG_UNI_ADV_SETTING_UPDATE,
// };
//
// bool smartbox_adv_msg_post(int msg, int argc, ...);
int UNI_smartbox_adv_event_handler(struct rcsp_event *rcsp);
int UNI_smartbox_adv_cmd_resp(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len);

#endif
