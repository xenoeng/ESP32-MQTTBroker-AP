/*
Base code for an ESP32 MQTT broker avaiable via its own AP.
No RPi-Mosquitto broker, cloud broker or wifi/router needed.
    - Broker runs on core 0, local client and main code on core 1 (default).
    - tested on phone with MQTT Analyzer and on PC with MQTT Explorer (simultaneously).
    - tested with two ESP8266 (D1 Mini) clients connected to D1 Mini ESP32 version as host-broker.
*/
#include "Arduino.h"
#include <sMQTTBroker.h>  // https://github.com/terrorsl/sMQTTBroker
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient

#define LED (2)
#define mqttPort 1883
#define mqttServer "127.0.0.1"

sMQTTBroker broker;
WiFiClient espClient;
PubSubClient MQTTclient(espClient);

unsigned long lastMsg = 0;
unsigned long now;
unsigned long interval = 1000; // default interval at which to update
unsigned long freeRam;
char testcount[5];
unsigned int testCount;
char HB[2];
bool HBflag = 0;

/////////////////////////////////////////////////////////////////////////////////////////
TaskHandle_t brokerTask;

void brokerTaskCode(void *pvParameters)
{
    Serial.print("broker running on core ");
    Serial.println(xPortGetCoreID());

    for (;;)
    {
        delay(1000);
        broker.update();
    }
}
/////////////////////////////////////////////////////////////////////////////////////////
void reconnect()
{
    /* Loop until (re)connected */
    while (!MQTTclient.connected())
    {
        Serial.print("Attempting MQTT connection...");

        /* Attempt to connect */
        if (MQTTclient.connect("ESP32"))
        {
            Serial.println("connected");
            /* Once connected, publish an announcement... */
            MQTTclient.publish("system", "connected");
            /* ... and resubscribe */
            MQTTclient.subscribe("system/HB");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(MQTTclient.state());
            Serial.println(" will try again in 5secs");
            delay(5000); // Wait 5 seconds before retrying
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////
void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    if (strcmp(topic, "system/HB") == 0) // matching topic?
    {
        if ((char)payload[0] == '1') // if an 1 was received as first character
        {
            digitalWrite(LED, HIGH); // LED on
        }
        else
        {
            digitalWrite(LED, LOW); // LED off
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
    pinMode(LED, OUTPUT);
    Serial.begin(115200);

    /*  create task for broker that will be executed on core 0  */
    xTaskCreatePinnedToCore(
        brokerTaskCode, /* Task function. */
        "brokerTask",   /* name of task. */
        10000,          /* Stack size of task */
        NULL,           /* parameter of the task */
        1,              /* priority of the task */
        &brokerTask,    /* Task handle to keep track of created task */
        0);             /* pin task to core 0 */
    delay(500);

    const char *ssid = "ESP32";        // SSID of access point
    const char *password = "12345678"; // password to connect to AP

    WiFi.softAP(ssid, password);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);

    broker.init(mqttPort);
    delay(1500);
    
    MQTTclient.setServer(mqttServer, mqttPort);
    MQTTclient.setCallback(callback);
}
/////////////////////////////////////////////////////////////////////////////////////////
void loop()
{
    if (!MQTTclient.connected())
    {
        reconnect();
    }
    MQTTclient.loop();

    /* update and publish */
    now = millis();
    if (now - lastMsg > interval) // time between updates
    {
        lastMsg = now;

        /* heartbeat message and LED - if clients subcribe and switch their own built-in
        LEDs to mirror the state of the topic it makes a useful indicator of connectivity */
        HBflag = !HBflag;
        itoa(HBflag, HB, 10);
        broker.publish("system/HB", HB);

        /* example counter */
        itoa(testCount++, testcount, 10);
        broker.publish("test", testcount);

        /* for information */
        if (ESP.getFreeHeap() != freeRam)
        {
            freeRam = ESP.getFreeHeap();
            Serial.print("RAM:");
            Serial.println(freeRam);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////