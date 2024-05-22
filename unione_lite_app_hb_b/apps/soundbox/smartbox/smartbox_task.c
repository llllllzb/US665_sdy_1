#include "smartbox/smartbox_task.h"
#include "smartbox/smartbox.h"
#include "file_transfer.h"
#include "file_delete.h"
#include "dev_format.h"
#include "clock_cfg.h"
#include "app_task.h"

#include "le_smartbox_module.h"

#if (SMART_BOX_EN)
//这个模式主要是提供一个空模式， 处理一些需要占用时间不较长的交互处理， 处理做完之后退回到原来的模式
struct __action_event {
  u8	type;		//1:手机端触发, 0:固件触发
  u8 	OpCode_SN;
  u8	action;
};
static struct __action_event action_prepare = {0};

static void app_smartbox_task_get_ready(void)
{
  printf("%s\n", __FUNCTION__);
  if (action_prepare.type) {
    UNI_CMD_response_send(UNI_OPCODE_ACTION_PREPARE, UNI_PRO_STATUS_SUCCESS, action_prepare.OpCode_SN, NULL, 0);
  }
}

static void app_smartbox_action_end_callback(void)
{
  if (app_get_curr_task() == APP_SMARTBOX_ACTION_TASK) {
    printf("action end callback!!\n");
    app_task_switch_back();
  }
}

static void app_smartbox_task_start(void)
{
  clock_add_set(SMARTBOX_ACTION_CLK);
  app_smartbox_task_get_ready();
  //根据不同的场景， 做不同的处理， 例如：初始化不同的UI显示
  switch (action_prepare.action)		{
    case SMARTBOX_TASK_ACTION_FILE_TRANSFER:
      file_transfer_init(app_smartbox_action_end_callback);
      break;
    case SMARTBOX_TASK_ACTION_FILE_DELETE:
      file_delete_init(app_smartbox_action_end_callback);
      break;
    case SMARTBOX_TASK_ACTION_DEV_FORMAT:
      dev_format_init(app_smartbox_action_end_callback);
      break;
    default:
      break;
  }
}

static void app_smartbox_task_stop(void)
{
  switch (action_prepare.action)		{
    case SMARTBOX_TASK_ACTION_FILE_TRANSFER:
      break;
    case SMARTBOX_TASK_ACTION_FILE_DELETE:
      break;
    case SMARTBOX_TASK_ACTION_DEV_FORMAT:
      break;
    default:
      break;
  }

  clock_remove_set(SMARTBOX_ACTION_CLK);
  printf("app_smartbox_task_stop\n");
}

static int app_smartbox_task_event_handle(struct sys_event *event)
{
  switch (action_prepare.action)		{
    case SMARTBOX_TASK_ACTION_FILE_TRANSFER:
      break;
    case SMARTBOX_TASK_ACTION_FILE_DELETE:
      break;
    case SMARTBOX_TASK_ACTION_DEV_FORMAT:
      break;
    default:
      break;
  }
  return 0;
}

void app_smartbox_task_prepare(u8 type, u8 action, u8 OpCode_SN)
{
  action_prepare.type = type;
  action_prepare.action = action;
  action_prepare.OpCode_SN = OpCode_SN;
  //切换模式
  if (app_get_curr_task() != APP_SMARTBOX_ACTION_TASK) {
    app_task_switch_to(APP_SMARTBOX_ACTION_TASK);
  } else {
    app_smartbox_task_get_ready();
  }
}


void app_smartbox_task(void)
{
  int msg[32];
  app_smartbox_task_start();
  while (1) {
    app_task_get_msg(msg, ARRAY_SIZE(msg), 1);

    switch (msg[0]) {
      case APP_MSG_SYS_EVENT:
        if (app_smartbox_task_event_handle((struct sys_event *)(msg + 1)) == false) {
          app_default_event_deal((struct sys_event *)(&msg[1]));
        }
        break;
      default:
        break;
    }

    if (app_task_exitting()) {
      app_smartbox_task_stop();
      return;
    }
  }
}
#else

#include "ofa_consts.h"
#include "ual_ofa.h"
#include "uni_databuf.h"
#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "asm/audio_src.h"
#include "audio_enc.h"
#include "audio_dec.h"
#include "app_main.h"
//#include "app_task.h"
#include "audio_config.h"
#include "kws_demo.h"

#define LOG_TAG_CONST       SMART_TASK
#define LOG_TAG             "[SMART_TASK]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define FRAME_LEN     512

typedef struct {
  struct audio_adc_output_hdl output;
  struct audio_adc_ch linein_ch;
  struct adc_mic_ch mic_ch;
  char temp_buf[FRAME_LEN];
  DataBufHandle mic_buf;
} laudio_adc_t;
static laudio_adc_t *ladc_var = NULL;

static struct audio_adc_hdl adc_hdl;
static void *lp = NULL;
static char g_running = 0;
static uni_sem_t g_mic_sem;

static void _audio_adc_cb(void *priv, s16 *data, int len)
{
  if (DataBufferGetFreeSize(ladc_var->mic_buf) < len) {
    printf("mic data overflow!\n");
  } else {
    DataBufferWrite(ladc_var->mic_buf, (char *)data, len);
    if (DataBufferGetDataSize(ladc_var->mic_buf) >= FRAME_LEN) {
      uni_sem_post(g_mic_sem);
    }
  }
}

static void _alg_init(void) {

  alg_unisound_init();

  printf("kwslp start ivm \n");
}

static void _audio_init(void) {
  int ret = -1;
  u16 ladc_sr = 16000;
  u8 mic_gain = 5;
  u8 linein_gain = 3;

  //adc
  //printf("audio_adc_open_demo,sr:%d,mic_gain:%d,linein_gain:%d\n", ladc_sr, mic_gain, linein_gain);
  if (ladc_var) {
    printf("ladc already open \n");
    return;
  }
  uni_sem_init(&g_mic_sem, 0);
  ladc_var = zalloc(sizeof(laudio_adc_t));
  if (ladc_var) {
    printf("audio adc start \n");
    ladc_var->mic_buf = DataBufferCreate(1024 * 8);
    ladc_var->output.handler = _audio_adc_cb;
    ladc_var->output.priv = &adc_hdl;
    if (audio_mic_open(&ladc_var->mic_ch, ladc_sr, mic_gain) == 0) {
      printf("audio mic start \n");
      audio_mic_add_output(&ladc_var->output);
      audio_mic_start(&ladc_var->mic_ch);
    }

  }
}

static void _audio_release(void) {
  if (ladc_var) {
    DataBufferDestroy(ladc_var->mic_buf);
    audio_mic_close(&ladc_var->mic_ch, &ladc_var->output);
    free(ladc_var);
    ladc_var = NULL;
  }
  uni_sem_destroy(g_mic_sem);
}

#if 0
#include "wuw.h"

static u32 read_size_sum = 0;
static u32 read_cnt_sum = 0;
u32 nn_size_sum = 0;
u32 nn_cnt_sum = 0;
static int64_t get_time() {
  return timer_get_ms();
}

static int _task_loop_rtf() {
  static int pos = 0;
  static int64_t t1 = 0;
  static int64_t t2 = 0;
  static int64_t t_start = 0;
  static int64_t tsum = 0;
  static int64_t tmp = 0;
  if (0 == t_start) {
    t_start = get_time();
  }
  t1 = get_time();
  int ret = UalOFARecognize(lp, (signed char *)(audiotest + pos), FRAME_LEN);
  t2 = get_time();
  tsum += (t2 - t1);
  if (0 > ret) {
    printf("recog failed !!!!!!\n");
    return -1;
  } else if (2 == ret) {
    const char* result_const = UalOFAGetResult(lp);
    printf("get result: %s \n", result_const);
  }
  if (pos + FRAME_LEN >= sizeof(audiotest)) {
    pos = 0;
    tmp = get_time();
    printf("loop cycle, process %zu ms, use %lld ms, ",
        sizeof(audiotest) / 32, (tmp - t_start));
    printf(" sum %lld \n", tsum);
    tsum = 0;
    printf("read size sum %d cnt %d \n", read_size_sum, read_cnt_sum);
    read_size_sum = read_cnt_sum = 0;
    printf("nn size sum %d cnt %d \n", nn_size_sum, nn_cnt_sum);
    nn_size_sum = nn_cnt_sum = 0;
    t_start = get_time();
  } else {
    pos += FRAME_LEN;
  }
  //printf("pos -> %d \n", pos);
  return 0;
}
#endif
static int _task_loop(void) {
  //audio_dec_sine_test();
  char cmd[64] = {0};
  float score = 0.0;
  int len = 0;

  uni_sem_wait(g_mic_sem);
  len = DataBufferRead(ladc_var->temp_buf, FRAME_LEN, ladc_var->mic_buf);
  if (FRAME_LEN == len) {
    alg_unisound_asr_proc(ladc_var->temp_buf);
  }
  return 0;
}
extern uni_err_t kws_first_start(void);
extern int alg_unisound_deal_user_msg(int msg);

static void app_smartbox_task_stop(void) {
  alg_unisound_deinit();
  _audio_release();
}

void app_smartbox_task(void)
{
  int msg[32];
  printf("entry app smartbox \n");
  wdt_close();
  // wzc:pa enable, may set in player api
  gpio_direction_output(IO_PORTB_05, 0);
  printf("free:%u\n", xPortGetFreeHeapSize());
  //_flash_read_thread();
  //_norflash_init();
  _alg_init();  //里面包含了gpio的初始化
  _audio_init();

  kws_init_done();
  printf("after kws init,free:%u\n", xPortGetFreeHeapSize());
  /*
     while (1) {
  //_flash_test();
  //_task_loop_rtf();
  _task_loop();
  }
  */


  //_task_list();
  //app_smartbox_task_start();
  while (1) {
    _task_loop();
    app_task_get_msg(msg, ARRAY_SIZE(msg), 0);
    switch (msg[0]) {
      case APP_MSG_SYS_EVENT:
        //if (app_smartbox_task_event_handle((struct sys_event *)(msg + 1)) == false) {
        app_default_event_deal((struct sys_event *)(&msg[1]));
        //}
        break;
      case APP_MSG_USER:
        alg_unisound_deal_user_msg(msg[1]);
        break;
      default:
        break;
    }
    if (app_task_exitting()) {
      app_smartbox_task_stop();
      printf("exit app smartbox\n");
      return;
    }
  }

}

#endif//SMART_BOX_EN

