#ifndef CONSTANTS_H
#define CONSTANTS_H

#define DISPLAY_I2C_ADDRESS 0x3F
#define REED_SWITCH_DELAY 50
#define TOUCH_BUTTON_DELAY 100

#define DAY 86400000L

#define ON LOW
#define OFF HIGH

#define TEMPERATURE_HYSTERESIS 0.3F 
#define HUMIDITY_HYSTERESIS 5

#define UPDATE_PERIOD 2000
#define ROTATION_PERIOD 2000

#define WET_PERIOD 120000L
#define WET_TIME 300

#define MENU_SWITCH_PERIOD 300000L

#define MIN_TEMPERATURE 36
#define MAX_TEMPERATURE 38
#define DELTA_TEMPERATURE 0.1

#define MIN_HUMIDITY 0
#define MAX_HUMIDITY 100
#define DELTA_HUMIDITY 1

#define MIN_ROT_PER_DAY 0
#define MAX_ROT_PER_DAY 24
#define DELTA_ROT_PER_DAY 1

#define ALARM_TEMPERATURE 39
#define STOP_TEMPERATURE 43

#define MAX_ARGS 4
#define MAX_CMD_LENGTH 255
#define MAX_ARG_LENGTH 63

#define NO_PERIOD 0xFFFFFFFFUL

/* DS18B20 */

#define TEMP_ERROR -127

enum MilliSeconds {
  MS_9_BIT  = 94,
  MS_10_BIT = 188,
  MS_11_BIT = 375,
  MS_12_BIT = 750 
};


#define BIT_RESOLUTION  12
#define VALUE(x) x

#define xstr(x) str(x)
#define str(x) x

#define CONCAT_3(a,b,c) a##b##c

#define __BIT(x, br) CONCAT_3(x, br, _BIT)
#define _BIT(x, br) __BIT(x, br)

#define RES_X_BIT _BIT(RES_, BIT_RESOLUTION)
#define CONVERSION_TIME _BIT(MS_, BIT_RESOLUTION)

#endif
