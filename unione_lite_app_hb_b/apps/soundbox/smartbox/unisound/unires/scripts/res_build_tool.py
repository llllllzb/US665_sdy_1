# -*- coding:utf-8 -*-
# coding=utf-8

import sys
import os
import codecs
import shutil
import random
import re
import json

from collections import OrderedDict
import auto_code
if '3' == sys.version[0]:
    pass
else:
    reload(sys)
    sys.setdefaultencoding('utf-8')

"""
1. PCM文件命名规则:
    所有文件PCM名字必须以纯数字命名。
    1 ~ 100 预留给系统特殊用途的PCM文件
    101 ~ 用命名配置产生的PCM文件。是根据配置文件cmd_content_total生成的PCM文件

2. cmd_content_total文件规则
    2.1 格式
        "="之前是命令名称，之后是命令词；@之后是回复语，没有回复语时也没有‘@’字符；
        多条命令词用"|"隔开；多条回复语用"|"隔开。

        命令    命令词        回复语（可无，没有回复语时也没有‘@’字符）
        xxx_cmd=命令词1|命令词2@回复语1|回复语2
        xxx_cmd=命令词1|命令词2
    2.2
        cmd_content_total文件应包含所有命令词。
        以”uni“为后缀的命令词是系统默认处理的命令词。
        asr_timeout_uni是虚拟命令词，用于关联识别超时回复用
"""
AUTO_GEN_HEADER = '''\
/**************************************************************************
 * Copyright (C) 2012-2021  Unisound
 *
 * DO NOT MODIFY.
 *
 * This file was generated by res_build_tool.py
 *
 **************************************************************************/
'''
GEN_H_INCLUDE_DIR = "../include/"
DEFAULT_PCM_DIR = "./default_tones/"
PCM_DIR = "./tones/"
WAV_PCM_DIR = "./wav_tones/"
TARGET_TONE_RESOURCE_FILE = GEN_H_INCLUDE_DIR + "tone_resource.h"

#必须是4K的整数倍
PCM_BASE_ADDR = 0x0
PCM_BIN = "./pcm.bin"
TARGET_PCM_ARRAY_FILE = GEN_H_INCLUDE_DIR + "uni_pcm_array.h"
TARGET_PCM_DTAT_FILE = GEN_H_INCLUDE_DIR + "pcm_bin.h"

ASRFIX_DAT = "./asrfix.dat"
ASRFIX_DAT_EN = "./asrfix_en.dat"
ASRFIX_DAT_CN = "./asrfix_cn.dat"
TARGET_ASRFIX_DATA_FILE = GEN_H_INCLUDE_DIR + "asrfix.h"

GRAMMAR_DATA = "./grammar.dat"
TARGET_GRAMMAR_DATA_FILE = GEN_H_INCLUDE_DIR + "grammar.h"

HASHTABLE_SIZE = 0
CUSTOM_CONFIG_FILE = "./custom_config.json"
TARGET_CUST_CONFIG_H_FILE = GEN_H_INCLUDE_DIR + "uni_cust_config.h"

CMD_CONTENT_FILE = "./cmd_reply_data.json"
TARGET_NUL_CONTENT_H_FILE = GEN_H_INCLUDE_DIR + "uni_nlu_content.h"
TARGET_NUL_CONTENT_TYPE_H_FILE = GEN_H_INCLUDE_DIR + "uni_nlu_content_type.h"
DEFAULT_WELCOME_FILENAME = "[1]"

CMD_CODE_START_NUM = 0x00
TARGET_SUCP_CONT_FILE = "./sucp_cmd_map.txt"
TARGET_CMD_CODE_H_FILE = "uni_cmd_code.h"

THRESH_DAT_FILE = "./thresh.dat"
ASR_THRESH_DAT_FILE = "./asr_thresh.dat"


# bin file transform to array
def _bin_2_array(bin_path, arrayName, inc_path):
    bin_file = open(bin_path, "rb")
    inc_file = open(inc_path, "w")
    inc_file.write(AUTO_GEN_HEADER)
    inc_file.write("static const char " + arrayName + "[]" + ' __attribute__ ((aligned(4)))' + " = {\n")
    while True:
        data = bin_file.read(16)  # 每一次读取16个字符
        if not data:
            break
        str_line = "    "  # 空出4个空格
        for i in data:
            str_line += "0x{0:02x}".format(ord(i)) + ", "
        inc_file.write(str_line + "\n")
    inc_file.write("};\n")
    bin_file.close()
    inc_file.close()

def _get_mp3_filelist(dir):
    file_list = []
    numbers = []
    alphas = []
    for home, dirs, files in os.walk(dir):
        cnt = 0
        for filename in files:
            file_name = os.path.splitext(filename)
            if (file_name[1] == ".mp3"):
                if file_name[0].isdigit():
                    idx = int(file_name[0])
                    numbers.append(idx)
                else:
                    alphas.append(filename)
            # 文件名列表，包含完整路径
            #file_list.append(os.path.join(home, filename))
            # 文件名列表，只包含文件名
    
    numbers.sort()
    for idx in numbers:
        filename = str(idx) + '.mp3'
        file_list.append(filename)
    for its in alphas:
        file_list.append(its)
    return file_list

def _copy_default_tons():
    file_list = _get_mp3_filelist(DEFAULT_PCM_DIR)
    for file in file_list:
        file_name = os.path.splitext(file)
        if (file_name[1] == ".mp3"):
            org_file = os.path.join(DEFAULT_PCM_DIR, file)
            dst_file = os.path.join(PCM_DIR, file)
            shutil.copyfile(org_file, dst_file)

def _wav_2_mp3_tons():
    for home, dirs, files in os.walk(WAV_PCM_DIR):
        for filename in files:
            wav_file = os.path.splitext(filename)
            if wav_file[1] == ".wav":
                mp3_file = wav_file[0] + ".mp3"
                wav_file_path = os.path.join(WAV_PCM_DIR, filename)
                mp3_file_path = os.path.join(PCM_DIR, mp3_file)
                os.system("lame --silent -b 16 " + wav_file_path + " " + mp3_file_path)

def _packet_tones(file_name_list):
    packet_arg = ''
    for name in file_name_list:
        packet_arg = packet_arg + ' ./tones/{}.mp3:{}'.format(name, name)
    os.system("../../../../../../cpu/br23/tools/packres -n tone -o tone_py.cfg " + packet_arg)
    shutil.copyfile('tone_py.cfg', '../../../../../../cpu/br23/tools/tone.cfg')
    os.remove('tone_py.cfg')
    
def _create_tone_resource(ctrl):
    if not os.path.exists(PCM_DIR):
        os.mkdir(PCM_DIR)
    else:
        shutil.rmtree(PCM_DIR)
        os.mkdir(PCM_DIR)
    inc_file = open(TARGET_TONE_RESOURCE_FILE, "w")
    _copy_default_tons()
    if (ctrl == "auto"):
        _wav_2_mp3_tons()
    file_list = _get_mp3_filelist(PCM_DIR)
    inc_file.write(AUTO_GEN_HEADER)
    inc_file.write("#ifndef INC_TONE_RESOURCE_H_\n")
    inc_file.write("#define INC_TONE_RESOURCE_H_\n")
    inc_file.write("\n")
    inc_file.write("#include <stdint.h>\n")
    inc_file.write("typedef struct tone_map_s {\n")
    inc_file.write("	uint16_t idx;\n")
    inc_file.write("	uint16_t name;\n")
    inc_file.write("} tone_map_t;\n")
    inc_file.write("\n")
    file_name_list = []
    for file in file_list:
        file_name = os.path.splitext(file)
        if (file_name[1] == ".mp3"):
            file_name_list.append(file_name[0])
    inc_file.write("enum {\n")
    for name in file_name_list:
        if name.isdigit():
            tone_idx_str = '    IDEX_TONE_NUM_{},\n'.format(name)
        else:
            tone_idx_str = '    IDEX_TONE_{},\n'.format(name.upper())
        inc_file.write(tone_idx_str)
    inc_file.write('\n    IDEX_TONE_NONE = 0xFF\n')
    inc_file.write("};\n\n")
    inc_file.write("static const char *const tone_index[] = {\n")
    for name in file_name_list:
        if name.isdigit():
            tone_idx_str = '    [IDEX_TONE_NUM_{}] = SDFILE_RES_ROOT_PATH"tone/{}.*",\n'.format(name, name)
        else:
            tone_idx_str = '    [IDEX_TONE_{}] = SDFILE_RES_ROOT_PATH"tone/{}.*",\n'.format(name.upper(), name)
        inc_file.write(tone_idx_str)
    inc_file.write("};\n\n")
    inc_file.write('static const tone_map_t tone_mapping[] = {\n')
    for name in file_name_list:
        if name.isdigit():
            tone_map_str = 'IDEX_TONE_NUM_{}, {}'.format(name, int(name))
            tone_map_str = '    {' + tone_map_str + '},\n'
            inc_file.write(tone_map_str)
    inc_file.write("};\n\n")
    inc_file.write("#endif\n")
    inc_file.close()
    _packet_tones(file_name_list)

# create ASR acoustic model array file
def _create_asrfix_array(ctrl):
    cust_cfg_file = open(CUSTOM_CONFIG_FILE, "r")
    config_json = json.load(cust_cfg_file)
    language = config_json["language"]
    if language == "en":
        shutil.copyfile(ASRFIX_DAT_EN, ASRFIX_DAT)
    elif language == "cn":
        shutil.copyfile(ASRFIX_DAT_CN, ASRFIX_DAT)
    cust_cfg_file.close()
    _bin_2_array(ASRFIX_DAT, "global_kws_lp_acoutstic_model", TARGET_ASRFIX_DATA_FILE)


# parse thresh data file to get wakeup thresh
def _get_thresh_map(file_name):
    fa_thresh_map = {}
    if os.path.isfile(file_name):
        thresh_file = open(file_name, "r")
        lines = thresh_file.readlines()
        for line in lines:
            fa_thresh = line.split(',')
            if len(fa_thresh) == 2:
                fa_str = fa_thresh[0].strip()
                thresh_str = fa_thresh[1].strip()
                fa_lit = fa_str.split('=')
                thresh_lit = thresh_str.split('=')
                if (len(fa_lit) == 2) and (len(thresh_lit) == 2):
                    if (fa_lit[0].strip() == "fa") and (thresh_lit[0].strip() == "thresh"):
                        fa_thresh_map[fa_lit[1].strip()] = float(thresh_lit[1].strip())
        thresh_file.close()
    return fa_thresh_map


def _get_wakeup_thresh_map():
    return _get_thresh_map(THRESH_DAT_FILE)


def _get_asr_thresh_map():
    return _get_thresh_map(ASR_THRESH_DAT_FILE)


# parse config and modiy lasr timeout
def _modify_config_bin(config_bin):
    ci_cfg_file = open(CUSTOM_CONFIG_FILE, "r")
    config_json = json.load(ci_cfg_file)
    timeout = config_json["local"]["custom_setting"]["timeout_time"]
    ci_cfg_file.close()
    cfg_bin_file = open(config_bin, "r")
    cfg_bin_json = json.load(cfg_bin_file, object_pairs_hook=OrderedDict)
    cfg_bin_file.close()
    cfg_bin_file = open(config_bin, "w")
    if timeout > 0:
        lasr_asr = cfg_bin_json["lasr"]["lasr_asr"]
        if lasr_asr and lasr_asr.has_key("timeout"):
            cfg_bin_json["lasr"]["lasr_asr"]["timeout"] = timeout
    json.dump(cfg_bin_json, cfg_bin_file, indent=4, separators=(',', ': '), encoding="utf-8", ensure_ascii=False,
              sort_keys=False)
    cfg_bin_file.close()

# create ASR grammar model array file
def _create_grammar_array(ctrl):
    _bin_2_array(GRAMMAR_DATA, "global_kws_lp_grammar", TARGET_GRAMMAR_DATA_FILE)

# joint pcm list to string
def _joint_pcm_list_str(pcm_list):
    str = "["
    if len(pcm_list) < 1:
        str += "]"
        return str
    for pcm in pcm_list:
        name = pcm.split('.')[0]
        str += (name + ", ")
    str = str[0:-2] + "]"
    return str

# create uni_cust_config.h
def _create_cust_config_file(ctrl):
    if ctrl == "manual":
        return None
    fo = open(TARGET_CUST_CONFIG_H_FILE, 'w')
    fo.write(AUTO_GEN_HEADER)
    fo.write("#ifndef __UNI_CUST_CONFIG_H__\n")
    fo.write("#define __UNI_CUST_CONFIG_H__\n")
    fo.write("\n")
    ci_cfg_file = open(CUSTOM_CONFIG_FILE, "r")
    config_json = json.load(ci_cfg_file)
    ci_cfg_file.close()
    # UNI_ASR_TIMEOUT
    timeout = config_json["local"]["custom_setting"]["timeout_time"]
    fo.write("#define UNI_ASR_TIMEOUT           " + str(timeout) + "    //ASR timeout Xs\n")
    fo.write("\n")
    # /*---MIC config---*/
    fo.write("#define UNI_MIC_ARRY_TYPE_LINER   0\n")
    fo.write("#define UNI_MIC_ARRY_TYPE_CIRCLE  1\n")
    fo.write("#define ACOUSTIC_MODEL_FIELD_NEAR 0\n")
    fo.write("#define ACOUSTIC_MODEL_FIELD_FAR  1\n")
    fo.write("\n")
    fa_wakeup_thresh_map = _get_wakeup_thresh_map()
    fa_cmd_thresh_map = _get_asr_thresh_map()
    wakeup_sensitivity = config_json["local"]["wakeup"]["sensitivity"]
    cmd_sensitivity = config_json["local"]["cmd"]["sensitivity"]
    if "low" == wakeup_sensitivity:
        fa = "0.1"
        fa_sleep = "0.04"
    elif "high" == wakeup_sensitivity:
        fa = "0.4"
        fa_sleep = "0.1"
    else:
        fa = "0.2"
        fa_sleep = "0.05"
    if "low" == cmd_sensitivity:
        fa_cmd = "0.1"
    elif "high" == cmd_sensitivity:
        fa_cmd = "0.8"
    else:
        fa_cmd = "0.4"
    if fa in fa_wakeup_thresh_map:
        fo.write("#define KWS_WAKEUP_SCORE_THRED    (%s)\n" % fa_wakeup_thresh_map[fa])
    if fa_sleep in fa_wakeup_thresh_map:
        fo.write("#define KWS_SLEEP_SCORE_THRED    (%s)\n" % fa_wakeup_thresh_map[fa_sleep])
    if fa_cmd in fa_cmd_thresh_map:
        fo.write("#define KWS_CMD_SCORE_THRED       (%s)\n" % fa_cmd_thresh_map[fa_cmd])
    fo.write("#define KWS_HASHTABLE_SIZE        (%s)\n" % str(HASHTABLE_SIZE))
    fo.write("\n")
    fo.write("/*---MIC config---*/\n")
    micarray = config_json["local"]["micarray"]
    # type of acoustic model
    field = "far"
    if micarray.has_key("field"):
        field = micarray["field"]
    if "near" == field:
        fo.write("#define ACOUSTIC_MODEL_FIELD  ACOUSTIC_MODEL_FIELD_NEAR\n")
    else:
        fo.write("#define ACOUSTIC_MODEL_FIELD  ACOUSTIC_MODEL_FIELD_FAR\n")
    mic_num = micarray["number"]
    fo.write("#define UNI_MIC_ARRY_NUM          " + str(mic_num) + "\n")
    if micarray.has_key("topo"):
        mic_type = micarray["topo"]
        if "circle" == mic_type:
            fo.write('#define UNI_MIC_ARRY_TYPE         UNI_MIC_ARRY_TYPE_CIRCLE\n')
        else:
            fo.write('#define UNI_MIC_ARRY_TYPE         UNI_MIC_ARRY_TYPE_LINER\n')
    if micarray.has_key("distance"):
        mic_distance = micarray["distance"]
        fo.write('#define UNI_MIC_ARRY_DISTANCE     ' + str(mic_distance) + "\n")
    fo.write("\n")
    # DEFAULT_PCM_WELCOME and DEFAULT_PCM_ASR_TIMEOUT
    cmd_replay_file = open(CMD_CONTENT_FILE, "r")
    cmd_replay_json = json.load(cmd_replay_file)
    cmd_replay_file.close()
    welcome_value = DEFAULT_WELCOME_FILENAME
    default_welcome = True
    for item in cmd_replay_json:
        if item["cmd"] == "asr_timeout_uni":
            value = _joint_pcm_list_str(item["pcm"])
            fo.write("#define DEFAULT_PCM_ASR_TIMEOUT   \"" + value + "\"\n")
        elif item["cmd"] == "bringup_uni":
            welcome_value = _joint_pcm_list_str(item["pcm"])
            default_welcome = False
        elif item["cmd"] == "wakeup_uni":
            value = _joint_pcm_list_str(item["pcm"])
            fo.write("#define DEFAULT_PCM_WAKEUP        \"" + value + "\"\n")
        elif item["cmd"] == "exitUni":
            value = _joint_pcm_list_str(item["pcm"])
            fo.write("#define DEFAULT_PCM_SLEEP         \"" + value + "\"\n")
        elif item["cmd"] == "volumeMinUni":
            value = _joint_pcm_list_str(item["pcm"])
            fo.write("#define DEFAULT_PCM_VOL_MIN       \"" + value + "\"\n")
        elif item["cmd"] == "volumeMaxUni":
            value = _joint_pcm_list_str(item["pcm"])
            fo.write("#define DEFAULT_PCM_VOL_MAX       \"" + value + "\"\n")
        elif item["cmd"] == "volumeMidUni":
            value = _joint_pcm_list_str(item["pcm"])
            fo.write("#define DEFAULT_PCM_VOL_MID       \"" + value + "\"\n")
        elif item["cmd"] == "volumeUpUni":
            value = _joint_pcm_list_str(item["pcm"])
            fo.write("#define DEFAULT_PCM_VOL_UP        \"" + value + "\"\n")
        elif item["cmd"] == "volumeDownUni":
            value = _joint_pcm_list_str(item["pcm"])
            fo.write("#define DEFAULT_PCM_VOL_DOWN      \"" + value + "\"\n")
        else:
            cmd = item["cmd"]
            if ( -1 != cmd.find("UNI_LOCAL_TONE_")):
                pos = cmd.find("LOCAL_TONE")
                cmd = cmd[pos:]
                value = _joint_pcm_list_str(item["pcm"])
                fo.write("#define " + cmd + "      \"" + value + "\"\n")
    if len(welcome_value) > 2: # skip none welcome ("[]")
        if default_welcome:
            wel_value = welcome_value[1:-1]
            org_filepath = os.path.join(DEFAULT_PCM_DIR)
            dst_filepath = os.path.join(PCM_DIR)
            shutil.copyfile(org_filepath+wel_value+".mp3", dst_filepath+wel_value+".mp3")
        fo.write("#define DEFAULT_PCM_WELCOME       \"" + welcome_value + "\"\n")

    fo.write("\n")
    fo.write("#endif /*__UNI_CUST_CONFIG_H__*/\n")
    fo.close()


# create NLU contents
def _create_nlu_contents_file(ctrl):
    if ctrl == "manual":
        return None
    type_str = "enum {\n"
    map_str = ''
    content_str = ''
    content_ext_str = ''
    cmd_replay_file = open(CMD_CONTENT_FILE, "r")
    cmd_replay_json = json.load(cmd_replay_file)
    cmd_replay_file.close()
    ci_cfg_file = open(CUSTOM_CONFIG_FILE, "r")
    config_json = json.load(ci_cfg_file)
    iot_enable = config_json["local"]['cmd']['iotserver'] == 'enable'
    ci_cfg_file.close()
    check_hash_list = []
    pat_inv = re.compile ('[^\w]')
    for item in cmd_replay_json:
        cmd = item["cmd"]
        if item["cmd"] == "asr_timeout_uni":
            continue
        elif item["cmd"] == "bringup_uni":
            continue
        elif -1 != cmd.find("UNI_LOCAL_TONE_"):
            continue
        else:
            cmd_list = item["cmd_word"]
            if "NULL" == cmd_list[0]:
                continue
            # type enum
            item_cmd = item["cmd"]
            if len(item_cmd.split('#')) == 4:
                name, identifier, dataType, dataValue = item_cmd.split('#')
                if dataType == 'null':
                    item_cmd = identifier + '#' + 'null' + '#' + '0'
                elif dataType == 'int' or dataType == 'enum' or dataType == 'bool':
                    item_cmd = identifier + '#' + 'val' + '#' + re.findall("\d+",dataValue)[0]
                else:
                    item_cmd = identifier + '#' + dataType + '#' + dataValue.split('-')[0]
            item_cmd_c = item_cmd
            while pat_inv.search(item_cmd_c) is not None:
                item_cmd_c = item_cmd_c.replace(pat_inv.search(item_cmd_c).group(), '_')
            type = "eCMD_" + item_cmd_c
            type_str += "  " + type + ",\n"
            # g_nlu_content_mapping
            hash_list = item["hash"]
            i = 0
            for hash in hash_list:
                if check_hash_list.count(hash) > 0:
                    map_str += "  {" + str(hash) + "U/*" + cmd_list[i] + "*/, " + type + ", \"" + cmd_list[i] + "\"},\n"
                else:
                    map_str += "  {" + str(hash) + "U/*" + cmd_list[i] + "*/, " + type + ", NULL},\n"
                check_hash_list.append(hash)
                i += 1
            # g_nlu_content_str[]
            content_ext_str += "  [" + type + "] = \"{\\\"cmd\\\":\\\"" + item_cmd + "\\\",\\\"pcm\\\":\\\"" + _joint_pcm_list_str(item["pcm"]) + "\\\"}\",\n"
    global HASHTABLE_SIZE
    HASHTABLE_SIZE = len(check_hash_list)
    type_str += '};\n'
    inc_file = open(TARGET_NUL_CONTENT_TYPE_H_FILE, "w")
    inc_file.write(AUTO_GEN_HEADER)
    inc_file.write("""\
#ifndef INC_UNI_NLU_CONTENT_TYPE_H_
#define INC_UNI_NLU_CONTENT_TYPE_H_
typedef struct {
  unsigned int  key_word_hash_code; /* 存放识别词汇对应的hashcode */
  unsigned char nlu_content_str_index; /* 存放nlu映射表中的索引，实现多个识别词汇可对应同一个nlu，暂支持256条，如果不够换u16 */
  char          *hash_collision_orginal_str; /* 类似Java String equal，当hash发生碰撞时，赋值为识别词汇，否则设置为NULL */
} uni_nlu_content_mapping_t;

""")
    inc_file.write(type_str)
    inc_file.write("\n")
    inc_file.write("#endif\n")
    inc_file.close()

    inc_file = open(TARGET_NUL_CONTENT_H_FILE, "w")
    inc_file.write(AUTO_GEN_HEADER)
    inc_file.write('''\
#ifndef INC_UNI_NLU_CONTENT_H_
#define INC_UNI_NLU_CONTENT_H_

#include "uni_nlu_content_type.h"

const char* g_nlu_content_str[] = {{
{nlu_content_ext_flash_str}
}};

/*TODO perf sort by hashcode O(logN), now version O(N)*/
const uni_nlu_content_mapping_t g_nlu_content_mapping[] = {{
{nlu_content_mapping_str}
}};

#endif
'''.format(nlu_content_ext_flash_str=content_ext_str[:-1],
           nlu_content_mapping_str=map_str[:-1]))
    inc_file.close()


# get chacksum
def _get_check_sum(cmd_code):
    c = 0x5A
    c = (c + cmd_code) & 0xFF
    return c


# create cmd code array file
def _create_cmd_code_array(ctrl):
    if ctrl == "manual":
        return None
    inc_file = open(TARGET_CMD_CODE_H_FILE, "w")
    inc_file.write(AUTO_GEN_HEADER)
    map_file = open(TARGET_SUCP_CONT_FILE, "w")
    inc_file.write("#ifndef INC_UNI_CMD_CODE_H_\n")
    inc_file.write("#define INC_UNI_CMD_CODE_H_\n")
    inc_file.write("\n")
    inc_file.write("typedef struct {\n")
    inc_file.write("  uni_u8      cmd_code; /* cmd code fro send base on SUCP */\n")
    inc_file.write("  const char  *cmd_str; /* action string on UDP */;\n")
    inc_file.write("} cmd_code_map_t;\n")
    inc_file.write("\n")
    inc_file.write("const cmd_code_map_t g_cmd_code_arry[] = {\n")
    cmd_replay_file = open(CMD_CONTENT_FILE, "r")
    cmd_replay_json = json.load(cmd_replay_file)
    cmd_replay_file.close()
    cmd_code = CMD_CODE_START_NUM
    for item in cmd_replay_json:
        cmd = item["cmd"]
        if item["cmd"] == "asr_timeout_uni":
            continue
        elif item["cmd"] == "bringup_uni":
            continue
        elif -1 != cmd.find("UNI_LOCAL_TONE_"):
            continue
        else:
            cmd_list = item["cmd_word"]
            if "NULL" == cmd_list[0]:
                continue
            inc_file.write("  {" + hex(cmd_code) + ", \"" + item["cmd"] + "\"},\n")
            map_file.write(item["cmd"] + "=" + '|'.join(item["cmd_word"]) + "@" + '|'.join(item["reply"]) + "\n")
            map_file.write("    0x5a " + hex(cmd_code) + " 0x0 0x0 " + hex(_get_check_sum(cmd_code)) + "\n")
        cmd_code += 1
    inc_file.write("};\n")
    inc_file.write("\n")
    inc_file.write("#endif\n")
    inc_file.close()
    map_file.close()


if __name__ == '__main__':
    ctrl = ""
    if len(sys.argv) > 1:
        if sys.argv[1] == "auto":
            ctrl = "auto"
        elif sys.argv[1] == "manual":
            ctrl = "manual"
        else:
            print("Use: python res_build_tool.py auto|manual(default auto if none)")
            sys.exit()
    else:
        ctrl = "auto"
    _create_tone_resource(ctrl)
    _create_asrfix_array(ctrl)
    _create_grammar_array(ctrl)
    _create_cust_config_file(ctrl)
    _create_nlu_contents_file(ctrl)
    _create_cust_config_file(ctrl)
    auto_code.create_gpio_config_code(ctrl)
