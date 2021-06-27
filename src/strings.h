#ifndef STRINGS_H
#define STRINGS_H

#include <avr/pgmspace.h>

const char request_state[] PROGMEM = "request_state";
const char request_config[] PROGMEM = "request_config";

const char current_temp[] PROGMEM = "current_temp";
const char current_humid[] PROGMEM = "current_humid";

const char needed_temp[] PROGMEM = "needed_temp";
const char needed_humid[] PROGMEM = "needed_humid";

const char rotations_per_day[] PROGMEM = "rotations_per_day";
const char number_of_programs[] PROGMEM = "number_of_programs";
const char switch_to_program[] PROGMEM = "switch_to_program";
const char current_program[] PROGMEM = "current_program";
const char uptime[] PROGMEM = "uptime";

const char heater[] PROGMEM = "heater";
const char cooler[] PROGMEM = "cooler";
const char wetter[] PROGMEM = "wetter";
const char chamber[] PROGMEM = "chamber";

const char rotate_to[] PROGMEM = "rotate_to";
const char rotate_left[] PROGMEM = "rotate_left";
const char rotate_right[] PROGMEM = "rotate_right";
const char rotate_off[] PROGMEM = "rotate_off";

const char set_uptime[] PROGMEM = "set_uptime";
const char set_menu[] PROGMEM = "set_menu";

#define f_success F("success")
#define f_overheat F("overheat")
#define f_error F("error")

const char float_fmt[] PROGMEM = "%S %.2f";
const char int_fmt[] PROGMEM = "%S %d";
const char long_fmt[] PROGMEM = "%S %lu";

#endif
