#include <U8g2lib.h>
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

#define SECS_PER_MIN (60UL)
#define SECS_PER_HOUR (3600UL)
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN)
#define numberOfHours(_time_) (_time_ / SECS_PER_HOUR)

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/15, /* data=*/4, /* reset=*/16);

Scheduler runner;

unsigned long tankSessionStartedTime = 0;
unsigned long tankSessionTotalMillis = 0;
unsigned long tankTotalMillis = 0;
unsigned long tankRemainMillis = 0;
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
  mqttClient.publish(topic_remain, String(tankRemainMillis), true, 0);
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

  tankSessionTotalMillis = 0;
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
  tankTotalMillis += tankSessionTotalMillis;
  tankSessionTotalMillis = 0;
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

  tankTotalMillis = 0;
  tankPercent = 100;
}
void humidifier_action_empty()
{
  Serial.println("humidifier_action_empty");

  // Only add to total if the tank is empty (not manual resets)
  addToTankAverage(tankTotalMillis);

  tankPercent = 0;
  tankRemainMillis = 0;

  beep(3);
}

void drawDisplayPercent()
{
  // Frame top
  u8g2.drawLine(7, 2, 33, 2);
  u8g2.drawLine(5, 3, 6, 3);
  u8g2.drawLine(34, 3, 35, 3);
  u8g2.drawPixel(4, 4);
  u8g2.drawPixel(36, 4);
  u8g2.drawLine(3, 5, 3, 6);
  u8g2.drawLine(37, 5, 37, 6);

  // Remainder of frame
  u8g2.drawLine(2, 7, 2, 63);
  u8g2.drawLine(38, 7, 38, 63);
  u8g2.drawLine(2, 63, 38, 63);

  // Fill
  if (tankPercent > 0)
  {
    int fillHeight = (57 * ((float)tankPercent / (float)100));
    u8g2.drawBox(4, 4 + (57 - fillHeight), 33, fillHeight + 1); // Tank percent fill
  }

  // Frame top overlay
  u8g2.setDrawColor(0); // Black
  u8g2.drawLine(5, 4, 7, 4);
  u8g2.drawLine(33, 4, 35, 4);
  u8g2.drawPixel(5, 5);
  u8g2.drawPixel(35, 5);
  u8g2.drawLine(4, 5, 4, 7);
  u8g2.drawLine(36, 5, 36, 7);

  // 50% line
  u8g2.drawLine(3, 34, 6, 34);
  u8g2.drawLine(34, 34, 37, 34);

  if (tankPercent == 0)
  {
    u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2.setDrawColor(1);           // Default
    u8g2.drawGlyph(14, 24, 0x0118); // Reset icon
  }
  else
  {
    u8g2.setFont(u8g2_font_helvB12_tf);
    u8g2.setDrawColor(2); // XOR
    u8g2.setFontMode(1);  // Transparent font
    u8g2.setFontPosBaseline();

    String str = String(tankPercent);
    int strWidth = u8g2.getStrWidth(str.c_str());
    u8g2.drawStr(20 - (strWidth / 2), 40, str.c_str());
  }
}

void drawDisplayCallback()
{
  u8g2.clearBuffer();

  drawDisplayPercent();

  u8g2.setDrawColor(1);
  u8g2.setFontMode(0);
  u8g2.setFontPosTop();

  // Icons
  u8g2.setFont(u8g2_font_open_iconic_all_2x_t);
  if (currentState == on)
  {
    u8g2.drawGlyph(44, 2, 0x00d3); // Play icon
  }
  else if (currentState == off)
  {
    u8g2.drawGlyph(44, 2, 0x00d2); // Pause icon
  }
  else if (currentState == awaiting_reset)
  {
    u8g2.drawGlyph(44, 2, 0x00cd); // Reset icon
  }

  if (isConnected())
  {
    u8g2.drawGlyph(110, 2, 0x00f7); // WiFi icon
  }

  u8g2.drawGlyph(44, 29, 0x0098); // Humidity icon
  u8g2.drawGlyph(44, 47, 0x008D); // Temp icon

  u8g2.setFont(u8g2_font_helvB12_tf);

  if (currentState == awaiting_reset)
  {
    u8g2.drawStr(64, 4, getTankMode().c_str());
  }
  else
  {
    int hours = numberOfHours(tankRemainMillis / 1000);
    int minutes = numberOfMinutes(tankRemainMillis / 1000);
    int seconds = tankRemainMillis / 1000;

    char remainStr[8];
    if (seconds % 2 == 0)
    {
      sprintf(remainStr, "%02d:%02d", hours, minutes);
    }
    else
    {
      sprintf(remainStr, "%02d %02d", hours, minutes);
    }

    u8g2.drawStr(64, 4, remainStr); // Remaining time
  }

  char humStr[8];
  sprintf(humStr, "%.1f%%", humidity);
  u8g2.drawStr(64, 31, humStr); // Humidity

  char tempStr[8];
  sprintf(tempStr, "%.1f°C", temperature);
  u8g2.drawUTF8(64, 49, tempStr); // Temperature

  u8g2.sendBuffer();
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

  u8g2.begin();

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
    tankSessionTotalMillis = millis() - tankSessionStartedTime;
    unsigned long tempTankTotal = tankTotalMillis + tankSessionTotalMillis;

    double percent = (double(tempTankTotal) / double(getTankAverage()) * 100);
    tankPercent = max(0, (int)(100 - (round(percent))));

    tankRemainMillis = (getTankAverage() - tempTankTotal);
  }
}