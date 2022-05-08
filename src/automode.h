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

extern const ProgramEntry programIndex[3];

#endif
