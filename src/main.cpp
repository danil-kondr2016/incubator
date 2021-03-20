#include <Arduino.h>
#include <Bounce2.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>

#include "pins.h"
#include "strings.h"
#include "letters.h"
#include "automode.h"

#define DISPLAY_I2C_ADDRESS 0x3F
#define REED_SWITCH_DELAY 50
#define TOUCH_BUTTON_DELAY 100

#define DAY 86400000L

#define ON LOW
#define OFF HIGH

#define DEFAULT_TEMPERATURE_HYSTERESIS 0.3F
#define DEFAULT_HUMIDITY_HYSTERESIS 5

#define UPDATE_PERIOD 2000

#define MIN_TEMPERATURE 36
#define MAX_TEMPERATURE 38

#define ALARM_TEMPERATURE 39
#define DEFAULT_STOP_TEMPERATURE 43

#define MAX_ARGS 4
#define MAX_CMD_LENGTH 255
#define MAX_ARG_LENGTH 31

#define NO_PERIOD 0xFFFFFFFFUL

Bounce menuBtn, plusBtn, minusBtn;
Bounce posm45, posn00, posp45;
DHT humiditySensor(DHTPin, DHT22);
OneWire onewire(DSPin);
DallasTemperature thermoSensor(&onewire);
LiquidCrystal_I2C display(DISPLAY_I2C_ADDRESS, 16, 2);

DeviceAddress address;

enum Menu {
  Current = 0,
  Temperature,
  Humidity,
  Rotating,
  Automatic
};

enum Position {
  M = -1, N, P, PosError, Undefined
};

float currentTemperature = 0;
float currentHumidity = 0;

float neededTemperature = 37.5;
float neededHumidity = 50;

float temperatureHysteresis = DEFAULT_TEMPERATURE_HYSTERESIS;
float humidityHysteresis = DEFAULT_HUMIDITY_HYSTERESIS;
float stopTemperature = DEFAULT_STOP_TEMPERATURE;

bool alarm = false;
bool need_update = false;

uint32_t rotationsPerDay = 12;
uint32_t period = DAY/rotationsPerDay;

int currentProgramNumber = 0, newProgramNumber;
int handProgram = 0;
int nProgram = 1;

bool needRotate = false;

ProgramEntry currentProgram;

Menu mode = Current;
Position pos;
Position rotateTo;

char cmd[MAX_CMD_LENGTH+1];
uint8_t cmd_len = 0;

char argv[MAX_ARGS][MAX_ARG_LENGTH+1];
uint8_t arg_len = 0;
uint8_t argc = 0;
uint8_t current_bank = 0;

uint32_t rotateTimer = 0;
int rotateCount = 0;

uint32_t updateTimer = 0;
uint32_t beginTimer = 0;

void initButtons();
void initReedSwitches();

void printScreen();
void handleControls();

Position determinePosition();

void loadProgram(int);

void setup() {
  Serial.begin(9600);

  pinMode(RelayMotor1, OUTPUT);
  pinMode(RelayMotor2, OUTPUT);
  pinMode(RelayWetter, OUTPUT);
  pinMode(RelayCooler, OUTPUT);
  pinMode(RelayHeater, OUTPUT);
  pinMode(RelayRing, OUTPUT);
  pinMode(RelayVentil, OUTPUT);

  digitalWrite(RelayMotor1, OFF);
  digitalWrite(RelayMotor2, OFF);
  digitalWrite(RelayWetter, OFF);
  digitalWrite(RelayCooler, ON);
  digitalWrite(RelayHeater, OFF);
  digitalWrite(RelayRing, OFF);
  digitalWrite(RelayVentil, OFF);
  
  initButtons();
  initReedSwitches();

  humiditySensor.begin();

  thermoSensor.begin();
  thermoSensor.getAddress(address, 0);
  thermoSensor.setResolution(12);
  thermoSensor.setWaitForConversion(false);

  display.init();
  display.backlight();

  display.createChar(1, rus_m);
  display.createChar(2, rus_t);
  display.createChar(3, rus_l);
  display.createChar(4, rus_zh);
  display.createChar(5, rus_n);
  display.createChar(6, rus_myagk_mal);
  display.createChar(7, rus_v);
  
  current_bank = 0;

  pos = determinePosition();

  {
    ProgramEntry *prog_index;
    memcpy_PF(
      (void*)prog_index,
      (uint_farptr_t)programIndex,
      sizeof(programIndex)
    );

    nProgram = prog_index[0].length;
    handProgram = 0;
    currentProgramNumber = handProgram;
  }

  rotateTimer = millis();
  beginTimer = millis();
}

void loop() {
  menuBtn.update();
  plusBtn.update();
  minusBtn.update();

  posm45.update();
  posn00.update();
  posp45.update();

  pos = determinePosition();

  thermoSensor.requestTemperatures();

  currentTemperature = thermoSensor.getTempC(address);
  currentHumidity = humiditySensor.readHumidity();

  for (int i = 0; i < currentProgram.length; i++) {
    if (currentProgram.type != TYPE_AUTO)
      break;
    uint32_t currentTime = millis() - beginTimer;
    if (
          (currentTime >= currentProgram.program[i].begin) 
       && (currentTime <= currentProgram.program[i].end)
       ) 
    {
      neededTemperature = currentProgram.program[i].neededTemp;
      neededHumidity = currentProgram.program[i].neededHumid;
      rotationsPerDay = currentProgram.program[i].rotationsPerDay;
      if (rotationsPerDay == 0)
        period = NO_PERIOD;
      else
        period = DAY / rotationsPerDay;
    }
  }

  if (currentTemperature < neededTemperature - temperatureHysteresis) {
    digitalWrite(RelayHeater, ON);
  } else if (currentTemperature >= neededTemperature) {
    digitalWrite(RelayHeater, OFF);
  }

  if ((currentTemperature >= ALARM_TEMPERATURE) 
   || (isnan(currentTemperature))) {
    digitalWrite(RelayRing, ON);
    alarm = true;
  } else {
    digitalWrite(RelayRing, OFF);
    alarm = false;
  }

  if (currentTemperature >= STOP_TEMPERATURE && alarm) {
    digitalWrite(RelayVentil, ON);
  } else if (currentTemperature <= neededTemperature) {
    digitalWrite(RelayVentil, OFF);
  }

  if (currentHumidity < neededHumidity - humidityHysteresis) {
    digitalWrite(RelayWetter, ON);
  } else {
    digitalWrite(RelayWetter, OFF);
  }

  if (((millis() - updateTimer) >= UPDATE_PERIOD) && mode == Current) {
    need_update = true;
    updateTimer = millis();
  }

  printScreen();
  handleControls();

  if (((millis() - rotateTimer) >= period) && (period != NO_PERIOD) && (!needRotate)) {
    if (pos == M || pos == N) {
      rotateTo = P;
      needRotate = true;
    } else if (pos == P) {
      rotateTo = M;
      needRotate = true;
    }
    
  } else if (period == NO_PERIOD) {
    if (pos != N) {
      rotateTo = N;
      needRotate = true;

      if (pos == P) {
        digitalWrite(RelayMotor1, OFF);
        digitalWrite(RelayMotor2, ON);
        if (determinePosition() == rotateTo)
          digitalWrite(RelayMotor2, OFF);
      } else if (pos == M) {
        digitalWrite(RelayMotor1, ON);
        digitalWrite(RelayMotor2, OFF);
        if (determinePosition() == rotateTo)
          digitalWrite(RelayMotor1, OFF);
      }
    }

  }

  if (needRotate) {
    if (rotateTo == P) {
      digitalWrite(RelayMotor1, ON);
      digitalWrite(RelayMotor2, OFF);
    } else if (rotateTo == M) {
      digitalWrite(RelayMotor1, OFF);
      digitalWrite(RelayMotor2, ON);
    }
    if (determinePosition() == rotateTo) {
        digitalWrite(RelayMotor1, OFF);
        digitalWrite(RelayMotor2, OFF);
        rotateTo = Undefined;
        rotateTimer = millis();
        needRotate = false;
      }
  }
  
}

void initButtons() {
  menuBtn.attach(MenuButton, INPUT);
  menuBtn.interval(TOUCH_BUTTON_DELAY);

  plusBtn.attach(PlusButton, INPUT);
  plusBtn.interval(TOUCH_BUTTON_DELAY);

  minusBtn.attach(MinusButton, INPUT);
  minusBtn.interval(TOUCH_BUTTON_DELAY);
}

void initReedSwitches() {
  posm45.attach(PositionM45, INPUT);
  posm45.interval(REED_SWITCH_DELAY);

  posn00.attach(PositionN00, INPUT);
  posn00.interval(REED_SWITCH_DELAY);

  posp45.attach(PositionP45, INPUT);
  posp45.interval(REED_SWITCH_DELAY);
}

void printScreen() {
  char buf[20];

  if (!need_update)
    return;
  
  switch (mode) {
    case Current: {
      sprintf(buf, "%2.1f\xDF   ", currentTemperature);
      display.setCursor(0, 0);
      display.print("  Te\1n ");
      display.print(buf);

      sprintf(buf, "%3d%%   ", (int)currentHumidity);
      display.setCursor(0, 1);
      display.print("  B\3a\4 ");
      display.print(buf);

      display.setCursor(14, 0);
      if (pos == M)
        display.print("-");
      else if (pos == N)
        display.print("0");
      else if (pos == P)
        display.print("+");
      else if (pos == Undefined)
        display.print("?");
      else if (pos == PosError)
        display.print("E");
      
      break;
    }
    case Temperature: {
      sprintf(buf, "%2.1f\xDF", neededTemperature);
      display.setCursor(0, 0);
      display.print("Te\1nepa\2ypa");
      display.setCursor(0, 1);
      display.print(buf);
      break;
    }
    case Humidity: {
      sprintf(buf, "%3d%%", (int)neededHumidity);
      display.setCursor(0, 0);
      display.print("B\3a\4\5oc\2\6");

      display.setCursor(0, 1);
      display.print(buf);
      break;
    }
    case Rotating: {
      display.setCursor(0, 0);
      display.print("Ko\3-\7o no\7opo\2o\7");

      display.setCursor(0, 1);
      display.print(rotationsPerDay);
      break;
    }
    case Automatic: {
      display.setCursor(0, 0);
      display.print("Pe\4u\1");
      display.setCursor(0, 1);
      display.print("P");
      display.print(newProgramNumber);
      break;
    }
  }

  need_update = false;
}

void handleControls() {
  bool menu = menuBtn.rose();
  bool plus = plusBtn.rose();
  bool minus = minusBtn.rose();

  if (menu || plus || minus)
    need_update = true;

  if (menu) {
    display.clear();
    if (mode == Automatic)
      loadProgram(newProgramNumber);
    mode = (Menu)(((int)mode + 1) % 5);
    if (mode == Automatic)
       newProgramNumber = currentProgramNumber;
  }

  if (mode == Temperature) {
    if (currentProgramNumber != handProgram)
      return;

    if (plus)
      neededTemperature = constrain(neededTemperature+0.1, MIN_TEMPERATURE, MAX_TEMPERATURE);
    else if (minus)
      neededTemperature = constrain(neededTemperature-0.1, MIN_TEMPERATURE, MAX_TEMPERATURE);
  } else if (mode == Humidity) {
    if (currentProgramNumber != handProgram)
      return;

    if (plus)
      neededHumidity = constrain(neededHumidity+1, 0, 100);
    else if (minus)
      neededHumidity = constrain(neededHumidity-1, 0, 100);
  } else if (mode == Rotating) {
    if (currentProgramNumber != handProgram)
      return;

    if (plus) {
      rotationsPerDay = constrain(rotationsPerDay+1, 0, 24);
      if (rotationsPerDay > 0)
        period = DAY/rotationsPerDay;
      else
        period = NO_PERIOD;
    } else if (minus) {
      rotationsPerDay = constrain(rotationsPerDay-1, 0, 24);
      if (rotationsPerDay > 0)
        period = DAY/rotationsPerDay;
      else
        period = NO_PERIOD;
    }
  } else if (mode == Automatic) {
    if (plus)
      newProgramNumber = constrain(newProgramNumber+1, 0, nProgram-1);
    else if (minus)
      newProgramNumber = constrain(newProgramNumber-1, 0, nProgram-1);
  }
}

Position determinePosition() {
  bool m45 = !posm45.read();
  bool n00 = posn00.read();
  bool p45 = !posp45.read();

  if (!m45 && !n00 && !p45)
    return Undefined;

  if (m45 && !(n00 || p45)) {
    return M;
  } else if (n00 && !(m45 || p45)) {
    return N;
  } else if (p45 && !(m45 || n00)) { 
    return P;
  }

  return PosError;
}

void loadProgram(int n_program) {
  ProgramEntry new_entry;
  memcpy_PF(&new_entry, (uint_farptr_t)&programIndex[n_program + 1], sizeof(ProgramEntry));

  currentProgramNumber = n_program;
  currentProgram.type = new_entry.type;
  currentProgram.length = new_entry.length;
  for (int i = 0; i < MAX_PROGRAM_LEN; i++)
    currentProgram.program[i] = new_entry.program[i];
}

void serialEvent() {
  int inc;
  char buf[256];

  while (Serial.available()) {
    inc = Serial.read();
    
    if (inc != '\n' && cmd_len < MAX_CMD_LENGTH) {
      if (inc == '\r')
        continue;
      if (inc == '\b' || inc == '\x7f') {
        cmd[cmd_len--] = '\0';
      } else {
        cmd[cmd_len++] = inc;
      }
    } else {
      cmd[cmd_len] = '\0';
      for (int i = 0; i <= cmd_len; i++) {
        if (arg_len < MAX_ARG_LENGTH && cmd[i] > ' ') {
          argv[argc][arg_len++] = cmd[i];
        } else {
          argv[argc][arg_len] = '\0';
          
          argc++;
          arg_len = 0;
        }
        if (argc >= MAX_ARGS)
          break;
      }

      if (strcmp_P(argv[0], request_state) == 0) {
        sprintf_P(buf, float_fmt, current_temp, (double)currentTemperature);
        Serial.println(buf);
        sprintf_P(buf, float_fmt, current_humid, (double)currentHumidity);
        Serial.println(buf);
        sprintf_P(buf, int_fmt, heater, (digitalRead(RelayHeater) == ON) ? 1 : 0);
        Serial.println(buf);
        sprintf_P(buf, int_fmt, cooler, (digitalRead(RelayCooler) == ON) ? 1 : 0);
        Serial.println(buf);
        sprintf_P(buf, int_fmt, wetter, (digitalRead(RelayWetter) == ON) ? 1 : 0);
        Serial.println(buf);
        sprintf_P(buf, int_fmt, chamber, (int)pos);
        Serial.println(buf);
        sprintf_P(buf, long_fmt, uptime, (millis() - beginTimer) / 1000);
        Serial.println(buf);
        if (alarm) {
          Serial.println(f_overheat);
        }
      } 
      else if (strcmp_P(argv[0], request_config) == 0) {
        sprintf_P(buf, float_fmt, needed_temp, (double)neededTemperature);
        Serial.println(buf);
        sprintf_P(buf, float_fmt, needed_humid, (double)neededHumidity);
        Serial.println(buf);
        sprintf_P(buf, int_fmt, rotations_per_day, rotationsPerDay);
        Serial.println(buf);
        sprintf_P(buf, int_fmt, number_of_programs, nProgram);
        Serial.println(buf);
        sprintf_P(buf, int_fmt, current_program, currentProgramNumber);
        Serial.println(buf);
      }
      else if (strcmp_P(argv[0], needed_temp) == 0) {
        if (currentProgram.type == TYPE_AUTO) {
          return;
        }
        neededTemperature = atof(argv[1]);
        Serial.println(f_success);
      } 
      else if (strcmp_P(argv[0], needed_humid) == 0) {
        if (currentProgram.type == TYPE_AUTO) {
          return;
        }
        neededHumidity = atof(argv[1]);
        Serial.println(f_success);
      }
      else if (strcmp_P(argv[0], rotations_per_day) == 0) {
        if (currentProgram.type == TYPE_AUTO) {
          return;
        }
        rotationsPerDay = atoi(argv[1]);
        period = DAY / rotationsPerDay;
        Serial.println(f_success);
      } 
      else if (strcmp_P(argv[0], switch_to_program) == 0) {
        loadProgram(atoi(argv[1]));
        beginTimer = millis();
        Serial.println(f_success);
      } 
      else {
        Serial.println(F("connected"));
      }

      for (int i = 0; i < cmd_len; i++)
        cmd[i] = '\0';
      cmd_len = 0;

      for (int i = 0; i < argc; i++) {
        for (int j = 0; j < MAX_ARG_LENGTH; j++) {
          argv[i][j] = '\0';
        }
      }
      argc = 0;
    }
  }
}