/*
 Name:		ArduinoMqttClient.ino
 Created:	2/8/2021 7:22:50 PM
 Author:	Daniele Viti
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char *ssid = "WiFi-IoT";
const char *password = "M03qE8fQkmtOvslbkjexRHR0";
const char *mqtt_server = "192.168.10.221";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
long int value = 0;

const char *mqttOutTopic = "ard1/home1/bmp280";
const char *mqttInTopic = "ard1/home1/led";

void setup_wifi()
{

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (uint i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1')
  {
    Serial.print("LED ON");
    digitalWrite(4, LOW); // Turn the LED on (Note that LOW is the voltage level
                          // but actually the LED is on; this is because
                          // it is active low on the ESP-01)
  }
  else
  {
    Serial.print("LED OFF");
    digitalWrite(4, HIGH); // Turn the LED off by making the voltage HIGH
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), "Hans", "Test"))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(mqttOutTopic, "hello world");
      // ... and resubscribe
      client.subscribe(mqttInTopic);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  pinMode(13, OUTPUT); // Initialize the 13 pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if (client.connect("ard1"))
  {
    client.publish(mqttOutTopic, "hello world");
    client.subscribe(mqttInTopic);
  }
}

void loop()
{

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 5000)
  {
    lastMsg = now;
    ++value;
    snprintf(msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(mqttOutTopic, msg);
  }
}