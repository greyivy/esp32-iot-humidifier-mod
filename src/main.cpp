#include <U8x8lib.h>
#include <main.h>
#include <math.h>
#include <tank.h>
#include <net.h>
#include <Fsm.h>
#include <buttons.h>
#include <sensors.h>
#include <TaskScheduler.h>

#include <pins.h>
#include <actions.h>

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/15, /* data=*/4, /* reset=*/16);

Scheduler runner;

unsigned long tankSessionStartedTime = 0;
unsigned long tankSessionTotal = 0;
unsigned long tankTotal = 0;
unsigned long tankRemain = 0;
int tankPercent = 0;

enum States
{
  awaiting_reset,
  off,
  on
};
States currentState;

void reportStatisticsCallback()
{
  mqttClient.publish(topic_percent, String(tankPercent), true, 0);
}

void setCurrentState(States value)
{
  currentState = value;

  mqttClient.publish(topic_empty, currentState == awaiting_reset ? "ON" : "OFF", true, 0);
  mqttClient.publish(topic_power, currentState == on ? "ON" : "OFF", true, 0);

  reportStatisticsCallback();
}

void humidifier_on_enter()
{
  setCurrentState(on);
  Serial.println("humidifier_on_enter");

  tankSessionTotal = 0;
  tankSessionStartedTime = millis();

  digitalWrite(PIN_RELAY, HIGH);
}
void humidifier_off_enter()
{
  setCurrentState(off);
}
void humidifier_on_exit()
{
  Serial.println("humidifier_on_exit");

  // Add to running total
  tankTotal += tankSessionTotal;
  tankSessionTotal = 0;
  tankSessionStartedTime = 0;

  digitalWrite(PIN_RELAY, LOW);
}
void humidifier_awaiting_reset_enter()
{
  setCurrentState(awaiting_reset);
  Serial.println("humidifier_awaiting_reset_enter");
}
void humidifier_awaiting_reset_exit()
{
  Serial.println("humidifier_awaiting_reset_exit");

  tankTotal = 0;
  tankPercent = 100;
}
void humidifier_action_empty()
{
  Serial.println("humidifier_action_empty");

  // Only add to total if the tank is empty (not manual resets)
  addToTankAverage(tankTotal);

  // Beep multiple times
  // TODO a function that accepts a number of times
  digitalWrite(PIN_BUZZER, HIGH);
  delay(100);
  digitalWrite(PIN_BUZZER, LOW);
  delay(100);
  digitalWrite(PIN_BUZZER, HIGH);
  delay(100);
  digitalWrite(PIN_BUZZER, LOW);
  delay(100);
  digitalWrite(PIN_BUZZER, HIGH);
  delay(100);
  digitalWrite(PIN_BUZZER, LOW);
}

void drawDisplayCallback()
{
  int tankSessionTotalSeconds = round((tankSessionTotal / 1000));
  unsigned long tempTankTotal = tankTotal + tankSessionTotal;
  int tankTotalSeconds = round((tempTankTotal / 1000));

  if (currentState == on)
  {
    u8x8.drawString(0, 0, "On        ");

    String string = String(tankSessionTotalSeconds) + " s session";
    char chars[string.length() + 1];

    string.toCharArray(chars, string.length() + 1);

    u8x8.drawString(0, 2, chars);

    String stringTotal = String(tankTotalSeconds) + " s total  ";
    char charsTotal[stringTotal.length() + 1];

    stringTotal.toCharArray(charsTotal, stringTotal.length() + 1);

    u8x8.drawString(0, 4, charsTotal);

    String stringPercent = String(tankPercent) + "%  ";
    char charsPercent[stringPercent.length() + 1];

    stringPercent.toCharArray(charsPercent, stringPercent.length() + 1);

    u8x8.drawString(0, 6, charsPercent);
  }
  else if (currentState == off)
  {
    u8x8.drawString(0, 0, "Off           ");
  }
  else if (currentState == awaiting_reset)
  {
    u8x8.drawString(0, 0, "Reset Plz     ");
    u8x8.drawString(0, 4, "TANK MODE");
    u8x8.drawString(0, 6, getTankMode().c_str());
  }
}

Task reportStatistics(TASK_SECOND * 30, TASK_FOREVER, &reportStatisticsCallback);
Task drawDisplay(TASK_MILLISECOND * 500, TASK_FOREVER, &drawDisplayCallback);

State state_humidifier_on(humidifier_on_enter, NULL, &humidifier_on_exit);
State state_humidifier_off(humidifier_off_enter, NULL, NULL);
State state_humidifier_awaiting_reset(humidifier_awaiting_reset_enter, NULL, &humidifier_awaiting_reset_exit);
Fsm fsm(&state_humidifier_awaiting_reset);

void handleButtonOnOff()
{
  if (currentState == awaiting_reset)
  {
    toggleTankMode();
  }
  else
  {
    fsm.trigger(ACTION_TOGGLE);
  }
}
void handleButtonReset()
{
  fsm.trigger(ACTION_RESET);
}

void setup()
{
  Serial.begin(115200);

  setupPins();

  setupTank();

  runner.init();

  runner.addTask(reportStatistics);
  reportStatistics.enable();

  runner.addTask(drawDisplay);
  drawDisplay.enable();

  u8x8.begin();
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_r);

  setupNet();

  setupSensors();

  fsm.add_transition(&state_humidifier_awaiting_reset, &state_humidifier_on, ACTION_RESET, NULL);
  fsm.add_transition(&state_humidifier_on, &state_humidifier_off, ACTION_TOGGLE, NULL);
  fsm.add_transition(&state_humidifier_off, &state_humidifier_on, ACTION_TOGGLE, NULL);
  fsm.add_transition(&state_humidifier_off, &state_humidifier_on, ACTION_ON, NULL);
  fsm.add_transition(&state_humidifier_on, &state_humidifier_off, ACTION_OFF, NULL);
  fsm.add_transition(&state_humidifier_on, &state_humidifier_awaiting_reset, ACTION_EMPTY, &humidifier_action_empty);
  fsm.add_transition(&state_humidifier_on, &state_humidifier_awaiting_reset, ACTION_RESET, NULL);
  fsm.add_transition(&state_humidifier_off, &state_humidifier_awaiting_reset, ACTION_RESET, NULL);
  fsm.add_transition(&state_humidifier_on, &state_humidifier_awaiting_reset, ACTION_RESET, NULL);
  fsm.add_transition(&state_humidifier_off, &state_humidifier_awaiting_reset, ACTION_RESET, NULL);

  fsm.run_machine();
}

void loop()
{
  if (isTankEmpty())
  {
    fsm.trigger(ACTION_EMPTY);
  }

  loopButtons();

  loopNet();

  runner.execute();

  // Tank calculations
  // TODO all tank related things to tank.cpp
  if (currentState == on)
  {
    tankSessionTotal = millis() - tankSessionStartedTime;
    unsigned long tempTankTotal = tankTotal + tankSessionTotal;

    double percent = (double(tempTankTotal) / double(getTankAverage()) * 100);
    tankPercent = percent < 0 ? 0 : (100 - (round(percent)));

    tankRemain = (getTankAverage() - tempTankTotal) / 60;
  }
}