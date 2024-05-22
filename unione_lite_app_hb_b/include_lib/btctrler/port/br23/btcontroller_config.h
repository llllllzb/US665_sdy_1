#ifndef _BTCONTROLLER_CONFIG_H_
#define _BTCONTROLLER_CONFIG_H_

#include "btcontroller_modules.h"

#include "ble/ll_config.h"

// #define CONFIG_LE_FEATURES \
(\
 LE_ENCRYPTION | \
 LE_CORE_V50_FEATURES \
)

#define CONFIG_LE_FEATURES      0//(LE_ENCRYPTION)

// #define CONFIG_LE_ROLES                            (LE_ADV|LE_SCAN|LE_INIT|LE_SLAVE|LE_MASTER)
// #define CONFIG_LE_ROLES                            (LE_ADV|LE_SCAN)
#define CONFIG_LE_ROLES                            (LE_ADV)

#include "classic/lmp_config.h"

#define CONFIG_CL_FEATURES

#define CONFIG_CL_EX_FEATURES


#define TWS_BLE_ESCO_CONNECT //通话过程进行连接使能


/*
 *-------------------
 * 蓝牙基带运行的模式
 */

struct lp_ws_t {
    u16 lrc_ws_inc;
    u16 lrc_ws_init;
    u16 bt_osc_ws_inc;
    u16 bt_osc_ws_init;
    u8  osc_change_mode;
};

#endif
