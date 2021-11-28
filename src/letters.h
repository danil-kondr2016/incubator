#ifndef LETTERS_H
#define LETTERS_H

#include <Arduino.h>

const char rus_ch[8] = {
  0b00100,
  0b00000,
  0b01110,
  0b10001,
  0b10000,
  0b10001,
  0b01110,
  0b00000
}; // ċ

const char rus_zh[8] = {
  0b00100,
  0b00000,
  0b11111,
  0b00010,
  0b00100,
  0b01000,
  0b11111,
  0b00000
}; // ż

#endif
