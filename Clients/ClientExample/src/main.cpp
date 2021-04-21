/*
 Name:		ArduinoMqttClient.ino
 Created:	2/8/2021 7:22:50 PM
 Author:	Daniele Viti
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

// Update these with values suitable for your network.

const char *ssid = "WiFi-IoT";
const char *password = "M03qE8fQkmtOvslbkjexRHR0";
const char *mqtt_server = "192.168.10.101";

#define DEBUGLN(A) Serial.println(A);
#define DEBUG(A) Serial.print(A);
#define DEBUG_ENABLED 0

#define EOLIC_VOLTAGE_PIN A0

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_BMP280 bmp;

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
long int value = 0;

// Topics

// Input
const char *mqttInTopic = "ard1/home1/led";

// Output
const char *mqttOutEolicVoltage = "ard1/home1/veolico";
const char *mqttOutEolicAmperage = "ard1/home1/aeolico";
const char *mqttOutTemperature = "ard1/home1/temp";
const char *mqttOutPressure = "ard1/home1/press";
const char *mqttOutAltitude = "ard1/home1/alt";

void debugLog(String s = "", bool newLine = true)
{
  if (DEBUG_ENABLED == 1)
  {
    if (newLine)
    {
      DEBUGLN(s);
    }
    else
    {
      DEBUG(s);
    }
  }
}

// double mapf(double val, double in_min, double in_max, double out_min, double out_max)
// {
//   return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
// }

double getEolicoVoltage()
{
  double res = analogRead(EOLIC_VOLTAGE_PIN) * (5.0 / 1023.0);
  return res;
}

double getEolicoAmpere()
{
  double res = analogRead(EOLIC_VOLTAGE_PIN) * (205.0 / 1023.0);
  return res;
}

void setup_wifi()
{

  delay(10);
  // We start by connecting to a WiFi network
  debugLog();
  debugLog("Connecting to ", false);
  debugLog(String(ssid));

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    debugLog(".", false);
  }

  randomSeed(micros());

  debugLog();
  debugLog("WiFi connected");
  debugLog("IP address: ");
  debugLog(WiFi.localIP().toString());
}

void sendEolicoVoltage()
{
  double p = getEolicoVoltage();

  debugLog(String(p));
  snprintf(msg, MSG_BUFFER_SIZE, "%f", p);
  client.publish(mqttOutEolicVoltage, msg);
}

void sendEolicoEfficiency()
{
  double p = getEolicoAmpere();

  debugLog(String(p));
  snprintf(msg, MSG_BUFFER_SIZE, "%f", p);
  client.publish(mqttOutEolicAmperage, msg);
}

void sendTemperature()
{
  float p = bmp.readTemperature();

  debugLog(String(p));
  snprintf(msg, MSG_BUFFER_SIZE, "%f", p);
  client.publish(mqttOutTemperature, msg);
}

void sendPressure()
{
  float p = bmp.readPressure();
  // Convert Pa to atm
  float atm = p / 101325;
  debugLog(String(p));
  snprintf(msg, MSG_BUFFER_SIZE, "%f", atm);
  client.publish(mqttOutPressure, msg);
}

void sendAltitude()
{
  float p = bmp.readAltitude(1019); // 1019 Umbria

  debugLog(String(p));
  snprintf(msg, MSG_BUFFER_SIZE, "%f", p);
  client.publish(mqttOutAltitude, msg);
}

void accendiLed(char ledValue)
{
  // Switch on the LED if an 1 was received as first character
  if (ledValue == '1')
  {
    debugLog("LED ON");
    digitalWrite(D4, LOW); // Turn the LED on (Note that LOW is the voltage level
                           // but actually the LED is on; this is because
                           // it is active low on the ESP-01)
  }
  else
  {
    debugLog("LED OFF");
    digitalWrite(D4, HIGH); // Turn the LED off by making the voltage HIGH
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  debugLog("Message arrived [", false);
  debugLog(topic, false);
  debugLog("] ", false);

  for (uint i = 0; i < length; i++)
  {
    debugLog(String(payload[i]), false);
  }
  debugLog();

  accendiLed((char)payload[0]);
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    debugLog("Attempting MQTT connection...", false);
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), "Hans", "Test"))
    {
      debugLog("connected");
      // ... and resubscribe
      client.subscribe(mqttInTopic);
    }
    else
    {
      debugLog("failed, rc=", false);
      debugLog(String(client.state()), false);
      debugLog(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  }
}

void setupBmp280()
{
  //if (!bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID)) {
  if (!bmp.begin(0x076))
  {
    debugLog(F("Could not find a valid BMP280 sensor, check wiring or "
               "try a different address!"));
    while (1)
      delay(10);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
}

void setup()
{
  pinMode(D4, OUTPUT); // Initialize the 4 pin as an output
  pinMode(D5, INPUT);
  pinMode(A0, INPUT); // Legge il voltaggio e l'amperaggio de
  Serial.begin(115200);
  setup_wifi();
  setupBmp280();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if (client.connect("ard1"))
  {
    client.subscribe(mqttInTopic);
  }

  // Setto il valore del pin del led per condividerlo con tutti all'avvio
  int ledVal = digitalRead(D5);
  snprintf(msg, MSG_BUFFER_SIZE, "%d", ledVal);
  client.publish(mqttInTopic, msg);
}

void loop()
{

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 100)
  {
    lastMsg = now;
    sendEolicoVoltage();
    sendEolicoEfficiency();

    sendTemperature();
    sendPressure();
    sendAltitude();
  }
}