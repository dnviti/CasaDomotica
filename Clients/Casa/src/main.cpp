/*
 Name:		ArduinoCasaDomotica2.ino
 Created:	5/26/2021 00:47:50 AM
 Author:	Daniele Viti
*/

/* #region Includes */
#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Keypad.h>
#include <Countimer.h>
#include <U8g2lib.h>
/* #endregion */

/* #region Defines */
#define LED_INT1_PIN D1
#define LED_EXT2_PIN D2
#define LED_INT3_PIN D3
#define LED_EXT4_PIN D4

#define DEBUGLN(A) Serial.println(A);
#define DEBUG(A) Serial.print(A);
#define DEBUG_ENABLED 1

#define MSG_BUFFER_SIZE (50)
/* #endregion */

/* #region Variables */

/* #region Timers */
Countimer tDisplay;
Countimer tReadings;
/* #endregion */

/* #region OLED Display */
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/SCL, /* data=*/SDA); // pin remapping with ESP8266 HW I2C

String displayTopLeft = "";
String displayTopRight = "";
String displayBottomLeft = "";
String displayBottomRight = "";

int displayState = 0;
/* #endregion */

/* #region Mqtt Variables */
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
char msg[MSG_BUFFER_SIZE];
long int value = 0;

const char *ssid = "WiFi-IoT";
const char *password = "M03qE8fQkmtOvslbkjexRHR0";
const char *mqtt_server = "192.168.10.103";

// Input - Output
const char *mqttInOutLed1 = "ard2/home1/led1";
const char *mqttInOutLed2 = "ard2/home1/led2";
const char *mqttInOutLed3 = "ard2/home1/led3";
const char *mqttInOutLed4 = "ard2/home1/led4";
const char *mqttInOutTimeOfDay = "ard2/home1/timeofday";

// Output
const char *mqttOutEarthquake = "ard2/home1/earthquake";

/* #endregion */

/* #endregion */

/* #region Utility */
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

int digitalCommonAnodeRead(uint8_t pin)
{
  int value = digitalRead(pin);
  if (value == LOW)
  {
    return HIGH;
  }
  else
  {
    return LOW;
  }
}

void digitalCommonAnodeWrite(uint8_t pin, uint8_t value)
{
  digitalWrite(pin, (value == HIGH ? LOW : HIGH));
}

void drawDisplay(char *topLeft, char *topRight, char *bottomLeft, char *bottomRight)
{
  // graphic commands to redraw the complete screen should be placed here
  u8g2.setFont(u8g_font_unifont);
  //u8g2.setFont(u8g_font_osb21);
  u8g2.drawStr(0, 11, topLeft);
  u8g2.drawStr(104, 11, topRight);
  u8g2.drawStr(0, 32, bottomLeft);
  u8g2.drawStr(80, 32, bottomRight);
}

void setup_wifi()
{
  displayTopLeft = "Avvio...";
  tDisplay.run();

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

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    debugLog("Attempting MQTT connection...", false);
    // Create a random client ID
    String clientId = "ESP8266Casa-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), "Casa", ""))
    {
      debugLog("connected");
      // ... and resubscribe
      client.subscribe(mqttInOutLed1);
      client.subscribe(mqttInOutLed2);
      client.subscribe(mqttInOutLed3);
      client.subscribe(mqttInOutLed4);
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

/* #region Generic Send Mqtt Value */
void sendMqttValue(const char *mqttTopic, char *val)
{
  //debugLog(String(val));
  client.publish(mqttTopic, val);
}
void sendMqttValue(const char *mqttTopic, float val)
{
  snprintf(msg, MSG_BUFFER_SIZE, "%.2f", val);
  sendMqttValue(mqttTopic, msg);
}
void sendMqttValue(const char *mqttTopic, double val)
{
  snprintf(msg, MSG_BUFFER_SIZE, "%.2f", val);
  sendMqttValue(mqttTopic, msg);
}
void sendMqttValue(const char *mqttTopic, int val)
{
  snprintf(msg, MSG_BUFFER_SIZE, "%d", val);
  sendMqttValue(mqttTopic, msg);
}
void sendMqttValue(const char *mqttTopic, String val)
{
  memset(msg, 0, sizeof msg);
  val.toCharArray(msg, 50);
  sendMqttValue(mqttTopic, msg);
}
/* #endregion */

/* #endregion */

/* #region Business Logic */
void lightLogic(char *mqttTopic, uint8_t value)
{
}

void displayReadings()
{
  switch (displayState)
  {
  case 0:
    break;
  case 1:
    break;
  case 2:
    break;
  case 3:
    break;
  case 4:
    break;
  default:
    break;
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

  //toggleLed(String((char)payload[0]).toInt());
}
/* #endregion */

/* #region Timers Definitions */
void tDisplayComplete() {}
void tDisplayInterval()
{
  char displayTopLeftBuf[50];
  char displayTopRightBuf[50];
  char displayBottomLeftBuf[50];
  char displayBottomRightBuf[50];

  memset(displayTopLeftBuf, 0, sizeof displayTopLeftBuf);
  memset(displayTopRightBuf, 0, sizeof displayTopRightBuf);
  memset(displayBottomLeftBuf, 0, sizeof displayBottomLeftBuf);
  memset(displayBottomRightBuf, 0, sizeof displayBottomRightBuf);

  displayTopLeft.toCharArray(displayTopLeftBuf, 50);
  displayTopRight.toCharArray(displayTopRightBuf, 50);
  displayBottomLeft.toCharArray(displayBottomLeftBuf, 50);
  displayBottomRight.toCharArray(displayBottomRightBuf, 50);

  u8g2.firstPage();
  do
  {
    drawDisplay(displayTopLeftBuf, displayTopRightBuf, displayBottomLeftBuf, displayBottomRightBuf);
  } while (u8g2.nextPage());
}

void tReadingsComplete() {}
void tReadingsInterval()
{
}
/* #endregion */

void setup()
{
  /* #region Setup Display */
  u8g2.begin();
  /* #endregion */

  /* #region Setup Timers */
  tDisplay.setCounter(24, 0, 0, tDisplay.COUNT_DOWN, tDisplayComplete);
  tDisplay.setInterval(tDisplayInterval, 1);
  tDisplay.start();

  tReadings.setCounter(24, 0, 0, tReadings.COUNT_DOWN, tReadingsComplete);
  tReadings.setInterval(tReadingsInterval, 1000);
  tReadings.start();
  /* #endregion */

  /* #region Setup Pins */
  pinMode(LED_INT1_PIN, OUTPUT);
  pinMode(LED_EXT2_PIN, OUTPUT);
  pinMode(LED_INT3_PIN, OUTPUT);
  pinMode(LED_EXT4_PIN, OUTPUT);
  /* #endregion */

  /* #region Setup MQTT */
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if (client.connect("ard2"))
  {
    client.subscribe(mqttInOutLed1);
    client.subscribe(mqttInOutLed2);
    client.subscribe(mqttInOutLed3);
    client.subscribe(mqttInOutLed4);
    client.subscribe(mqttInOutTimeOfDay);
  }
  /* #endregion */

  /* #region Value Presets */
  // Setto il valore del pin del led per condividerlo con tutti all'avvio
  int led1Val = digitalCommonAnodeRead(LED_INT1_PIN);
  debugLog("Initial Led Value: " + String(led1Val));
  snprintf(msg, MSG_BUFFER_SIZE, "%d", led1Val);
  client.publish(mqttInOutLed1, msg);

  int led2Val = digitalCommonAnodeRead(LED_EXT2_PIN);
  debugLog("Initial Led Value: " + String(led2Val));
  snprintf(msg, MSG_BUFFER_SIZE, "%d", led2Val);
  client.publish(mqttInOutLed2, msg);

  int led3Val = digitalCommonAnodeRead(LED_INT3_PIN);
  debugLog("Initial Led Value: " + String(led3Val));
  snprintf(msg, MSG_BUFFER_SIZE, "%d", led3Val);
  client.publish(mqttInOutLed3, msg);

  int led4Val = digitalCommonAnodeRead(LED_EXT4_PIN);
  debugLog("Initial Led Value: " + String(led4Val));
  snprintf(msg, MSG_BUFFER_SIZE, "%d", led4Val);
  client.publish(mqttInOutLed4, msg);
  /* #endregion */

  Serial.begin(115200);
}

void loop()
{
  digitalCommonAnodeWrite(LED_INT1_PIN, HIGH);
  digitalCommonAnodeWrite(LED_EXT2_PIN, HIGH);
  digitalCommonAnodeWrite(LED_INT3_PIN, HIGH);
  digitalCommonAnodeWrite(LED_EXT4_PIN, HIGH);
}