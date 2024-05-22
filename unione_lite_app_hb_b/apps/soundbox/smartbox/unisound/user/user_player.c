/**************************************************************************
 * Copyright (C) 2020-2020  Unisound
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **************************************************************************
 *
 * Description : user_player.c
 * Author      : yuanshifeng@unisound.com
 * Date        : 2021.06.17
 *
 **************************************************************************/
#include <stdlib.h>

#include "automute.h"
#include "tone_player.h"
#include "uni_iot.h"
#include "uni_cust_config.h"
#include "user_gpio.h"

#define LOG_TAG_CONST       USER_PLAYER
#define LOG_TAG             "[USER_PLAYER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

extern uni_err_t kws_stop(void);
extern uni_err_t kws_relaunch(void);

typedef struct {
  char *files;
  int play_cnt;
  int play_index;
  unsigned char *play_addr;
  uni_u32 len;
  int state;
  uni_mutex_t mutex;
} UniPlayer;
static UniPlayer g_uni_player;

#define LIST_COUNT_MAX 5
static int play_list[LIST_COUNT_MAX] = {0};
static int _parse_pcm_list(const char *str_list) {
  int count = 0, sum = 0;
  char last_ch = 0;
  const char *p = str_list;
  g_uni_player.play_index = 0;
  memset(play_list, 0, sizeof(play_list));
  if (!str_list || str_list[0] != '[') {
    return -1;
  }
  while (*p) {
    if (*p == '[') {
      last_ch = *p;
      p++;
      continue;
    }
    if (*p == ',' || *p == ']') {
      if ((last_ch == '[') || (last_ch == ',')) {
        /* invalid style */
        sum = 0;
        last_ch = *p;
        if (*p == ']') {
          /* finished if ']' */
          break;
        }
        continue;
      }
      play_list[count++] = sum;
      sum = 0;
      last_ch = *p;
      if (count >= LIST_COUNT_MAX) break;
      if (*p == ']') {
        /* finished if ']' */
        break;
      }
    } else if (*p >= '0' && *p <= '9') {
      sum = sum * 10 + *p - '0';
      last_ch = *p;
    } else if (*p != ' ' && *p != '\t') {
      /* failed style if there is other char */
      return -1;
    }
    p++;
  }
  if (count <= 0) {
    return -1;
  }
  return count;
}

static int _uni_get_number_pcm(const char *str_list, int num) {
  int count = 0;
  if (num < 0) {
    return -1;
  }
  count = _parse_pcm_list(str_list);
  if (count <= 0 || count <= num) {
    return -1;
  }
  return play_list[num];
}

static int _uni_get_random_pcm(const char *str_list) {
  int count = _parse_pcm_list(str_list);
  if (count <= 0) {
    return -1;
  }
  return play_list[rand() % count];
}

static int _uni_pcm_play_next() {
  if (g_uni_player.play_cnt > g_uni_player.play_index) {
    int num;
    num = play_list[g_uni_player.play_index];
    return num;
  }
  return -1;
}

static void _finish_cb(void *priv, int flag) {
  g_uni_player.state = 0;
  ArptPrint("TTS END\n");
  LOGD(LOG_TAG, "play %d file finished", (u32)priv);
  kws_relaunch();
}

static int _uni_play_filenum(int play_num) {
  if (play_num < 0) {
    LOGE(LOG_TAG, "play file %d invalid", play_num);
    return -1;
  }

  g_uni_player.play_index++;
  g_uni_player.state = 1;
  uni_u8 idx =  tone_get_index(play_num);
  LOGD(LOG_TAG, "play %d file, idx=%d", play_num, idx);
  kws_stop();
  ArptPrint("TTS START\n");
  return tone_play_with_callback_by_index(idx, 1, _finish_cb, (void *)play_num);;
}

int user_player_init(void) {
  //local_audio_register_cb(_feed_data_cb, _finish_cb);
  return 0;
}

int user_player_play_filenum(int play_num) {
  return _uni_play_filenum(play_num);
}

int user_player_play(const char *file) {
  int play_num = 0;
  if (file == NULL) {
    LOGE(LOG_TAG, "play file invalid");
    return -1;
  }
  play_num = _uni_get_number_pcm(file, 0);
  g_uni_player.play_cnt = 1;
  return user_player_play_filenum(play_num);
}

int user_player_reply_list_num(const char *file_list, int num) {
  int play_num = 0;
  if (NULL == file_list) {
    LOGE(LOG_TAG, "Invalid file name is NULL");
    return -1;
  }
  play_num = _uni_get_number_pcm(file_list, num);
  if (play_num == -1) {
    LOGE(LOG_TAG, "Cannot found %dst file in list %s", num, file_list);
    return -1;
  }
  g_uni_player.play_cnt = 1;
  return user_player_play_filenum(play_num);
}

int user_player_reply_list_random(const char *file_list) {
  int play_num = 0;
  if (NULL == file_list) {
    LOGE(LOG_TAG, "Invalid file name is NULL");
    return -1;
  }
  play_num = _uni_get_random_pcm(file_list);
  if (play_num == -1) {
    LOGE(LOG_TAG, "Cannot found any file in list %s", file_list);
    return -1;
  }
  g_uni_player.play_cnt = 1;
  return user_player_play_filenum(play_num);
}

int user_player_reply_list_in_order(const char *file_list) {
  int count = 0;

  if (NULL == file_list) {
    printf(LOG_TAG "%s:Invalid file name is NULL\n", __func__);
    return -1;
  }

  count = _parse_pcm_list(file_list);
  if (count <= 0) {
    return -1;
  }
  g_uni_player.play_cnt = count;
  user_player_play_filenum(play_list[g_uni_player.play_index]);
  return 0;
}

int user_player_stop(void) {
  tone_play_stop();
  return 0;
}

int user_player_set_volume_min(void) {
  set_volume_gear(0);
  return 0;
}

int user_player_set_volume_max(void) {
  set_volume_gear(USER_AUDIO_VOL_LEVEL - 1);
  return 0;
}

int user_player_set_volume_mid(void) {
  user_set_volume(USER_AUDIO_VOL_MID);
  return 0;
}

int user_player_set_volume_up(void) {
  volume_gear_up();
  return 0;
}

int user_player_set_volume_down(void) {
  volume_gear_down();
  return 0;
}

int user_player_speaker_mute(void) {
  gpio_direction_output(IO_PORTB_05, 1);
  return 0;
}

int user_player_speaker_unmute(void) {
  gpio_direction_output(IO_PORTB_05, 0);
  return 0;
}

