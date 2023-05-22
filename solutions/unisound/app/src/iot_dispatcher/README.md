# 概述

IoT分发模块，可以从不同来源（云端、语音）获取IoT数据及控制命令，并分发给应用设备

# 示例代码

```c


enum iot_property {
    IOT_PROP_LED_SWITCH = 1,
    IOT_PROP_LED_BRIGHTNESS,
};

static ioteng_config_t iot_configs[] = {
    {0, IOT_PROP_LED_SWITCH,        IOT_SRC_SMARTLIVING,    IOT_TYPE_VALUE,     "powerstate"},
    {0, IOT_PROP_LED_SWITCH,        IOT_SRC_VOICE,          IOT_TYPE_VALUE,     "打开灯光",     1},
    {0, IOT_PROP_LED_SWITCH,        IOT_SRC_VOICE,          IOT_TYPE_VALUE,     "关闭灯光",     0},
    {0, IOT_PROP_LED_BRIGHTNESS,    IOT_SRC_SMARTLIVING,    IOT_TYPE_VALUE,     "brightness"},
};

static int led_brightness = 60;

static void iot_user_cb(iot_src_e src, int property)
{
    LOGD(TAG, "iot user cb %d %d", src, property);

    switch (property) {
        case IOT_PROP_LED_SWITCH: {
            int led_on;

            if (ioteng_get_int_result(&led_on) == 0) {
                led_brightness = led_on ? 100 : 0;
                LOGD(TAG, "switch led %s", led_on ? "on" : "off");
            }
            break;
        }

        case IOT_PROP_LED_BRIGHTNESS: {
            if (ioteng_get_int_result(&led_brightness) == 0) {
                led_brightness = led_brightness > 100 ? 100 : led_brightness < 0 ? 0 : led_brightness;
                LOGD(TAG, "set brightness to %d", led_brightness);
            }
            break;
        }
    }

}

ioteng_init(iot_configs, sizeof(iot_configs) / sizeof(ioteng_config_t), iot_user_cb);

```