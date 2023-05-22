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
GPIO_CONFIG_CODE_FILE = "../../src/unisound/user/uni_auto_control.c"
APP_IOT_FILE = "../../src/app_iot.c"

GPIO_DEF_VOLTAGE_MAP = {}
PWM_PORT_MAP = {}
USER_PIN_MAP = {
    "1": "PB20",
    "2": "PB19",
    "13": "PA0",
    "14": "PA1",
    "15": "PA4",
    "16": "PA7",
    "18": "PB0",
    "19": "PB1",
    "20": "PB2",
    "21": "PB3",
    "22": "PB4",
    "23": "PB5",
    "26": "PB6",
    "27": "PB7",
    "28": "PB8",
    "29": "PB9",
    "30": "PB10",
    "32": "PB11",
}

setting_pinmux_str = ''
setting_init_str = ''
setting_iot_funcs_str = ''
setting_handle_kws_event_str = ''

'''
    PA0_SPI_CS     = 0,
    PA0_IIS_DO     = 1,
    PA0_IIS_MCLK   = 2,
    PA0_PWM        = 3,

    PA1_IIS_WS     = 0,
    PA1_IIC_SCL    = 1,
    PA1_PWM        = 2,
    PA1_ADC        = 5,

    PA4_IIS_CK     = 0,
    PA4_IIC_SDA    = 1,
    PA4_PWM        = 2,
    PA4_ADC        = 5,

    PA7_SPI_DO     = 0,
    PA7_IIS_DI     = 1,
    PA7_IIS_EXTCLK = 2,
    PA7_PWM        = 3,

    PB0_UART3_TX   = 0,
    PB0_SPI_DI     = 1,
    PB0_PWM        = 2,

    PB1_UART3_RX   = 0,
    PB1_SPI_CK     = 1,
    PB1_PWM        = 2,

    PB2_UART2_TX   = 0,
    PB2_SPI_CK     = 1,
    PB2_PWM        = 2,

    PB3_UART2_RX   = 0,
    PB3_SPI_DI     = 1,
    PB3_PWM        = 2,

    PB4_UART2_RTS  = 0,
    PB4_UART4_TX   = 1,
    PB4_SPI_CS     = 2,

    PB5_UART2_CTS  = 0,
    PB5_UART4_RX   = 1,
    PB5_SPI_DO     = 2,

    PB6_UART1_TX   = 0,

    PB7_UART1_RX   = 0,

    PB8_IIS_CK     = 0,

    PB9_IIS_WS     = 0,

    PB10_IIS_DI    = 0,

    PB11_IIS_DO    = 0,

    PB19_UART0_TX  = 0,
    PB19_UART1_RTS = 1,
    PB19_IIC_SDA   = 2,
    PB19_PWM       = 3,

    PB20_UART0_RX  = 0,
    PB20_UART1_CTS = 1,
    PB20_IIC_SCL   = 2,
    PB20_PWM       = 3,

    PIN_FUNC_GPIO  = 5
'''

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

#define LOG_TAG "[uni_auto_ctrl]"

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


def _gen_uart_init_str(pin_config):
    tmp_str = ''
    if 'params' in pin_config:
        user_pin = USER_PIN_MAP[pin_config['pin']]
        baud_rate = pin_config["params"]["baud_rate"]
        data_bits = pin_config["params"]["data_bits"]
        stop_bits = pin_config["params"]["stop_bits"]
        parity_bits = pin_config["params"]["parity_bits"]
        if parity_bits == "none":
            real_parity_bits = '0'
        elif parity_bits == "odd":
            real_parity_bits = '1'
        elif parity_bits == "even":
            real_parity_bits = '2'
        else:
            real_parity_bits = '0'
        tmp_str = '  user_uart_init(%(user_pin)s, %(baud_rate)s, %(data_bits)s, %(stop_bits)s, %(parity_bits)s);' % \
                  {'user_pin': user_pin, 'baud_rate': str(baud_rate), 'data_bits': str(data_bits),
                   'stop_bits': str(stop_bits), 'parity_bits': real_parity_bits}
    return tmp_str


def _get_setting_pinmux(pin_config_arry, reply_type='speaker'):
    global setting_pinmux_str
    tmp_str_list = []
    for pin_config in pin_config_arry:
        if 'GPIO' == pin_config["function"][0:4]:
            user_pin = 'P' + str(pin_config["function"][-3]) + str(int(pin_config["function"][-2:]))
            pin_func = 'PIN_FUNC_GPIO'
        elif 'PWM' == pin_config["function"][0:3]:
            user_pin = USER_PIN_MAP[pin_config['pin']]
            pin_func = user_pin + '_PWM'
        elif 'UART' == pin_config["function"][0:4]:
            user_pin = USER_PIN_MAP[pin_config['pin']]
            pin_func = user_pin + '_' + pin_config["function"][0:8]
        else:
            print("Unknown pin num: " + str(pin_config["pin"]) + ' func ' + pin_config["function"])
            continue
        tmp_str_list.append('  user_pin_set_func(%(user_pin)s, %(pin_func)s);'
                            % {'user_pin': user_pin, 'pin_func': pin_func})
    setting_pinmux_str = '\n'.join(tmp_str_list)


def _gen_gpio_level_set_str(user_pin, def_voltage, is_init=False):
    tmp_str_list = []
    if is_init:
        tmp_str_list.append('  user_gpio_set_direction({user_pin}, GPIO_DIRECTION_OUTPUT);'.format(
            user_pin=user_pin))
    tmp_str_list.append('  user_gpio_set_level({user_pin}, {def_voltage});'.format(
        user_pin=user_pin, def_voltage=def_voltage))
    return tmp_str_list


def _get_setting_init(pin_config_arry, reply_type='speaker'):
    global setting_init_str
    tmp_str_list = []
    for pin_config in pin_config_arry:
        if pin_config["function"][0:4] == 'GPIO':
            user_pin = 'P' + str(pin_config["function"][-3]) + str(int(pin_config["function"][-2:]))
            if 'params' in pin_config and "def_voltage" in pin_config["params"]:
                def_voltage = pin_config["params"]["def_voltage"]
                def_voltage_str = 'UNI_GPIO_LEVEL_LOW' if def_voltage == 'low' else 'UNI_GPIO_LEVEL_HIGH'
                GPIO_DEF_VOLTAGE_MAP[pin_config["function"]] = '0' if def_voltage == 'low' else '1'
                tmp_str_list.extend(_gen_gpio_level_set_str(user_pin, def_voltage_str, True))
        elif pin_config["function"][0:3] == 'PWM':
            user_pin = USER_PIN_MAP[pin_config['pin']]
            if 'params' in pin_config:
                PWM_PORT_MAP[pin_config["function"]] = user_pin
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
            if 'params' in pin_config:
                tmp_str = _gen_uart_init_str(pin_config)
                if tmp_str != '':
                    tmp_str_list.append(tmp_str)
    setting_init_str = '\n'.join(tmp_str_list)


def _gen_pwm_ctrl_code(function, command, params):
    tmp_str_list = []
    real_pin_num = PWM_PORT_MAP[function]
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
    real_pin_num = 'P' + str(function[-3]) + str(int(function[-2:]))
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
    iot_dict = {}
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
            '  {{0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "{}"}},\n'.format(value[0]))
        tmp_str_list.append(UNI_IOT_FUNC_TEMPLATE % {'func_name': value[0], 'case_list': ''.join(value[1])})
    setting_iot_funcs_str = '\n'.join(tmp_str_list)
    with open(APP_IOT_FILE, 'r') as readfd:
        input_contents = readfd.readlines()
    with open(APP_IOT_FILE + '.bak', 'w') as writefd:
        wflag = 0
        for input_line in input_contents:
            if input_line.startswith('const static iotdispatcher_config_t iot_configs[]'):
                writefd.write(input_line)
                writefd.writelines(uni_auto_iot_configs)
            elif '0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALU' in input_line:
                pass
            else:
                writefd.write(input_line)
    if os.access(APP_IOT_FILE + '.bak', os.F_OK):
        shutil.move(APP_IOT_FILE + '.bak', APP_IOT_FILE)


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
