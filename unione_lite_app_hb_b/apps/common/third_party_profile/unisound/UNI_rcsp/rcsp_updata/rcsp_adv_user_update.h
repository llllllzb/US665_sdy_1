#ifndef _RCSP_ADV_USER_UPDATE_H_
#define _RCSP_ADV_USER_UPDATE_H_

//#include "rcsp_protocol.h"
//#include "rcsp_packet.h"
#include "typedef.h"

#define UNI_OPCODE_GET_DEVICE_UPDATE_FILE_INFO_OFFSET    0xe1
#define UNI_OPCODE_INQUIRE_DEVICE_IF_CAN_UPDATE          0xe2
#define UNI_OPCODE_ENTER_UPDATE_MODE                     0xe3
#define UNI_OPCODE_EXIT_UPDATE_MODE                      0xe4
#define UNI_OPCODE_SEND_FW_UPDATE_BLOCK                  0xe5
#define UNI_OPCODE_GET_DEVICE_REFRESH_FW_STATUS          0xe6
#define UNI_OPCODE_SET_DEVICE_REBOOT                     0xe7
#define UNI_OPCODE_NOTIFY_UPDATE_CONENT_SIZE				0xe8


void UNI_rcsp_update_cmd_resp(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len);
void UNI_rcsp_update_cmd_receive_resp(void *priv, u8 OpCode, u8 status, u8 *data, u16 len);
void UNI_rcsp_msg_deal(void *hdl, u8 event, u8 *msg);
u8 get_uni_update_flag(void);
void set_uni_update_flag(u8 flag);
u8 get_curr_device_type(void);
void update_slave_adv_reopen(void);

#endif

