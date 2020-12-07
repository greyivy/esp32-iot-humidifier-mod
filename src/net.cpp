#include <WiFi.h>
#include <MQTT.h>
#include <main.h>

#include <actions.h>

#include <tank.h>

MQTTClient mqttClient;
WiFiClient wifi;

const char *ssid = "Pepperomia Pepperspot 2.4 GHz";
const char *password = "yrdZx6SihxGhoR&m";
const char *mqtt_server = "192.168.1.10";
const char *mqtt_user = "mqtt";
const char *mqtt_password = "puF9yAeHgRWhkhJa";

char chipId[21];

String topic_power;
String topic_power_set;
String topic_empty;
String topic_tank;
String topic_humidity;
String topic_temperature;
String topic_total;
String topic_average;
String topic_debug;
String topic_percent;

void messageReceived(String &topic, String &payload)
{
    Serial.println("MQTT: " + topic + " - " + payload);

    if (topic.equals(topic_power_set))
    {
        if (payload.equalsIgnoreCase("on"))
        {
            fsm.trigger(ACTION_ON);
        }
        else if (payload.equalsIgnoreCase("off"))
        {
            fsm.trigger(ACTION_OFF);
        }
    } else if (topic.equals(topic_debug)) {
        if (payload.equalsIgnoreCase("empty"))
        {
            fsm.trigger(ACTION_EMPTY);
        }
        if (payload.equalsIgnoreCase("clear_average"))
        {
            clearAverage();
        }
    }

    // Note: Do not use the client in the callback to publish, subscribe or
    // unsubscribe as it may cause deadlocks when other things arrive while
    // sending and receiving acknowledgments. Instead, change a global variable,
    // or push to a queue and handle it in the loop after calling `client.loop()`.
}

void connectMQTT()
{
    Serial.print("\nConnecting to MQTT");
    mqttClient.begin(mqtt_server, wifi);
    mqttClient.onMessage(messageReceived);
    while (!mqttClient.connect(chipId, mqtt_user, mqtt_password))
    {
        delay(100);
        Serial.print(".");
    }

    Serial.println("\nConnected");

    mqttClient.subscribe(topic_power_set);
    mqttClient.subscribe(topic_debug);
}

void setupNet()
{
    // Generate topics
    sprintf(chipId, "%" PRIu64, ESP.getEfuseMac());
    String topicPrefix = "humidifiers/" + String(chipId);
    topic_power = topicPrefix + "/power";
    topic_power_set = topicPrefix + "/power/set";
    topic_empty = topicPrefix + "/empty";
    topic_tank = topicPrefix + "/tank";
    topic_humidity = topicPrefix + "/humidity";
    topic_temperature = topicPrefix + "/temperature";
    topic_total = topicPrefix + "/total";
    topic_average = topicPrefix + "/average";
    topic_debug = topicPrefix + "/debug";
    topic_percent = topicPrefix + "/percent";

    Serial.println("Chip ID: ");
    Serial.println(chipId);

    Serial.print("\nConnecting to WiFi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
        Serial.print(".");
    }

    Serial.println("\nIP address: ");
    Serial.println(WiFi.localIP());

    connectMQTT();
}

void loopNet()
{
    mqttClient.loop();

    if (!mqttClient.connected())
    {
        connectMQTT();
    }
}