#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

const int DHTPin = 2;
const int DSPin = 11;

/* Relays */

const int RelayMotorP   = 3;
const int RelayMotorM   = 4;
const int RelayWetter   = 5;
const int RelayCooler   = 6;
const int RelayHeater   = 7;
const int RelayRing     = 12;
const int RelayVentil   = 13;

/* Touch buttons */

const int MenuButton    = 8;
const int PlusButton    = 9;
const int MinusButton   = 10;

/* Reed switches */

const int PositionM45   = A1;
const int PositionN00   = A2;
const int PositionP45   = A3;

#endif
