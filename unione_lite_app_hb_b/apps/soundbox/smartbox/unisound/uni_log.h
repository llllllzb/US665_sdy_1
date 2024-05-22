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
 * Description : uni_log.h
 * Author      : wufangfang@unisound.com
 * Date        : 2019.09.10
 *
 **************************************************************************/

#ifndef HAL_INC_UNI_LOG_H_
#define HAL_INC_UNI_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "uni_types.h"
#include "debug.h"

#define UNI_LOG(level, tag, fmt, ...) do { \
  u32 time = tls_os_get_time();	                 \
  printf("<%d.%02d>[%s]%s:"fmt"\n", (time/100), (time%100), tag, __func__ , ##__VA_ARGS__); \
} while (0)

#define LOGD(tag, fmt, ...) log_debug(fmt, ##__VA_ARGS__)
#define LOGT(tag, fmt, ...) log_info(fmt, ##__VA_ARGS__)
#define LOGI(tag, fmt, ...) log_info(fmt, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...) log_print(__LOG_WARN,NULL,"<Warn>: " STR(tag) fmt "\r\n", ## __VA_ARGS__)
#define LOGE(tag, fmt, ...) log_error(fmt, ##__VA_ARGS__)
#define LOGR(tag, fmt, ...) printf("<RAW>: " "["tag"]" fmt, ##__VA_ARGS__)

#if defined(CONFIG_ARPT_PRINT) && CONFIG_ARPT_PRINT
#define ArptPrint(...)     printf( __VA_ARGS__ )
#else
#define ArptPrint(...)
#endif

#ifdef __cplusplus
}

#endif
#endif /*HAL_INC_UNI_LOG_H_*/
