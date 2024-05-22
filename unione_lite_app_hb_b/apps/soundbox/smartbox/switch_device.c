#include "smartbox/config.h"
#include "smartbox/feature.h"
#include "btstack/avctp_user.h"
#include "smartbox/event.h"
#include "smartbox_rcsp_manage.h"
#include "smartbox_bt_manage.h"
#include "smartbox_update.h"

#include "app_main.h"

#if (SMART_BOX_EN)

/* #define RCSP_DEBUG_EN */

#ifdef  RCSP_DEBUG_EN
#define rcsp_putchar(x)                putchar(x)
#define rcsp_printf                    printf
#define rcsp_printf_buf(x,len)         printf_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_printf_buf(...)
#endif

void smartbox_switch_device(u8 *data)
{
#if RCSP_UPDATE_EN
    if (get_uni_update_flag()) {
        if (RCSP_BLE == bt_3th_get_cur_bt_channel_sel()) {
            rcsp_printf("BLE_ CON START DISCON\n");
            UNI_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_UNI_DEV_DISCONNECT, NULL, 0);
        } else {
            rcsp_printf("WAIT_FOR_SPP_DISCON\n");
        }
    } else {
        UNI_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_UNI_SWITCH_DEVICE, &data[0], 1);
    }
#else
    UNI_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_UNI_SWITCH_DEVICE, &data[0], 1);
#endif
}
#endif//SMART_BOX_EN


