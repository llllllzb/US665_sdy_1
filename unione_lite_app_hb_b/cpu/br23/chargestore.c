#include "generic/typedef.h"
#include "generic/gpio.h"
#include "asm/power/p33.h"
#include "asm/hwi.h"
#include "asm/gpio.h"
#include "asm/clock.h"
#include "asm/chargestore.h"
#include "update.h"
#include "app_config.h"

struct chargestore_handle {
    const struct chargestore_platform_data *data;
    UNI_UART_TypeDef *UART;
    u32 baudrate;
};
#define DMA_ISR_LEN 64
#define DMA_BUF_LEN 64
#define __this  (&hdl)
static struct chargestore_handle hdl;
u8 uart_dma_buf[DMA_BUF_LEN] __attribute__((aligned(4)));
volatile u8 send_busy;

//串口时钟和串口超时时钟是分开的
#define UART_SRC_CLK    clk_get("uart")
#define UART_OT_CLK     clk_get("lsb")

enum {
    UPGRADE_NULL = 0,
    UPGRADE_USB_HARD_KEY,
    UPGRADE_USB_SOFTKEY,
    UPGRADE_UART_SOFT_KEY,
    UPGRADE_UART_ONE_WIRE_HARD_KEY,
};
extern void charge_reset_pb5_pd_status(void);
extern void nvram_set_boot_state(u32 state);
extern void local_irq_disable();
void chargestore_set_update_ram(void)
{
    //需要补充设置ram
    int tmp;
    __asm__ volatile("%0 =icfg" : "=r"(tmp));
    tmp &= ~(3 << 8);
    __asm__ volatile("icfg = %0" :: "r"(tmp));//GIE1
    local_irq_disable();
    nvram_set_boot_state(UPGRADE_UART_SOFT_KEY);
}

static u8 chargestore_get_f95_det_res(u32 equ_res)
{
    u8 det_res = (equ_res + 50) / 100;
    if (det_res > 0) {
        det_res -= 1;
    }
    if (det_res > 0x0f) {
        det_res = 0x0f;
    }
    return det_res;
}

//br23, LDOIN电压为2V时等效电阻约1M, 功耗约1.8uA
u8 chargestore_get_det_level(u8 chip_type)
{
    switch (chip_type) {
    case TYPE_F95:
        return chargestore_get_f95_det_res(1600);
    case TYPE_NORMAL:
    default:
        return 0x0f;
    }
}

void __attribute__((weak)) chargestore_data_deal(u8 cmd, u8 *data, u8 len)
{

}

___interrupt
static void uart_isr(void)
{
    u16 i;
    u32 rx_len = 0;
    if ((__this->UART->CON0 & BIT(2)) && (__this->UART->CON0 & BIT(15))) {
        __this->UART->CON0 |= BIT(13);
        send_busy = 0;
        chargestore_data_deal(CMD_COMPLETE, NULL, 0);
    }
    if ((__this->UART->CON0 & BIT(3)) && (__this->UART->CON0 & BIT(14))) {
        __this->UART->CON0 |= BIT(12);//清RX PND
        chargestore_data_deal(CMD_RECVDATA, uart_dma_buf, DMA_ISR_LEN);
        memset((void *)uart_dma_buf, 0, sizeof(uart_dma_buf));
        __this->UART->RXSADR = (u32)uart_dma_buf;
        __this->UART->RXEADR = (u32)(uart_dma_buf + DMA_BUF_LEN);
        __this->UART->RXCNT = DMA_ISR_LEN;
    }
    if ((__this->UART->CON0 & BIT(5)) && (__this->UART->CON0 & BIT(11))) {
        //OTCNT PND
        __this->UART->CON0 |= BIT(7);//DMA模式
        __this->UART->CON0 |= BIT(10);//清OTCNT PND
        asm volatile("nop");
        rx_len = __this->UART->HRXCNT;//读当前串口接收数据的个数
        __this->UART->CON0 |= BIT(12);//清RX PND(这里的顺序不能改变，这里要清一次)
        chargestore_data_deal(CMD_RECVDATA, uart_dma_buf, rx_len);
        memset((void *)uart_dma_buf, 0, sizeof(uart_dma_buf));
        __this->UART->RXSADR = (u32)uart_dma_buf;
        __this->UART->RXEADR = (u32)(uart_dma_buf + DMA_BUF_LEN);
        __this->UART->RXCNT = DMA_ISR_LEN;
    }
}

void chargestore_write(u8 *data, u8 len)
{
    u32 data_addr = (u32)data;
    if (data_addr % 4) {//4byte对齐
        ASSERT(0, "%s: unaligned accesses!", __func__);
    }
    send_busy = 1;
    __this->UART->TXADR = data_addr;
    __this->UART->TXCNT = len;
}

void chargestore_open(u8 mode)
{
    send_busy = 0;
    __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10);
    if (mode == MODE_RECVDATA) {
        //约定:ut0->input_ch0 ut1->input_ch3(因为input_ch1要给IR用)
        if (__this->UART == UNI_UART0) {
            gpio_uart_rx_input(__this->data->io_port, 0, INPUT_CH0);
        } else {
            gpio_uart_rx_input(__this->data->io_port, 1, INPUT_CH3);
        }
        //避免插入普通充电舱,舱体不升压 only for br23
        if (__this->data->io_port == IO_PORTB_05) {
            charge_reset_pb5_pd_status();
        }
        memset((void *)uart_dma_buf, 0, sizeof(uart_dma_buf));
        __this->UART->RXSADR = (u32)uart_dma_buf;
        __this->UART->RXEADR = (u32)(uart_dma_buf + DMA_BUF_LEN);
        __this->UART->RXCNT = DMA_ISR_LEN;
        __this->UART->CON0 |= BIT(6) | BIT(5) | BIT(3);
    } else {
        //约定:ut0->output_ch0 ut1->output_ch1
        if (__this->UART == UNI_UART0) {
            gpio_output_channle(__this->data->io_port, CH0_UT0_TX);
        } else {
            gpio_output_channle(__this->data->io_port, CH1_UT1_TX);
        }
        gpio_set_hd(__this->data->io_port, 1);
        __this->UART->CON0 |= BIT(2);
    }
    __this->UART->CON0 |= BIT(13) | BIT(12) | BIT(10) | BIT(0);
}

void chargestore_close(void)
{
    __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10) | BIT(0);
    gpio_set_pull_down(__this->data->io_port, 0);
    gpio_set_pull_up(__this->data->io_port, 0);
    gpio_set_die(__this->data->io_port, 1);
    gpio_set_hd(__this->data->io_port, 0);
    gpio_direction_input(__this->data->io_port);
    send_busy = 0;
    memset((void *)uart_dma_buf, 0, sizeof(uart_dma_buf));
    if (__this->data->io_port == IO_PORTB_05) {
        charge_reset_pb5_pd_status();
    }
}

void chargestore_set_baudrate(u32 baudrate)
{
    u32 uart_timeout;
    __this->baudrate = baudrate;
    uart_timeout = 20 * 1000000 / __this->baudrate;
    __this->UART->OTCNT = uart_timeout * (UART_OT_CLK / 1000000);
    __this->UART->BAUD = (UART_SRC_CLK / __this->baudrate) / 4 - 1;
}

void chargestore_init(const struct chargestore_platform_data *data)
{
    u32 uart_timeout;
    __this->data = (struct chargestore_platform_data *)data;
    ASSERT(data);
    switch (__this->data->uart_irq) {
    case IRQ_UART0_IDX:
        __this->UART = UNI_UART0;
        break;
    case IRQ_UART1_IDX:
        __this->UART = UNI_UART1;
        break;
    default:
        ASSERT(0, "uart irq(%d) err!\n", __this->data->uart_irq);
        break;
    }
    if (__this->UART->CON0 & BIT(0)) {
        ASSERT(0, "uart used!\n");
    }
    send_busy = 0;
    uart_timeout = 20 * 1000000 / __this->data->baudrate;
    __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10) | BIT(0);
    __this->UART->OTCNT = uart_timeout * (UART_OT_CLK / 1000000);
    __this->UART->BAUD = (UART_SRC_CLK / __this->data->baudrate) / 4 - 1;
    __this->baudrate = __this->data->baudrate;
    if (__this->data->io_port == IO_PORTB_05) {
        charge_reset_pb5_pd_status();
    } else {
        gpio_set_pull_down(__this->data->io_port, 0);
    }
    gpio_set_pull_up(__this->data->io_port, 0);
    gpio_set_die(__this->data->io_port, 1);
    gpio_direction_input(__this->data->io_port);
    request_irq(__this->data->uart_irq, 2, uart_isr, 0);
    if (__this->UART == UNI_UART0) {
        //不占用IO
        gpio_set_uart0(-1);
    } else {
        //不占用IO
        gpio_set_uart1(-1);
    }
#if (!TCFG_CHARGE_ENABLE)
    LDO5V_EN(1);
    LDO5V_EDGE_SEL(1);
    LDO5V_PND_CLR();
    LDO5V_EDGE_WKUP_EN(1);
#endif
}

static void clock_critical_enter(void)
{
    u8 cmp_buf[2] = {0x55, 0xAA};
    //等待数据收完
    extern void *memmem(void *srcmem, int src_len, void *desmem, int des_len);
    while (memmem(uart_dma_buf, sizeof(uart_dma_buf), cmp_buf, sizeof(cmp_buf)));
    //等待数据发完
    while (send_busy);
}

static void clock_critical_exit(void)
{
    u32 uart_timeout;
    if (__this->UART == NULL) {
        return;
    }
    uart_timeout = 20 * 1000000 / __this->baudrate;
    __this->UART->OTCNT = uart_timeout * (UART_OT_CLK / 1000000);
    __this->UART->BAUD = (UART_SRC_CLK / __this->baudrate) / 4 - 1;
}
CLOCK_CRITICAL_HANDLE_REG(chargestore, clock_critical_enter, clock_critical_exit)

