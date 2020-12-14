#pragma once

#define PIN_RELAY 25
#define PIN_BUZZER 5

#define PIN_BUTTON_RESET 4
#define PIN_BUTTON_ON_OFF 17

#define PIN_LDR 35

#define PIN_SDA 14
#define PIN_SCL 22

void setupPins();

void beep(int times);