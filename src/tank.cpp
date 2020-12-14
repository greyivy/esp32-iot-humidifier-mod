#include <Arduino.h>
#include <Preferences.h>
#include <pins.h>
#include <main.h>

Preferences preferences;

unsigned long cachedTankAverage = 0;

#define KEY_TANK_MODE_LOW "Low"
#define KEY_TANK_MODE_MED "Med"
#define KEY_TANK_MODE_HIGH "High"

String KEY_PROGRESSION[3] = {KEY_TANK_MODE_LOW, KEY_TANK_MODE_MED, KEY_TANK_MODE_HIGH};
int currentTankMode = 1; // Med

// med 102000s default

void toggleTankMode()
{
    // TODO publish
    if (currentTankMode == 2)
    {
        currentTankMode = 0;
    }
    else
    {
        currentTankMode++;
    }

    cachedTankAverage = 0;
}

String getTankMode()
{
    return KEY_PROGRESSION[currentTankMode];
}

const String getKey(String key)
{
    return getTankMode() + key;
}

void setupTank()
{
    preferences.begin("humidifier", false);
    // TODO call preferences.end()?
}

unsigned long getTankAverage()
{
    if (cachedTankAverage == 0)
    {
        cachedTankAverage = preferences.getULong(getKey("avg").c_str(), TASK_HOUR * 26); // TODO determine values for first calibration
        Serial.print("Tank average: ");
        Serial.println(cachedTankAverage / 1000);
    }

    return cachedTankAverage;
}

void setTankAverage(unsigned long value)
{
    cachedTankAverage = value;
    preferences.putULong(getKey("avg").c_str(), value);
}

void addToTankAverage(unsigned long tankTotalMillis)
{
    if (tankTotalMillis < TASK_HOUR) return;

    unsigned long previousAverage = getTankAverage();
    int count = preferences.getInt(getKey("cnt").c_str(), 0);

    if (count == 0)
    {
        setTankAverage(tankTotalMillis);
        preferences.putInt(getKey("cnt").c_str(), 1);
    }
    else
    {
        unsigned long newAverage = ((previousAverage * count) + tankTotalMillis) / (count + 1);
        setTankAverage(newAverage);
        preferences.putInt(getKey("cnt").c_str(), count + 1);

        Serial.print("Old average: ");
        Serial.println(previousAverage);

        Serial.print("Added value to average: ");
        Serial.println(tankTotalMillis);
    }

    Serial.print("New average: ");
    Serial.println(getTankAverage());
}

void clearAverage()
{
    preferences.clear();
    Serial.println("Clear average");
}

int previousLdrSensorValue = 0;
bool isTankEmpty()
{
    int value = analogRead(PIN_LDR);

    if (value != previousLdrSensorValue)
    {
        previousLdrSensorValue = value;
    }

    return value > 300; // LED on
}