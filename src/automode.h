#ifndef AUTOMODE_H
#define AUTOMODE_H

#include <Arduino.h>

#define TIME(h, m, s, ms) ((ms) + ((s)*1000) + ((m)*60000) + ((h)*3600000))

#define TYPE_HAND 0x00
#define TYPE_LEN  0x40
#define TYPE_AUTO 0x80

#define MAX_PROGRAM_LEN 5

typedef struct {
    unsigned long begin, end;
    float neededTemp;
    int neededHumid;
    int rotationsPerDay;
} ProgramRecord;

typedef struct {
    int type, length;
    ProgramRecord program[MAX_PROGRAM_LEN];
} ProgramEntry;

const ProgramEntry programIndex[] PROGMEM = {
    {TYPE_LEN, 2,  {}},
    {TYPE_HAND, 0, {}},
    {TYPE_AUTO, 3, {
        {TIME(0, 0, 0, 0), TIME(20*24, 0, 0, 0), 37.7f, 53, 12},
        {TIME(20*24, 0, 0, 0), TIME(20*24 + 12, 0, 0, 0), 37, 78, 0},
        {TIME(20*24 + 12, 0, 0, 0), TIME(21*24, 0, 0, 0), 36, 60, 0}
    }}
};

#endif