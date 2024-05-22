#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*
 *  板级配置选择
 */

#define CONFIG_BOARD_US665P3_DEMO


#define DAC_OUTPUT_MONO_L                  0    //左声道
#define DAC_OUTPUT_MONO_R                  1    //右声道
#define DAC_OUTPUT_LR                      2    //立体声
#define DAC_OUTPUT_MONO_LR_DIFF            3    //单声道差分输出

#define DAC_OUTPUT_DUAL_LR_DIFF            6    //双声道差分
#define DAC_OUTPUT_FRONT_LR_REAR_L         7    //三声道单端输出 前L+前R+后L (不可设置vcmo公共端)
#define DAC_OUTPUT_FRONT_LR_REAR_R         8    //三声道单端输出 前L+前R+后R (可设置vcmo公共端)
#define DAC_OUTPUT_FRONT_LR_REAR_LR        9    //四声道单端输出(不可设置vcmo公共端)

#include "board_us665p3_demo/board_us665p3_demo_cfg.h"


#define  DUT_AUDIO_DAC_LDO_VOLT   							DACVDD_LDO_2_90V


#endif
