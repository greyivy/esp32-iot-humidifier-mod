#include <Arduino.h>
#include <pins.h>

void setupPins()
{
    // Input
    pinMode(PIN_LDR, INPUT);

    // Buttons
    pinMode(PIN_BUTTON_RESET, INPUT_PULLUP);
    pinMode(PIN_BUTTON_ON_OFF, INPUT_PULLUP);

    // Output
    pinMode(PIN_RELAY, OUTPUT);

    pinMode(PIN_BUZZER, OUTPUT);

    pinMode(21, OUTPUT); // HACK fix for OLED brightness issue
}