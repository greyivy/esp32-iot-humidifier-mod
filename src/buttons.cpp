#include <Arduino.h>
#include <main.h>

#include <pins.h>
#include <actions.h>

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

int buttonResetState;
int lastButtonResetState = LOW;
int buttonOnOffState;
int lastButtonOnOffState = LOW;
void loopButtons()
{
    // Reset
    int reading = digitalRead(PIN_BUTTON_RESET);
    if (reading != lastButtonResetState)
    {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        if (reading != buttonResetState)
        {
            buttonResetState = reading;

            if (buttonResetState == LOW)
            {
                beep(1);
                handleButtonReset();
            }
        }
    }
    lastButtonResetState = reading;

    // OnOff
    reading = digitalRead(PIN_BUTTON_ON_OFF);
    if (reading != lastButtonOnOffState)
    {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        if (reading != buttonOnOffState)
        {
            buttonOnOffState = reading;

            if (buttonOnOffState == LOW)
            {
                beep(1);
                handleButtonOnOff();
            }
        }
    }
    lastButtonOnOffState = reading;
}