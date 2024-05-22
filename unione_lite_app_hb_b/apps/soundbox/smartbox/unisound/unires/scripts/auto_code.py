# -*- coding:utf-8 -*-
# coding=utf-8

import sys
import os
import codecs
import shutil
import random
import re
import json
import struct

reload(sys)
sys.setdefaultencoding('utf-8')

os.chdir(sys.path[0])
GPIO_CONFIG_FILE = "./custom_config.json"
GPIO_CONFIG_CODE_FILE = "../../user/uni_auto_control.c"
IOT_DEVICE_MODEL_FILE = "../../iot/device_model.c"

UNI_PIN_PERFIX = 'UNI_PIN_NUM_' 
GPIO_DEF_VOLTAGE_MAP = {}
USER_PIN_MAP = {
    "DM" :"1",
    "DP" :"2",
    "A01":"3",
    "A00":"4",
    "B11":"13",
    "B10":"14",
    "B09":"15",
    "B08":"16",
    "B05":"17",
    "B01":"20",
}

setting_pinmux_str = ''
setting_init_str = ''
setting_iot_funcs_str = ''
setting_handle_kws_event_str = ''

UNI_AUTO_CONTROL_C_TEMPLATE = '''\
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
 * Description : uni_auto_control.c
 * Author      : yuanshifeng@unisound.com
 * Date        : 2021.03.01
 *
 **************************************************************************/
#include "user_gpio.h"
#include "uni_iot.h"

#define LOG_TAG_CONST       UNI_AUTO_CTRL
#define LOG_TAG             "[UNI_AUTO_CTRL]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

static void _user_gpio_set_pinmux(void) {
%(setting_pinmux)s
}

int user_gpio_init(void) {
  _user_gpio_set_pinmux();

%(setting_init)s

  LOGI(LOG_TAG, " %%s success", __func__);
  return 0;
}

%(setting_iot_funcs)s
int user_gpio_handle_action_event(const char *action,
                               uint8_t *need_reply, uint8_t *need_post) {
  if (action != NULL) {
    LOGD(LOG_TAG, "handle kws result action: %%s", action);
%(setting_handle_kws_event)s
  } else {
    return -1;
  }

  return 0;
}
'''

UNI_IOT_FUNC_CASE_TEMPLATE = '''\
    case %(case_val)s: {
%(case_control)s
      break;
    }
'''

UNI_IOT_FUNC_TEMPLATE = '''\
void uni_iot_set_%(func_name)s(int val) {
  switch (val) {
%(case_list)s
    default:
      LOGW(LOG_TAG, "%%s val %%d unknown", __func__, val);
      break;
  }
}
'''

USER_IOT_DEVICE_MODEL_TEMPLATE = '''\
typedef struct DeviceModel_ {
  const char *device_type;
  //STUB: array length
  int property_number;
  PropertyModel property_models[%(p_num)s];
} DeviceModel;

//STUB: model init
static DeviceModel g_device_model = {
  "device_type_stub",
  %(p_num)s,
  {
%(p_list)s
    {"id_stub", ACCESS_MODE_UNKNOWN, DATA_TYPE_UNKNOWN}
  }
};
// auto_code.py edit end

'''


def _gen_uart_init_str(tx_config, rx_config):
    tmp_str = ''
    rx_pin = UNI_PIN_PERFIX + rx_config["pin"]
    if 'params' in tx_config:
        tx_pin = UNI_PIN_PERFIX + tx_config["pin"]
        baud_rate = tx_config["params"]["baud_rate"]
        data_bits = tx_config["params"]["data_bits"]
        stop_bits = tx_config["params"]["stop_bits"]
        parity_bits = tx_config["params"]["parity_bits"]
        if parity_bits == "none":
            real_parity_bits = '0'
        elif parity_bits == "odd":
            real_parity_bits = '1'
        elif parity_bits == "even":
            real_parity_bits = '2'
        else:
            real_parity_bits = '0'
        tmp_str = '  user_uart_init(%(tx_pin)s, %(rx_pin)s, %(baud_rate)s, %(data_bits)s, %(stop_bits)s, %(parity_bits)s);' % \
                  {'tx_pin': tx_pin, 'rx_pin': rx_pin, 'baud_rate': str(baud_rate), 'data_bits': str(data_bits),
                   'stop_bits': str(stop_bits), 'parity_bits': real_parity_bits}
    return tmp_str


def _get_setting_pinmux(pin_config_arry, reply_type='speaker'):
    global setting_pinmux_str
    tmp_str_list = []
    for pin_config in pin_config_arry:
        user_pin = UNI_PIN_PERFIX + pin_config["pin"]
        if 'GPIO' == pin_config["function"][0:4]:
            pin_func = 'UNI_FUNC_GPIO'
        elif 'PWM' == pin_config["function"][0:3]:
            pin_func = 'UNI_FUNC_PWM'
        elif 'UART' == pin_config["function"][0:4]:
            pin_func = 'UNI_FUNC_UART' + '_' + pin_config["function"][-2:]
        else:
            print("Skip Unknown or DEFAULT pin num: " + str(pin_config["pin"]) + ' func ' + pin_config["function"])
            continue
        tmp_str_list.append('  user_pin_set_func(%(user_pin)s, %(pin_func)s);'
                            % {'user_pin': user_pin, 'pin_func': pin_func})
    setting_pinmux_str = '\n'.join(tmp_str_list)


def _gen_gpio_level_set_str(user_pin, def_voltage, is_init=False):
    tmp_str_list = []
    if is_init:
        tmp_str_list.append('  user_gpio_set_direction({user_pin}, UNI_GPIO_DIRECTION_OUTPUT);'.format(
            user_pin=user_pin))
    tmp_str_list.append('  user_gpio_set_level({user_pin}, {def_voltage});'.format(
        user_pin=user_pin, def_voltage=def_voltage))
    return tmp_str_list


def _get_setting_init(pin_config_arry, reply_type='speaker'):
    global setting_init_str
    tmp_str_list = []
    uart_tx = {}
    uart_rx = {}
    for pin_config in pin_config_arry:
        if pin_config["function"][0:4] == 'GPIO':
            user_pin = UNI_PIN_PERFIX + pin_config["pin"]
            if 'params' in pin_config and "def_voltage" in pin_config["params"]:
                def_voltage = pin_config["params"]["def_voltage"]
                def_voltage_str = 'UNI_GPIO_LEVEL_LOW' if def_voltage == 'low' else 'UNI_GPIO_LEVEL_HIGH'
                GPIO_DEF_VOLTAGE_MAP[pin_config["function"]] = '0' if def_voltage == 'low' else '1'
                tmp_str_list.extend(_gen_gpio_level_set_str(user_pin, def_voltage_str, True))
        elif pin_config["function"][0:3] == 'PWM':
            user_pin = UNI_PIN_PERFIX + pin_config["pin"]
            if 'params' in pin_config:
                frequency = pin_config["params"]["frequency"]
                inverse_str = '0'
                duty_str = '0'
                reverse_voltage = pin_config["params"]["reverse_voltage"]
                if reverse_voltage == "enable":
                    inverse_str = '1'
                elif reverse_voltage == "disable":
                    inverse_str = '0'
                else:
                    print("Unknown reverse_voltage: " + reverse_voltage)
                if 'duty' in pin_config["params"]:
                    duty_str = str(pin_config["params"]["duty"])
                tmp_str_list.append('  user_pwm_init({user_pin}, {freq}, {inverse}, {duty});'.format(
                    user_pin=user_pin, freq=frequency, inverse=inverse_str, duty=duty_str))
        elif pin_config["function"][0:4] == 'UART' and pin_config["function"][6:8] == "TX":
            uart_tx[pin_config["function"][4]] = pin_config
            uart_rx.setdefault(pin_config["function"][4], -1)
        elif pin_config["function"][0:4] == 'UART' and pin_config["function"][6:8] == "RX":
            uart_rx[pin_config["function"][4]] = pin_config
            uart_tx.setdefault(pin_config["function"][4], -1)
    for k in uart_tx.keys():
        tmp_str = _gen_uart_init_str(uart_tx[k], uart_rx[k])
        if tmp_str != '':
            tmp_str_list.append(tmp_str)
    setting_init_str = '\n'.join(tmp_str_list)


def _gen_pwm_ctrl_code(function, command, params):
    tmp_str_list = []
    real_pin_num = 'UNI_' + function
    duty_str = str(params["duty"])
    if command == "set":
        tmp_str_list.append('      user_pwm_enable({pin_num}, {duty});'.format(pin_num=real_pin_num, duty=duty_str))
    elif command == "increase":
        tmp_str_list.append('      user_pwm_duty_inc({pin_num}, {duty});'.format(pin_num=real_pin_num, duty=duty_str))
    elif command == "decrease":
        tmp_str_list.append('      user_pwm_duty_dec({pin_num}, {duty});'.format(pin_num=real_pin_num, duty=duty_str))
    return '\n'.join(tmp_str_list)


def _gen_gpio_ctrl_code(function, command, params):
    tmp_str_list = []
    real_pin_num = UNI_PIN_PERFIX + USER_PIN_MAP[str(function[5:])]
    if command == "set":
        level_str = '0'
        if params['voltage'] == 'high':
            level_str = '1'
        tmp_str_list.append('      user_gpio_set_level({pin_num}, {level});'.format(pin_num=real_pin_num,
                                                                                    level=level_str))
    elif command == "pulse":
        period_str = str(params['period'])
        times_str = str(params['times'])
        def_val_str = '0' if function not in GPIO_DEF_VOLTAGE_MAP else GPIO_DEF_VOLTAGE_MAP[function]
        tmp_str_list.append('      user_sw_timer_gpio_pulse({pin_num}, {period}, {times}, {def_val});'.format(
            pin_num=real_pin_num, period=period_str, times=times_str, def_val=def_val_str))
    return '\n'.join(tmp_str_list)


def _gen_uart_ctrl_code(function, command, params):
    tmp_str_list = []
    if command == 'send':
        uart_port = str(int(function[4]))
        send_buf_str = 'send_buf_' + function[0:5]
        data_split = params["data"].split()
        if len(data_split) > 16:
            print("UART send data must less than 16")
            return []
        tmp_str_list.append('      unsigned char {send_buf}[{data_size_str}] = {{{data_str}}};'.format(
            send_buf=send_buf_str, data_size_str=str(len(data_split)), data_str="0x" + ", 0x".join(data_split)))
        tmp_str_list.append('      user_uart_send({uart_port}, {send_buf}, sizeof({send_buf}));'.format(
            send_buf=send_buf_str, uart_port=uart_port))
    return '\n'.join(tmp_str_list)


def _get_setting_handle_kws_event(iotSrcConfig, action_config_arry, reply_type='speaker', iot_enable=True):
    global setting_handle_kws_event_str, setting_iot_funcs_str
    iot_dict = {} # key:(id, type)  value:[id, [val_case_code_list]]
    tmp_str_list = []
    uni_auto_iot_configs = []
    is_first = True
    if iot_enable:
        for properties in iotSrcConfig['properties']:
            iot_dict[(properties["identifier"], 'temp')] = [properties["identifier"], []]
    for action_config in action_config_arry:
        if iot_enable:
            if action_config['dataType'] == 'int' or \
                    action_config['dataType'] == 'enum' or \
                    action_config['dataType'] == 'bool':
                cmd = action_config['identifier'] + '#' + 'val' + '#' + re.findall('\d+', action_config['dataValue'])[0]
            elif action_config['dataType'] == 'null':
                cmd = action_config['identifier'] + '#' + 'null' + '#' + '0'
            else:
                print('warnning: action {} with dataType {} not support auto code'.format(action_config['identifier'],
                                                                                          action_config['dataType']))
                continue
        else:
            cmd = action_config['action']
        if is_first:
            tmp_str_list.append('    if (0 == strcmp(action, "{cmd}")) {{'.format(cmd=cmd))
            is_first = False
        else:
            tmp_str_list.append('    }} else if (0 == strcmp(action, "{cmd}")) {{'.format(cmd=cmd))
        if iot_enable and action_config['dataType'] != 'null':
            cmd_split = cmd.split('#')
            if len(cmd_split) != 3:
                continue
            identifier = (cmd_split[0], cmd_split[1])
            val = cmd_split[2]
            if (cmd_split[0], 'temp') in iot_dict:
                iot_dict.pop((cmd_split[0], 'temp'))
            if identifier not in iot_dict:
                iot_dict[identifier] = [identifier[0], []]
            tmp_str_list.append(
                '      uni_iot_set_{identifier_name}({val});'.format(identifier_name=identifier[0], val=val))
            if 'ctrl_items' in action_config:
                func_list = []
                for ctrl_config in action_config['ctrl_items']:
                    function = ctrl_config['function']
                    command = ctrl_config['command']
                    params = ctrl_config['params']
                    if function[0:3] == "PWM":
                        control = _gen_pwm_ctrl_code(function, command, params)
                    elif function[0:4] == "UART":
                        control = _gen_uart_ctrl_code(function, command, params)
                    elif function[0:4] == "GPIO":
                        control = _gen_gpio_ctrl_code(function, command, params)
                    else:
                        print("Unknown function: " + function)
                        continue
                    func_list.append(control)
            case_list = iot_dict[identifier][1]
            case_list.append(UNI_IOT_FUNC_CASE_TEMPLATE % {'case_val': val, 'case_control': '\n'.join(func_list)})
            iot_dict[identifier][1] = case_list
        else:
            if 'ctrl_items' in action_config:
                for ctrl_config in action_config['ctrl_items']:
                    function = ctrl_config['function']
                    command = ctrl_config['command']
                    params = ctrl_config['params']
                    if function[0:3] == "PWM":
                        tmp_str_list.append(_gen_pwm_ctrl_code(function, command, params))
                    elif function[0:4] == "UART":
                        tmp_str_list.append(_gen_uart_ctrl_code(function, command, params))
                    elif function[0:4] == "GPIO":
                        tmp_str_list.append(_gen_gpio_ctrl_code(function, command, params))
                    else:
                        print("Unknown function: " + function)
    if is_first:
        tmp_str_list.append('    if (NULL != strstr(action, "#val#")) {\n      return 0;')
        is_first = False
    else:
        tmp_str_list.append('    } else if (NULL != strstr(action, "#val#")) {\n      return 0;')
    tmp_str_list.append('    } else {\n      return -1;\n    }')
    setting_handle_kws_event_str = '\n'.join(tmp_str_list)
    tmp_str_list = []
    for value in iot_dict.values():
        uni_auto_iot_configs.append(
            '    {{"{}", ACCESS_MODE_RW, DATA_TYPE_INT}},'.format(value[0]))
        tmp_str_list.append(UNI_IOT_FUNC_TEMPLATE % {'func_name': value[0], 'case_list': ''.join(value[1])})
    setting_iot_funcs_str = '\n'.join(tmp_str_list)
    with open(IOT_DEVICE_MODEL_FILE, 'r') as readfd:
        input_contents = readfd.readlines()
    with open(IOT_DEVICE_MODEL_FILE + '.bak', 'w') as writefd:
        wflag = 0
        for input_line in input_contents:
            if input_line.startswith('typedef struct DeviceModel_ {'):
                wflag = 1
                writefd.write(USER_IOT_DEVICE_MODEL_TEMPLATE % {'p_num': len(uni_auto_iot_configs), \
                    'p_list': '\n'.join(uni_auto_iot_configs)})
            elif 'const char *GetDeviceType() {' in input_line:
                wflag = 0
                writefd.write(input_line)
            elif wflag == 1:
                pass
            else:
                writefd.write(input_line)
    if os.access(IOT_DEVICE_MODEL_FILE + '.bak', os.F_OK):
        shutil.move(IOT_DEVICE_MODEL_FILE + '.bak', IOT_DEVICE_MODEL_FILE)

# create gpio config demo code
def create_gpio_config_code(ctrl):
    global setting_pinmux_str, setting_init_str, setting_iot_funcs_str, setting_handle_kws_event_str
    if ctrl == "auto":
        code_file = open(GPIO_CONFIG_CODE_FILE, "w")
        setting_pinmux_str = ''
        setting_init_str = ''
        setting_handle_kws_event_str = ''
        with open(GPIO_CONFIG_FILE, "r") as json_file:
            config_json = json.load(json_file)
        try:
            pin_config_arry = config_json["local"]["pin_config"]
        except Exception as e:
            print('pin_config prase err:' + str(e))
            code_file.write(UNI_AUTO_CONTROL_C_TEMPLATE % {'setting_pinmux': setting_pinmux_str,
                                                           'setting_init': setting_init_str,
                                                           'setting_iot_funcs': setting_iot_funcs_str,
                                                           'setting_handle_kws_event': '    return -1;'})
            code_file.close()
            return None
        try:
            action_config_arry = config_json["local"]["action_config"]
        except Exception as e:
            print('action_config prase err:' + str(e))
            action_config_arry = []
        try:
            if config_json["local"]['cmd']['iotserver'] == 'enable':
                #print config_json["local"]['cmd']['iotserverJson']
                iotSrcConfig = json.loads(config_json["local"]['cmd']['iotserverJson'])
            else:
                iotSrcConfig = []
        except Exception as e:
            print('iotSrcConfig prase err:' + str(e))
            iotSrcConfig = []

        _get_setting_pinmux(pin_config_arry)
        _get_setting_init(pin_config_arry)
        if len(action_config_arry) > 0 or len(iotSrcConfig) > 0:
            _get_setting_handle_kws_event(iotSrcConfig, action_config_arry, "", config_json["local"]['cmd']['iotserver'] == 'enable')
        code_file.write(UNI_AUTO_CONTROL_C_TEMPLATE % {'setting_pinmux': setting_pinmux_str,
                                                       'setting_init': setting_init_str,
                                                       'setting_iot_funcs': setting_iot_funcs_str,
                                                       'setting_handle_kws_event': setting_handle_kws_event_str if setting_handle_kws_event_str != '' else '    return -1;'})
        code_file.close()


if __name__ == '__main__':
    ctrl = "auto"
    create_gpio_config_code(ctrl)
