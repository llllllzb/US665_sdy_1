/**
 * Copyright 2020, unisound.com. All rights reserved.
 */

#include <stdint.h>
#include <stdlib.h>
#include "includes.h"
#include "ofa_consts.h"
#include "ual_ofa.h"
#include "asrfix.h"
#include "grammar.h"
#include "uni_json.h"

#include "uni_cust_config.h"
#include "uni_json.h"
#include "uni_float2string.h"
#include "uni_nlu.h"
#include "uni_lasr_result_parser.h"
#include "uni_iot.h"
#include "uni_kws.h"
#include "user_player.h"
#include "uni_auto_control.h"

#include "app_task.h"
#include "system/includes.h"
#include "system/event.h"
#include "user_iot.h"
#include "kws_demo.h"

#define LOG_TAG_CONST       KWS
#define LOG_TAG             "[KWS]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define KWS_TAG "kws_main"

#ifdef KWS_WAKEUP_SCORE_THRED
#define WUW_STD_THRES   (KWS_WAKEUP_SCORE_THRED)
#else
#define WUW_STD_THRES   (-0.43f)
#endif
#ifdef KWS_SLEEP_SCORE_THRED
#define WUW_SLEEP_THRES KWS_SLEEP_SCORE_THRED
#else
#define WUW_SLEEP_THRES (WUW_STD_THRES + 2.0f)
#endif
#define WUW_LOW_THRES   (-10.0f)

#ifdef KWS_CMD_SCORE_THRED
#define CMD_STD_THRES   (KWS_CMD_SCORE_THRED)
#else
#define CMD_STD_THRES   (1.19f)
#endif
#define CMD_LOW_THRES   (-10.0f)

#define KWS_CMD_TIMEOUT_MS       (UNI_ASR_TIMEOUT * 1000)
#define KWS_WUW_SLEEP_TIMEOUT_MS (60000)

#ifdef KWS_HASHTABLE_SIZE
#define KEYWORD_NUM     KWS_HASHTABLE_SIZE
#else
#define KEYWORD_NUM     4
#endif

#define KWS_DECODER_BUF_SIZE (30 * 1024)
uint8_t g_kws_share_mem[KWS_DECODER_BUF_SIZE];

#if defined(CONFIG_DEBUG) && CONFIG_DEBUG
#define DEBUG_RTF 1
#else
#define DEBUG_RTF 0
#endif

//#define KWS_DATA_MONITOR

enum {
  KWS_EVT_START = 0,
  KWS_EVT_STOP,
};

#define Q_KWS_MSG_SIZE  6  ///理论Queue受任务声明struct task_info.qsize限制,但不宜过大,建议<=6

enum {
  KEY_UNI_KWS_POST_EVENT = 0,
  KEY_UNI_PLAY_EVENT,
};

/* KWS work context */
typedef struct kws_context {
  HANDLE                    kws;
  kws_work_state            state;
  engine_kws_mode           engine_mode;
  engine_kws_mode           last_mode;
  uni_bool                  running;
  uni_bool                  working;
  uni_sem_t                 started_sem;
  uni_sem_t                 stopped_sem;
  uni_u32                   timeout;
  uni_u32                   sleep_timeout;
  uni_u32                   cur_timestamp;
  float                     std_thres;
  int                       am_id;
  char                      cmd[32];
}kws_context_t;

static kws_context_t g_kws_context = {0};

static char asr_timeout_enable = 1;

#if DEBUG_RTF
static int64_t engine_time = 0.0;
static int64_t wav_time = 0.0;
static uint32_t rtf_cnt = 1000;
#endif

void enable_asr_timeout(void)
{
  asr_timeout_enable = 1;
}

void disable_asr_timeout(void)
{
  asr_timeout_enable = 0;
}

static uni_err_t _kws_semaphore_init(void) {
  uni_sem_init(&g_kws_context.started_sem, 0);
  uni_sem_init(&g_kws_context.stopped_sem, 0);
  return UNI_NO_ERR;
}

static uni_err_t _kws_semaphore_deinit(void) {
  uni_sem_destroy(g_kws_context.started_sem);
  uni_sem_destroy(g_kws_context.stopped_sem);
  return UNI_NO_ERR;
}

int kws_state(void) {
  return g_kws_context.state;
}

bool kws_is_running(void) {
  return (KWS_WORK_RUNNING == g_kws_context.state);
}

uni_err_t kws_switch_mode(engine_kws_mode mode) {
  if (KWS_WORK_RUNNING != g_kws_context.state) {
    return UNI_INVALID_STATE_ERR;
  }
  if (g_kws_context.engine_mode == mode) {
    return UNI_NO_ERR;
  }
  LOGD(KWS_TAG, "switch to mode %d", mode);
  g_kws_context.last_mode = g_kws_context.engine_mode;
  g_kws_context.engine_mode = mode;
  g_kws_context.sleep_timeout = 0;
  g_kws_context.timeout = 0;
  g_kws_context.cur_timestamp = 0;
  app_task_put_usr_msg(APP_MSG_USER, 1, KWS_EVT_STOP);
  uni_sem_wait(g_kws_context.stopped_sem);
  app_task_put_usr_msg(APP_MSG_USER, 1, KWS_EVT_START);
  uni_sem_wait(g_kws_context.started_sem);
  g_kws_context.state = KWS_WORK_RUNNING;
  return UNI_NO_ERR;
}

uni_err_t kws_relaunch(void) {
  if (KWS_WORK_IDLE != g_kws_context.state) {
    return UNI_INVALID_STATE_ERR;
  }
  if (uni_strstr(g_kws_context.cmd, "wakeup_uni") != NULL) {
    g_kws_context.engine_mode = ENGINE_KWS_MODE_CMD;
  } else if (uni_strstr(g_kws_context.cmd, "exitUni") != NULL) {
    g_kws_context.engine_mode = ENGINE_KWS_MODE_WAKEUP;
    /*
  } else if (uni_strstr(g_kws_context.cmd, "btStartUni") != NULL) {
    app_task_switch_to(APP_BT_TASK);
    return UNI_NO_ERR;
    */
  }
  memset(g_kws_context.cmd, 0, sizeof(g_kws_context.cmd));
  g_kws_context.cur_timestamp = 0;
  app_task_put_usr_msg(APP_MSG_USER, 1, KWS_EVT_START);
  uni_sem_wait(g_kws_context.started_sem);
  g_kws_context.state = KWS_WORK_RUNNING;
  return UNI_NO_ERR;
}

uni_err_t kws_start(void) {
  if (KWS_WORK_IDLE != g_kws_context.state) {
    return UNI_INVALID_STATE_ERR;
  }
  g_kws_context.cur_timestamp = 0;
  app_task_put_usr_msg(APP_MSG_USER, 1, KWS_EVT_START);
  uni_sem_wait(g_kws_context.started_sem);
  g_kws_context.state = KWS_WORK_RUNNING;
  return UNI_NO_ERR;
}

uni_err_t kws_first_start(void) {
  if (KWS_WORK_IDLE != g_kws_context.state) {
    return UNI_INVALID_STATE_ERR;
  }
  g_kws_context.cur_timestamp = 0;
  _internal_kws_start();
  uni_sem_wait(g_kws_context.started_sem);
  g_kws_context.state = KWS_WORK_RUNNING;
  return UNI_NO_ERR;
}

uni_err_t kws_stop(void) {
  if (KWS_WORK_RUNNING != g_kws_context.state) {
      return UNI_INVALID_STATE_ERR;
  }
  g_kws_context.timeout = 0;
  g_kws_context.sleep_timeout = 0;
  app_task_put_usr_msg(APP_MSG_USER, 1, KWS_EVT_STOP);
  uni_sem_wait(g_kws_context.stopped_sem);
  g_kws_context.state = KWS_WORK_IDLE;
  return UNI_NO_ERR;
}

static int uni_pcm_play(const char* pcm, const char* cmd) {
  uni_strcpy(g_kws_context.cmd, cmd);
  return user_player_play(pcm);
}

static int uni_pcm_play_order(const char* pcm, const char* cmd) {
  uni_strcpy(g_kws_context.cmd, cmd);
  return user_player_reply_list_in_order(pcm);
}

static int uni_pcm_play_ramdom(const char* pcm, const char* cmd) {
  uni_strcpy(g_kws_context.cmd, cmd);
  g_kws_context.cur_timestamp = 0;
  return user_player_reply_list_random(pcm);
}

void kws_init_done(void) {
  #ifdef DEFAULT_PCM_WELCOME
  LOGD(KWS_TAG, "play welcome start");
  os_taskq_post_event("uni_kws", 2, KEY_UNI_PLAY_EVENT, DEFAULT_PCM_WELCOME);
  #else
  kws_first_start();
  #endif
}

void _uni_kws_send_event(kws_result_msg_t *kws_event)
{
    os_taskq_post_event("uni_kws", 2, KEY_UNI_KWS_POST_EVENT, kws_event);
}

static void _write_arpt_log(const char *command, float score, const char *json,
                                char is_wakeup, char is_basic, char is_rasr) {
#if defined(CONFIG_ARPT_PRINT) && CONFIG_ARPT_PRINT
  char buffer_log[256] = {0};
  char tag[16] = {0};
  char result[16] = {0};
  char kws[8] = {0};
  char pure_cmd[64] = {0};
  char is_more = !(is_basic || is_rasr);
  /* basic content */
  snprintf(tag, sizeof(tag), "%s_%s", is_wakeup ? "asr" : "wakeup",
               "normal");
  snprintf(kws, sizeof(kws), "%s", is_basic ? "KWS\t" : "");
  snprintf(result, sizeof(result), "%s", is_rasr ? "online_json" : "offline_result");
  if (is_rasr) {
    snprintf(buffer_log, sizeof(buffer_log), "%s%s:[%s]\t", kws, result, tag);
  } else {
    //StrSubEx(pure_cmd, command, ">", "</");
    snprintf(pure_cmd, sizeof(pure_cmd), "%s", command);
    snprintf(buffer_log, sizeof(buffer_log), "%s%s:[%s]\tcommand[%s]\tscore[%.02f]\t",
             kws, result, tag, pure_cmd, score);
  }
  ArptPrint(buffer_log);
  /* more content */
  if (!is_more) {
    ArptPrint(json);
  }
  ArptPrint("\n");
#endif
}

static uni_err_t _check_kws_timeout(void) {
  if ((0 != g_kws_context.sleep_timeout) &&
      (g_kws_context.sleep_timeout == g_kws_context.cur_timestamp)) {
    LOGT(KWS_TAG, "kws goto sleep");
    g_kws_context.std_thres = WUW_SLEEP_THRES;
  }
  if ((asr_timeout_enable == 1) &&
      (0 != g_kws_context.timeout) &&
      (g_kws_context.timeout == g_kws_context.cur_timestamp)) {
    LOGT(KWS_TAG, "kws timeout");
    kws_result_msg_t kws_msg = {0};
    uni_memset(&kws_msg, 0, sizeof(kws_result_msg_t));
    kws_msg.event_id = VUI_LOCAL_ASR_TIMEOUT_EVENT;
    _uni_kws_send_event(&kws_msg);
  }
  return UNI_NO_ERR;
}

static Result _lasr_check_score(float score) {
  if (score >= g_kws_context.std_thres) {
    return E_OK;
  }
  return E_FAILED;
}

static void process_volume_change(const char *cmd) {
  if (0 == uni_strcmp(cmd, "volumeUpUni")) {
    user_player_set_volume_up();
  } else if (0 == uni_strcmp(cmd, "volumeDownUni")) {
    user_player_set_volume_down();
  } else if (0 == uni_strcmp(cmd, "volumeMinUni")) {
    user_player_set_volume_min();
  } else if (0 == uni_strcmp(cmd, "volumeMaxUni")) {
    user_player_set_volume_max();
  } else if (0 == uni_strcmp(cmd, "volumeMidUni")) {
    user_player_set_volume_mid();
  }
}

static void _assign_nlu_type(cJSON *jnlu, kws_result_msg_t *kws_result) {
  char *cmd = NULL;
  char *pcm = NULL;
  uint8_t need_reply = 1;
  uint8_t need_post = 1;
  int is_play = 0;
  int ret;

  if (0 != JsonReadItemString(jnlu, "pcm", &pcm)) {
    goto L_END;
  }
  if (0 != JsonReadItemString(jnlu, "cmd", &cmd)) {
    goto L_END;
  }
  _write_arpt_log(kws_result->command, kws_result->score, cmd, g_kws_context.engine_mode, 1, 0);
  ret = user_gpio_handle_action_event(cmd, &need_reply, &need_post);
  if (need_reply) {   //quick replay
    if (uni_pcm_play_ramdom(pcm, cmd) == 0) {
      is_play = 1;
    }
  }
  if (need_post && ret == 0) {
    if (user_iot_voice_set(cmd) != 0) {
      LOGW(KWS_TAG, "iot dispatcher %s not found", cmd);
    }
  }
  if (0 == uni_strcmp(cmd, "volumeUpUni")
      || 0 == uni_strcmp(cmd, "volumeDownUni")
      || 0 == uni_strcmp(cmd, "volumeMinUni")
      || 0 == uni_strcmp(cmd, "volumeMaxUni")
      || 0 == uni_strcmp(cmd, "volumeMidUni")) {
    process_volume_change(cmd);
    goto L_END;
  }
  if (is_play) {
    goto L_END;
  }
  if (uni_strstr(cmd, "wakeup_uni") != NULL) {
    kws_switch_mode(ENGINE_KWS_MODE_CMD);
    goto L_END;
  }
  /*
  if (uni_strstr(cmd, "btStartUni") != NULL) {
    app_task_switch_to(APP_BT_TASK);
    goto L_END;
  }
  */
  if (uni_strstr(cmd, "exitUni") != NULL) {
    kws_switch_mode(ENGINE_KWS_MODE_WAKEUP);
    goto L_END;
  }
L_END:
  if (cmd) {
    uni_free(cmd);
  }
  if (pcm) {
    uni_free(pcm);
  }
}

static void uni_kws_task_handle(void *p)
{
  int msg[Q_KWS_MSG_SIZE + 1] = {0, 0, 0, 0, 0, 0};
  int res = 0;
  kws_result_msg_t kws_result = {0};
  cJSON *jnlu = NULL;

  while (1) {
    //这里的"taskq"并不特指任线程,只是一个名字而已(理论上名字乱写也没事)
    //os_task_pend是阻塞式地等待其他地方往本线程发送信息，这里的本线程名字为uni_kws
    //往本线程发送信息时需要知道本线程的名字，如uni_kws
    res = os_task_pend("taskq", msg, ARRAY_SIZE(msg));
    if (res != OS_TASKQ) {
        continue;
    }
    if (msg[0] == Q_EVENT) 
    {
      switch (msg[1]) 
      {
        //这个应该是识别语音的
        case KEY_UNI_KWS_POST_EVENT:
          memcpy(&kws_result, (kws_result_msg_t *)msg[2], sizeof(kws_result_msg_t));
          LOGD(KWS_TAG,"recv result:%s, scord:%s", kws_result.command, Float2String(kws_result.score));
          do {
            if (kws_result.event_id == VUI_LOCAL_ASR_TIMEOUT_EVENT && kws_is_running()) 
            {
              #ifdef DEFAULT_PCM_ASR_TIMEOUT
              if (0 != uni_pcm_play(DEFAULT_PCM_ASR_TIMEOUT, "exitUni")) 
              {
                kws_switch_mode(ENGINE_KWS_MODE_WAKEUP);
              }
              #else
              kws_switch_mode(ENGINE_KWS_MODE_WAKEUP);
              #endif
              break;
            }
            if (NULL == (jnlu = NluParseLasr(kws_result.command))) {
              break;
            }
            _assign_nlu_type(jnlu, &kws_result);
          } while(0);
          if (jnlu != NULL) {
            cJSON_Delete(jnlu);
            jnlu = NULL;
          }
          break;
        case KEY_UNI_PLAY_EVENT:
          if (0 != user_player_play(msg[2])) 
          {
            kws_relaunch();
          }
          break;
        default:
          LOGE(KWS_TAG, "unknown event %d", msg[1]);
          break;
      }
    } else {
      LOGE(KWS_TAG, "msg not Q_EVENT");
    }
  }
}

static void ShowResourceInfo(void* kws) {
  /* 打印所有声学模型、语法的信息 */
  UalOFAPrintResourceInfo(kws);
#if 1
  /* 获取当前的工作语种 */
  const char* default_lang =
    UalOFAGetOptionString(kws, 800);

  /* 获取当前使用的声学模型ID */
  int default_am_id = UalOFAGetActiveAmId(kws);

  /* 获取当前使用的语法domain */
  const char* default_grammar_domain = UalOFAGetActiveGrammarDomain(kws);

  printf("Default Info: \n");
  printf("AmID: %d \n", default_am_id);
  printf("Language: %s \n", default_lang);
  printf("GrammarDomain: %s \n", default_grammar_domain);
#endif
}

static int _internal_kws_stop(void) {
  UalOFAStop(g_kws_context.kws);
  LOGI(KWS_TAG, "kws stop");
  uni_sem_post(g_kws_context.stopped_sem);
  g_kws_context.working = FALSE;
  return 0;
}

static int _internal_kws_start(void) {
  int status = 0;
  void* decoder_pool = g_kws_share_mem;

  // seems have clk config
  clk_set("sys", 240000000);

  if (g_kws_context.engine_mode == ENGINE_KWS_MODE_WAKEUP) {
    status = UalOFAStart(g_kws_context.kws, "wakeup", g_kws_context.am_id, decoder_pool, KWS_DECODER_BUF_SIZE);
    //status = UalOFAStart(g_kws_context.kws, "wakeup", /*51*/43, decoder_pool, KWS_DECODER_BUF_SIZE);
    g_kws_context.std_thres = WUW_STD_THRES;
    g_kws_context.sleep_timeout = KWS_WUW_SLEEP_TIMEOUT_MS / 16;
    ArptPrint("enter wakeup_normal\n");
  } else {
    status = UalOFAStart(g_kws_context.kws, "ivm", g_kws_context.am_id, decoder_pool, KWS_DECODER_BUF_SIZE);
    //status = UalOFAStart(g_kws_context.kws, "ivm", /*51*/43, decoder_pool, KWS_DECODER_BUF_SIZE);
    g_kws_context.std_thres = CMD_STD_THRES;
    g_kws_context.timeout = KWS_CMD_TIMEOUT_MS / 16;
    ArptPrint("enter asr_normal\n");
  }

  if (status != ASR_RECOGNIZER_OK) {
    LOGE(KWS_TAG, "Start Engine failed.");
    return -1;
  }
  LOGI(KWS_TAG, "kws start in %d mode", g_kws_context.engine_mode);
  uni_sem_post(g_kws_context.started_sem);
  g_kws_context.working = TRUE;
  return 0;
}

static int _internal_kws_init(void) {
  if (KWS_WORK_INIT != g_kws_context.state) {
    return UNI_INVALID_STATE_ERR;
  }
  _kws_semaphore_init();  

  g_kws_context.running = TRUE;
  g_kws_context.working = FALSE;
  g_kws_context.engine_mode = ENGINE_KWS_MODE_WAKEUP;
  g_kws_context.last_mode = g_kws_context.engine_mode;
  g_kws_context.state = KWS_WORK_IDLE;
  g_kws_context.timeout = 0;

  //初始化lp模型的长度以及内存起始地址，至于什么是lp模型我只知道和kws相关。。。
  int64_t len_am_buffer = sizeof(global_kws_lp_acoutstic_model);
  const char* am_buffer = global_kws_lp_acoutstic_model;

  //初始化lp语法的长度以及内存起始地址，至于什么是lp模型我就更不知道了。。。
  int64_t len_grammar_buffer = sizeof(global_kws_lp_grammar);
  const char* grammar_buffer = global_kws_lp_grammar;

  //拿lp模型和lp语法的内存初始化地址来生成kws句柄
  g_kws_context.kws = UalOFAInitializeFromBuffer(am_buffer, grammar_buffer);
  if (NULL == g_kws_context.kws) {
    LOGE(KWS_TAG, "Error initializing!");
    return -1;
  }

  //ShowResourceInfo(g_kws_context.kws);
  //设置kws引擎的某些属性的属性值
  UalOFASetOptionInt(g_kws_context.kws, KWS_SET_BUNCH_FRAME_NUMBER, 12);
  //UalOFASetOptionString(g_kws_context.kws, 800, "mandarin");
  UalOFASetOptionInt(g_kws_context.kws, KWS_BEAM_ID, 10);
  printf("kwslp init done \n");
  
  //获取当前活跃的lp模型的id？
  g_kws_context.am_id = UalOFAGetActiveAmId(g_kws_context.kws);

  const char* version = UalOFAGetVersion(g_kws_context.kws);
  LOGI(KWS_TAG, "kws version is :%s amid: %d", version, g_kws_context.am_id);
  UalOFAReset(g_kws_context.kws);

  task_create(uni_kws_task_handle, &g_kws_context, "uni_kws");

  return 0;
}

static int _internal_kws_deinit(void) {
  if (KWS_WORK_RUNNING == g_kws_context.state) {
    g_kws_context.timeout = 0;
    g_kws_context.sleep_timeout = 0;
    UalOFAStop(g_kws_context.kws);
    LOGI(KWS_TAG, "kws stop");
    g_kws_context.working = FALSE;
    g_kws_context.state = KWS_WORK_IDLE;
  }
  UalOFARelease(g_kws_context.kws);
  g_kws_context.kws = NULL;
  _kws_semaphore_deinit();
  task_kill("uni_kws");

  return 0;
}

int alg_unisound_init(void)
{
    user_gpio_init();
    user_iot_init();    //初始化用户iot 实际就是初始化和声音相关的线程互斥锁

    /* kws全称keyword spotting 是语音唤醒技术 */
    if (_internal_kws_init() < 0) {
      LOGE(KWS_TAG, "module_kws_init Failed!");
      return -1;
    }

    return 0;
}

int alg_unisound_deinit()
{
  if (_internal_kws_deinit() < 0) {
      LOGE(KWS_TAG, "module_kws_deinit Failed!");
      return -1;
  }
  return 0;
}

int alg_unisound_asr_proc(void *mic)
{
    int ret = -1;
    unsigned int flag;

    //int time_offset = 0;  // ms
#if DEBUG_RTF
    long long start_time, end_time;
#endif
    kws_result_msg_t kws_msg = {0};

    if (!g_kws_context.working) {
      return ret;
    }

#if DEBUG_RTF
    wav_time += (float)CAPTURE_FRAME_MS;
    start_time = aos_now() / 1000;
#endif
 
#ifdef KWS_DATA_MONITOR
    static int16_t last_value = 0;
    static int16_t monitor_cnt = 0;
    int16_t *p = (int16_t *)mic;
    if (*p == last_value && *p == *(p + CAPTURE_FRAME_SIZE_1CH / 2 - 1)) {
      if (++monitor_cnt > 200) {
        LOGE(KWS_TAG, "data abnormal: continuous 200 frames seems keep same value %04x", last_value);
        //TBD reboot here
      }
    } else {
      last_value = *p;
      monitor_cnt = 0;
    }
#endif

    g_kws_context.cur_timestamp += 1;
    ret = UalOFARecognize(g_kws_context.kws, (signed char*)mic, CAPTURE_FRAME_SIZE_1CH);
    if (ret < 0) {
      LOGE(KWS_TAG, "Recognize error!");
      return 1;
    }
    if (ret == 2) {
      const char* result_const = UalOFAGetResult(g_kws_context.kws);
      memset(&kws_msg, 0, sizeof(kws_result_msg_t));
      kws_msg.event_id = VUI_LOCAL_ASR_SELF_RESULT_EVENT;
      if (LasrResultParse(result_const, kws_msg.command, &kws_msg.score) != E_OK) {
        LOGE(KWS_TAG, "LasrResultParse failed!");
        return 1;
      }

      LOGI(KWS_TAG, "command=%s, score=%s, std_thresh=%s", kws_msg.command, Float2String(kws_msg.score), Float2String(g_kws_context.std_thres));
      if (E_OK == _lasr_check_score(kws_msg.score)) {
        _uni_kws_send_event(&kws_msg);
      } else {
        _write_arpt_log(kws_msg.command, kws_msg.score, NULL, g_kws_context.engine_mode, 0, 0);
      }
    }

#if DEBUG_RTF
    end_time = aos_now() / 1000;
    engine_time += (end_time - start_time);
    rtf_cnt--;
    if (rtf_cnt == 0) {
      printf("RTF = %lld (ms) / %lld (ms) = %.4fx\n", engine_time / 1000,
             wav_time, (float)engine_time / 1000 / wav_time);
      wav_time = 0;
      engine_time = 0;
      rtf_cnt = 1000;
    }
#endif
    _check_kws_timeout();

    return 0;
}

int alg_unisound_deal_user_msg(int msg) {
  switch (msg) {
    case KWS_EVT_START:
      _internal_kws_start();
      break;
    case KWS_EVT_STOP:
      _internal_kws_stop();
      break;
    default:
      break;
  }
  return 0;
}

