#include <Arduino.h>
#include <Adafruit_Si7021.h>
#include <Wire.h>
#include <net.h>
#include <main.h>
#include <TaskSchedulerDeclarations.h>

#include <pins.h>

Adafruit_Si7021 sensorSi7021 = Adafruit_Si7021();

void refreshHumidityCallback();

// Every minute
Task refreshHumidity(TASK_MINUTE, TASK_FOREVER, &refreshHumidityCallback);

void refreshHumidityCallback()
{
    float humidity = sensorSi7021.readHumidity();
    float temperature = sensorSi7021.readTemperature();

    mqttClient.publish(topic_humidity, String(humidity));
    mqttClient.publish(topic_temperature, String(temperature));

    Serial.print("Humidity: ");
    Serial.print(humidity, 2);
    Serial.print("% \tTemperature: ");
    Serial.print(temperature, 2);
    Serial.println(" C");
}

void setupSi7021()
{
    if (!sensorSi7021.begin(PIN_SDA, PIN_SCL))
    {
        Serial.println("Did not find Si7021 sensor!");
        while (true)
            ;
    }

    Serial.print("Found Si7021 model ");
    switch (sensorSi7021.getModel())
    {
    case SI_Engineering_Samples:
        Serial.print("SI engineering samples");
        break;
    case SI_7013:
        Serial.print("Si7013");
        break;
    case SI_7020:
        Serial.print("Si7020");
        break;
    case SI_7021:
        Serial.print("Si7021");
        break;
    case SI_UNKNOWN:
    default:
        Serial.print("Unknown");
    }
    Serial.print(" Rev(");
    Serial.print(sensorSi7021.getRevision());
    Serial.print(")");
    Serial.print(" Serial #");
    Serial.print(sensorSi7021.sernum_a, HEX);
    Serial.println(sensorSi7021.sernum_b, HEX);
}

void setupSensors()
{
    setupSi7021();

    runner.addTask(refreshHumidity);
    refreshHumidity.enable();
}
