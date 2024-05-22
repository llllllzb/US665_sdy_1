#ifndef __UNI_BLUETOOTH_ADV_H__
#define __UNI_BLUETOOTH_ADV_H__

#include "app_config.h"
#include "typedef.h"
#include "system/event.h"

#include "le_common.h"
#include "ble_user.h"
#include "spp_user.h"
#if (RCSP_ADV_EN)
void rcsp_init();
void rcsp_dev_select(u8 type);
void function_change_inform(void);

bool common_msg_deal(u32 param, void *priv);
bool ble_msg_deal(u32 param);
bool music_msg_deal(u32 param, void *priv);
bool linein_msg_deal(u32 param);
bool rtc_msg_deal(u32 param);

void rcsp_exit(void);
u8 rcsp_get_asr_status(void);

enum {
    RCSP_BLE,
    RCSP_SPP,
};


enum {
    ANDROID,
    APPLE_IOS,
};

struct UNI_AI_VAR {
    ble_state_e  UNI_ble_status;
    struct ble_server_operation_t *rcsp_ble;
    u8 UNI_spp_status;
    struct spp_operation_t *rcsp_spp;
    volatile u8 speech_state;
    u32 feature_mask;
    u8 device_type;
    u8 phone_platform;
    void (*start_speech)(void);
    void (*stop_speech)(void);
    u8 err_report;
    volatile u8 file_browse_lock_flag;
    u32 return_msg;
    u8 spec_mode;
    struct __rcsp_user_var *rcsp_user;
    volatile u8 rcsp_run_flag;
    u8 ffr_mode;
    u16 ffr_time;
    volatile u8 wait_asr_end;
};

struct _SPEECH_OVER_DEAL {
    u8 last_task;
    u8 status;
};

extern struct UNI_AI_VAR uni_ai_var;
extern struct _SPEECH_OVER_DEAL speech_deal_val;


#define DEVICE_EVENT_FROM_RCSP	(('R' << 24) | ('C' << 16) | ('S' << 8) | 'P')

enum RCSP_MSG_T {
    MSG_UNI_GET_DEV_UPDATE_FILE_INFO_OFFSET,
    MSG_UNI_INQUIRE_DEVEICE_IF_CAN_UPDATE,
    MSG_UNI_LOADER_DOWNLOAD_START,
    MSG_UNI_UPDATE_START,
    MSG_UNI_ENTER_UPDATE_MODE,
    MSG_UNI_DEV_DISCONNECT,
    MSG_UNI_BLE_UPDATE_START,
    MSG_UNI_SPP_UPDATE_START,
    MSG_UNI_ADV_SETTING_SYNC,
    MSG_UNI_ADV_SETTING_UPDATE,
    MSG_UNI_UPDATE_EQ,
    MSG_UNI_UPDATE_SEQ,
    MSG_UNI_SWITCH_DEVICE,
    MSG_UNI_UPDATE_PLAYER_TIME,
    MSG_UNI_UPDATE_PLAYER_STATE,
    MSG_UNI_UPDATE_MUSIC_INFO,
    MSG_UNI_UPDATE_MUSIC_PLAYER_TIME_TEMER,
    MSG_UNI_UPDAET_ADV_STATE_INFO,
    MSG_UNI_REBOOT_DEV,
    MSG_UNI_FIND_DEVICE_RESUME,
    MSG_UNI_FIND_DEVICE_STOP,
};

void UNI_rcsp_event_to_user(u32 type, u8 event, u8 *msg, u8 size);
int UNI_rcsp_event_handler(struct rcsp_event *rcsp);
void rcsp_tws_auth_sync_deal(void);
extern u8 UNI_get_cur_bt_channel_sel(void);

#define		SDK_TYPE_US660P1		0x0
#define		SDK_TYPE_US662P2		0x1
#define 	SDK_TYPE_US663P1		0x2
#define 	SDK_TYPE_US665P3 	0x3
#define		SDK_TYPE_US667P1 	0x4

#if   (defined CONFIG_CPU_BR21)
#define		RCSP_SDK_TYPE		SDK_TYPE_US662P2
#elif (defined CONFIG_CPU_BR22)
#define		RCSP_SDK_TYPE		SDK_TYPE_US663P1
#elif (defined CONFIG_CPU_BR23)
#define		RCSP_SDK_TYPE		SDK_TYPE_US665P3
#elif (defined CONFIG_CPU_BR30)
#define		RCSP_SDK_TYPE		SDK_TYPE_US667P1
#else
#define		RCSP_SDK_TYPE		SDK_TYPE_US663P1
#endif

#endif
#endif
