#include <Arduino.h>
#include <Bounce2.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiNINA.h>

#include "pins.h"
#include "letters.h"
#include "automode.h"
#include "constants.h"

Bounce menuBtn, plusBtn, minusBtn;
Bounce posm45, posn00, posp45;
DHT humiditySensor(DHTPin, DHT22);
OneWire onewire(DSPin);
DallasTemperature thermoSensor(&onewire);
LiquidCrystal_I2C display(DISPLAY_I2C_ADDRESS, 16, 2);

DeviceAddress address;

WiFiServer http(80);

enum Menu {
  Current = 0,
  Temperature,
  Humidity,
  Rotating,
  Automatic,
  ManualRotation,
  N_MENU_MODES
};

#define DELTA_MENU_MODE 1

enum Position {
  M = -1, N, P, PosError, Undefined
};

float currentTemperature = 0;
float currentHumidity = 0;

float neededTemperature = 37.5;
float neededHumidity = 50;

bool alarm = false;
bool need_update = false;

uint32_t rotationsPerDay = 12;
uint32_t period = DAY/rotationsPerDay;

int currentProgramNumber = 0, newProgramNumber;
int handProgram = 0;
int nProgram = 1;

bool needRotate = false;
bool hasChanges = false;
bool wetting = false;

ProgramEntry currentProgram;

Menu mode = Current;
Position pos;
Position rotateTo;

char cmd_str[MAX_CMD_LENGTH+1];
uint8_t cmd_len = 0;

char argv[MAX_ARGS][MAX_ARG_LENGTH+1];
uint8_t arg_len = 0;
uint8_t argc = 0;
uint8_t current_bank = 0;

uint32_t rotateTimer = 0;
int rotateCount = 0;

uint32_t updateTimer = 0;
uint32_t beginTimer = 0;
uint32_t wetTimer = 0;
uint32_t menuSwitchTimer = 0;

void initButtons();
void initReedSwitches();
void initWiFi();

void putPosition();
void putRotateTo();
void printScreen();
void handleControls();
void handleRequest();

Position determinePosition();

void loadProgram(int);

void rotateLeft() {
  digitalWrite(RelayMotorP, OFF);
  digitalWrite(RelayMotorM, ON);
}

void rotateRight() {
  digitalWrite(RelayMotorP, ON);
  digitalWrite(RelayMotorM, OFF);
}

void rotateOff() {
  digitalWrite(RelayMotorP, OFF);
  digitalWrite(RelayMotorM, OFF);
}

void setup() {
  Serial.begin(19200);

  pinMode(RelayMotorP, OUTPUT);
  pinMode(RelayMotorM, OUTPUT);
  pinMode(RelayWetter, OUTPUT);
  pinMode(RelayCooler, OUTPUT);
  pinMode(RelayHeater, OUTPUT);
  pinMode(RelayRing, OUTPUT);
  pinMode(RelayVentil, OUTPUT);

  digitalWrite(RelayMotorP, OFF);
  digitalWrite(RelayMotorM, OFF);
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

  display.createChar(1, rus_zh);
  display.createChar(2, rus_ch);
  
  current_bank = 0;

  pos = determinePosition();
  rotateTo = pos;

  nProgram = programIndex[0].length;
  handProgram = 0;
  currentProgramNumber = handProgram;

  {
    uint32_t posTimer = millis();
    pos = determinePosition();
    display.setCursor(0, 0);
    display.print("Korrektirovka");
    display.setCursor(0, 1);
    display.print("polo\1enija");
  
    if (pos == M) {
      rotateRight();
      rotateTo = P;
    } else {
      rotateLeft();
      rotateTo = M;
    }

    while ((millis() - posTimer) <= ROTATION_PERIOD) {
      if (determinePosition() == rotateTo)
        break;
    }
    rotateOff();
  }

  initWiFi();

  rotateTimer = millis();
  beginTimer = millis();
  wetTimer = millis(); 
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

  if (currentTemperature < neededTemperature - TEMPERATURE_HYSTERESIS) {
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

  if ((millis() - wetTimer) >= WET_PERIOD) {
    if (currentHumidity < neededHumidity - HUMIDITY_HYSTERESIS) {
      digitalWrite(RelayWetter, ON);
      wetting = true;
      if ((millis() - wetTimer) >= WET_PERIOD + WET_TIME) {
        digitalWrite(RelayWetter, OFF);
        wetTimer = millis();
      }
    }
  }

  if (((millis() - updateTimer) >= UPDATE_PERIOD)
      && (mode == Current || mode == ManualRotation)) {
    need_update = true;
    updateTimer = millis();
  }

  printScreen();
  handleControls();

  if (((millis() - rotateTimer) >= period) && (period != NO_PERIOD) && (!needRotate)) {
    if (pos == M || pos == N) {
      rotateTo = P;
      needRotate = true;
    } else if (pos == P || pos == Undefined) {
      rotateTo = M;
      needRotate = true;
    }
    
  } else if (period == NO_PERIOD) {
    if (pos != N) {
      rotateTo = N;
      needRotate = true;
      rotateTimer = millis();

      if (pos == P) {
        rotateLeft();
        if (determinePosition() == rotateTo 
            || (millis() - rotateTimer) >= period + ROTATION_PERIOD)
        {
          rotateOff();
        }
      } else if (pos == M) {
        rotateRight();
        if (determinePosition() == rotateTo 
            || (millis() - rotateTimer) >= period + ROTATION_PERIOD)
        {
          rotateOff();
        }
      }
    }

  }

  if (needRotate) {
    if (rotateTo == P) {
      rotateRight();
    } else if (rotateTo == M) {
      rotateLeft();
    }
    if (determinePosition() == rotateTo 
        || (millis() - rotateTimer) >= period + ROTATION_PERIOD)
    {
        rotateOff();
        rotateTo = Undefined;
        rotateTimer = millis();
        needRotate = false;
      }
  }
  
  handleRequest();
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

void initWiFi() {
  WiFi.beginAP("Incubator");

  http.begin();
}

void putPosition() {
  display.setCursor(15, 0);
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
}

void putRotateTo() {
  display.setCursor(15, 1);
  if (rotateTo == M)
    display.print("-");
  else if (rotateTo == P)
    display.print("+");
  else if (rotateTo == N)
    display.print("0");
}

// Vsö! Objavläjem latinizacyju!
void printScreen() {
  char buf[20];

  if (!need_update)
    return;
  
  switch (mode) {
    case Current: {
      sprintf(buf, "%2.1f\xDF   ", currentTemperature);
      display.setCursor(0, 0);
      display.print("  Temp ");
      display.print(buf);

      sprintf(buf, "%3d%%   ", (int)currentHumidity);
      display.setCursor(0, 1);
      display.print("  Vla\1 ");
      display.print(buf);

      putPosition();
      putRotateTo();
     
      break;
    }
    case Temperature: {
      sprintf(buf, "%2.1f\xDF", neededTemperature);
      display.setCursor(0, 0);
      display.print("Temperatura");
      display.setCursor(0, 1);
      display.print(buf);
      break;
    }
    case Humidity: {
      sprintf(buf, "%d%%   ", (int)neededHumidity);
      display.setCursor(0, 0);
      display.print("Vla\1nostj");

      display.setCursor(0, 1);
      display.print(buf);
      break;
    }
    case Rotating: {
      display.setCursor(0, 0);
      display.print("Kol-vo povorotov");

      display.setCursor(0, 1);
      display.print(rotationsPerDay);
      break;
    }
    case Automatic: {
      display.setCursor(0, 0);
      display.print("Re\1ym");
      display.setCursor(0, 1);
      display.print("P");
      display.print(newProgramNumber);
      break;
    }
    case ManualRotation: {
      display.setCursor(0, 0);
      display.print("Ru\2noj povorot");
      display.setCursor(0, 1);
      display.print("jajic");
      putPosition(); 
      break;
    }
  }

  need_update = false;
}

void handleControls() {
  bool menu = menuBtn.rose();
  bool plus = plusBtn.rose();
  bool minus = minusBtn.rose();
  char buf[20] = {0};

  if (menu || plus || minus) {
    need_update = true;
    menuSwitchTimer = millis();
  }

  if ((menuSwitchTimer) && (millis() - menuSwitchTimer) >= MENU_SWITCH_PERIOD) {
    mode = Current;
    need_update = true;
    menuSwitchTimer = 0;
  }

  if (menu) {
    display.clear();
    if (mode == Automatic)
      loadProgram(newProgramNumber);
    mode = (Menu)(((int)mode + 1) % N_MENU_MODES);
    if (mode == Automatic)
      newProgramNumber = currentProgramNumber;
    else if (mode == Current)
      menuSwitchTimer = 0;
    rotateOff();
  }

  if (mode != Current && (plus || minus))
    hasChanges = true;

  if (mode == Temperature) {
    if (currentProgramNumber != handProgram)
      return;

    if (plus)
      neededTemperature = constrain(
        neededTemperature + DELTA_TEMPERATURE,
        MIN_TEMPERATURE, 
        MAX_TEMPERATURE
      );
    else if (minus)
      neededTemperature = constrain(
        neededTemperature - DELTA_TEMPERATURE,
        MIN_TEMPERATURE,
        MAX_TEMPERATURE
      );
  } else if (mode == Humidity) {
    if (currentProgramNumber != handProgram)
      return;

    if (plus)
      neededHumidity = constrain(
        neededHumidity + DELTA_HUMIDITY, 
        MIN_HUMIDITY, 
        MAX_HUMIDITY
      );
    else if (minus)
      neededHumidity = constrain(
        neededHumidity - DELTA_HUMIDITY, 
        MIN_HUMIDITY, 
        MAX_HUMIDITY
      );
  } else if (mode == Rotating) {
    if (currentProgramNumber != handProgram)
      return;

    if (plus) {
      rotationsPerDay = constrain(
        rotationsPerDay + DELTA_ROT_PER_DAY, 
        MIN_ROT_PER_DAY, 
        MAX_ROT_PER_DAY
      );
      if (rotationsPerDay > 0)
        period = DAY/rotationsPerDay;
      else
        period = NO_PERIOD;
    } else if (minus) {
      rotationsPerDay = constrain(
        rotationsPerDay - DELTA_ROT_PER_DAY, 
        MIN_ROT_PER_DAY, 
        MAX_ROT_PER_DAY
      );
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
  } else if (mode == ManualRotation) {
    if (plusBtn.rose()) {
      rotateRight();
    } else if (minusBtn.rose()) {
      rotateLeft();
    }
    if (plusBtn.fell() || minusBtn.fell()) {
      rotateOff();
    }
    if (plusBtn.read() || minusBtn.read()) {
      putPosition();
    }
  }
}

void handleRequest() {
  WiFiClient client = server.available();
  char request_str[1024]; // TODO define MAX_REQUEST_STR 1024

  if (client) {
    
    client.stop();
  }
}

Position determinePosition() {
  bool m45 = !posm45.read();
  bool n00 = posn00.read();
  bool p45 = !posp45.read();

  if (!m45 && !n00 && !p45) {
    return Undefined;
  }

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
  currentProgramNumber = n_program;
  currentProgram.type = programIndex[n_program + 1].type;
  currentProgram.length = programIndex[n_program + 1].length;
  for (int i = 0; i < MAX_PROGRAM_LEN; i++)
    currentProgram.program[i] = programIndex[n_program + 1].program[i];
}
