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
 * Description : Uniapp uni_nlu.h
 * Author      : chenxiaosong@unisound.com
 * Date        : 2018.6.8
 *
 **************************************************************************/

#ifndef SDK_VUI_VUI_SERVICE_INC_UNI_NLU_H_
#define SDK_VUI_VUI_SERVICE_INC_UNI_NLU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "uni_iot.h"
#include "uni_json.h"

cJSON* NluParseLasr(const char *command);
cJSON* NluParseRasr(const char *rasr_result);
int add_word_into_blacklist(const char *word);
int del_word_from_blacklist(const char *word);

#ifdef __cplusplus
}
#endif
#endif  //  SDK_VUI_VUI_SERVICE_INC_UNI_NLU_H_
