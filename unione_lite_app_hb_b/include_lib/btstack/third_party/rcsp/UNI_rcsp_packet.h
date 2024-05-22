#ifndef __UNI_PACKET_H__
#define __UNI_PACKET_H__

#include "typedef.h"


#define READ_BIG_U16(a)   ((*((u8*)(a)) <<8)  + *((u8*)(a)+1))
#define READ_BIG_U32(a)   ((*((u8*)(a)) <<24) + (*((u8*)(a)+1)<<16) + (*((u8*)(a)+2)<<8) + *((u8*)(a)+3))

#define READ_LIT_U16(a)   (*((u8*)(a))  + (*((u8*)(a)+1)<<8))
#define READ_LIT_U32(a)   (*((u8*)(a))  + (*((u8*)(a)+1)<<8) + (*((u8*)(a)+2)<<16) + (*((u8*)(a)+3)<<24))


#define WRITE_BIG_U16(a,src)   {*((u8*)(a)+0) = (u8)(src>>8); *((u8*)(a)+1) = (u8)(src&0xff); }
#define WRITE_BIG_U32(a,src)   {*((u8*)(a)+0) = (u8)((src)>>24);  *((u8*)(a)+1) = (u8)(((src)>>16)&0xff);*((u8*)(a)+2) = (u8)(((src)>>8)&0xff);*((u8*)(a)+3) = (u8)((src)&0xff);}

#define WRITE_LIT_U16(a,src)   {*((u8*)(a)+1) = (u8)(src>>8); *((u8*)(a)+0) = (u8)(src&0xff); }
#define WRITE_LIT_U32(a,src)   {*((u8*)(a)+3) = (u8)((src)>>24);  *((u8*)(a)+2) = (u8)(((src)>>16)&0xff);*((u8*)(a)+1) = (u8)(((src)>>8)&0xff);*((u8*)(a)+0) = (u8)((src)&0xff);}


#pragma pack(1)
typedef union __HEAD_BIT {
    struct {
        u16 _OpCode: 8; //OpCode val
        u16 _unsed : 6; //unsed
        u16 _resp : 1; //request for response
        u16 _type : 1; //command or response
    } _i;
    u16         _t;
} HEAD_BIT;

struct __UNI_PACKET {
    u8          tag[3];
    HEAD_BIT    head;
    u16         length;
    u8          data[0];
};
#pragma pack()
typedef struct __UNI_PACKET UNI_PACKET;


#define UNI_PACK_START_TAG0          (0xfe)
#define UNI_PACK_START_TAG1          (0xdc)
#define UNI_PACK_START_TAG2          (0xba)
#define UNI_PACK_END_TAG             (0xef)
#define UNI_ONE_PACKET_LEN(n)        (sizeof(UNI_PACKET) + n + 1)

#ifdef UNI_RCSP_UBOOT_LIB
#define UNI_MTU_RESV                 (540L)
#define UNI_MTU_SEND                 (128L)
#define UNI_RECIEVE_BUF_SIZE         ((UNI_MTU_RESV + sizeof(UNI_PACKET))*2)
#define UNI_CMD_POOL_SIZE            (UNI_MTU_SEND)
#define UNI_RESP_POOL_SIZE           (UNI_MTU_SEND*2)
#define UNI_WAIT_RESP_POOL_SIZE      (UNI_MTU_SEND)
#else
#define UNI_MTU_RESV                 (540L)
#define UNI_MTU_SEND                 (128L)
#define UNI_RECIEVE_BUF_SIZE         (UNI_MTU_RESV + sizeof(UNI_PACKET) + 128)
#define UNI_CMD_POOL_SIZE            (UNI_MTU_SEND*4)
#define UNI_RESP_POOL_SIZE           (UNI_MTU_SEND*2)
#define UNI_WAIT_RESP_POOL_SIZE      (UNI_MTU_SEND*2)
#endif

void set_uni_rcsp_recieve_buf_size(u16 size);
u16 rcsp_packet_write_alloc_len(void);

u32 rcsp_packet_need_buf_size(void);
u32 rcsp_packet_buf_init(u8 *buf, u32 len);

u16 UNI_packet_get_rx_max_mtu(void);
u16 UNI_packet_get_tx_max_mtu(void);
u16 UNI_packet_set_mtu(u16 mtu);
void UNI_packet_recieve(void *buf, u16 len);
u32 UNI_pack_data_read_all(void *buf, u16 len);
void UNI_packet_clear_all_data(void);
bool UNI_packet_find(u8 *r_buf, UNI_PACKET **packet);
void UNI_packet_init(void);
void UNI_packet_clear(void);

void UNI_packet_packing(
    UNI_PACKET *packet,
    u8 OpCode,
    u8 type,
    u8 request_rsp,
    u8 *extra_param,
    u16 extra_len,
    u8 *data,
    u16 len);

void set_uni_mtu_resv(u16 uni_mtu_resv_var);
void set_uni_mtu_send(u16 uni_mtu_send_var);

extern u16 uni_mtu_resv;
extern u16 uni_mtu_send;
extern u16 uni_recieve_buf_size;
extern u16 uni_cmd_pool_size;
extern u16 uni_rcsp_pool_size;
extern u16 uni_wait_rcsp_pool_size;

#endif//__UNI_PACKET_H__


