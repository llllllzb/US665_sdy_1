/**************************************************************************
 * Copyright (C) 2017-2017  Unisound
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
 * Description : uni_pcm_default.h
 * Author      : wufangfang@unisound.com
 * Date        : 20120.02.28
 *
 **************************************************************************/

#ifndef INC_UNI_REPLAY_DEFINE_H_
#define INC_UNI_REPLAY_DEFINE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "tone_resource.h"

#define NONE                           "[0]"
#define BYE                            "[9]"
#define HI                             "[3]"
#define HI_AC_OFF                      "[2]"
#define NICE_TO_MEET_YOU               "[1]"
#define VOLUME_DEC_NTF                 "[6]"
#define VOLUME_INC_NTF                 "[4]"
#define VOLUME_MIN_NTF                 "[7]"
#define VOLUME_MAX_NTF                 "[5]"
#define VOLUME_MID_NTF                 "[8]"

#define IDEX_TONE_BT_MODE               IDEX_TONE_NUM_4
#define IDEX_TONE_BT_CONN               IDEX_TONE_NUM_4
#define IDEX_TONE_BT_DISCONN            IDEX_TONE_NUM_4
#define IDEX_TONE_MUSIC                 IDEX_TONE_NUM_4
#define IDEX_TONE_POWER_OFF             IDEX_TONE_NUM_4
#define IDEX_TONE_POWER_ON              IDEX_TONE_NUM_4

#ifdef __cplusplus
}
#endif
#endif  //  SDK_PLAYER_INC_UNI_MEDIA_PLAYER_H_
