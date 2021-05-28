/*
 Name:		ArduinoCasaDomotica
 Created:	5/26/2021 00:47:50 AM
 Author:	Daniele Viti
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <Countimer.h>
#include <U8g2lib.h>
#include <Key.h>
#include <Keypad.h>
#include <Keypad_I2C.h>

#define LED_INT1_PIN D5
#define LED_INT3_PIN D7
#define LED_EXT2_PIN D0
#define LED_EXT4_PIN D6
#define PHOTORESISTOR_PIN A0
#define BUZZER_PIN D8

#define DEBUGLN(A) Serial.println(A);
#define DEBUG(A) Serial.print(A);
#define DEBUG_ENABLED 0

#define MSG_BUFFER_SIZE (50)

#define I2CADDR 0x20 // Set the Address of the PCF8574

Countimer tDisplay;
Countimer tReadings;
Countimer tMqttUpdater;

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/SCL, /* data=*/SDA); // pin remapping with ESP8266 HW I2C

String displayTopLeft = "";
String displayTopRight = "";
String displayBottomLeft = "";
String displayBottomRight = "";

char lastSelectedOption = '*';

uint8_t led1LastState;
uint8_t led2LastState;
uint8_t led3LastState;
uint8_t led4LastState;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
char msg[MSG_BUFFER_SIZE];
long int value = 0;

int lightLimit = 14;

bool configActive = false;

const char *ssid = "WiFi-IoT";
const char *password = "M03qE8fQkmtOvslbkjexRHR0";
const char *mqtt_server = "192.168.10.103";

// Input - Output
const char *mqttInOutLed1 = "casa/led1";
const char *mqttInOutLed2 = "casa/led2";
const char *mqttInOutLed3 = "casa/led3";
const char *mqttInOutLed4 = "casa/led4";
const char *mqttInOutLightLimit = "casa/lightlimit";

// Output
const char *mqttOutMqttUpdater = "casa/earthquake";

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};
byte rowPins[ROWS] = {0, 1, 2, 3}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {4, 5, 6};    //connect to the column pinouts of the keypad
Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR, PCF8574);

/* #region Helpers */
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
  u8g2.drawStr(95, 11, topRight);
  u8g2.drawStr(0, 30, bottomLeft);
  u8g2.drawStr(100, 30, bottomRight);
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

/* #region Business Logic */
void mqttLogicIn(String mqttTopic, String value)
{
  int iValue = value.toInt();
  uint8_t ledState = iValue > 0 ? HIGH : LOW;
  // Garage
  if (mqttTopic == String(mqttInOutLed1))
  {
    led1LastState = ledState;
    digitalCommonAnodeWrite(LED_INT1_PIN, ledState);
  }
  // Serranda
  if (mqttTopic == String(mqttInOutLed2))
  {
    led2LastState = ledState;
    digitalCommonAnodeWrite(LED_EXT2_PIN, ledState);
  }
  if (mqttTopic == String(mqttInOutLed3))
  {
    led3LastState = ledState;
    digitalCommonAnodeWrite(LED_INT3_PIN, ledState);
  }
  if (mqttTopic == String(mqttInOutLed4))
  {
    led4LastState = ledState;
    digitalCommonAnodeWrite(LED_EXT4_PIN, ledState);
  }
  if (mqttTopic == String(mqttInOutLightLimit))
  {
    lightLimit = value.toInt();
  }
}

void updateCrepuscolareLedMqtt()
{
  sendMqttValue(mqttInOutLed2, led2LastState);
  sendMqttValue(mqttInOutLed4, led4LastState);
}

void displayReadings()
{
}

void callback(char *topic, byte *payload, unsigned int length)
{
  debugLog("Message arrived [", false);
  debugLog(topic, false);
  debugLog("] ", false);

  char cPayload[length];

  for (uint i = 0; i < length; i++)
  {
    debugLog(String(payload[i]), false);
    cPayload[i] = (char)payload[i];
  }
  debugLog();

  String sTopic = String(topic);

  String sPayload = String(cPayload);

  mqttLogicIn(String(topic), sPayload);
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
      client.subscribe(mqttInOutLightLimit);
      updateCrepuscolareLedMqtt();
      sendMqttValue(mqttInOutLightLimit, lightLimit);
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
/* #endregion */

/* #region Timers */
void tDisplayComplete()
{
}
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

void tMqttUpdaterComplete() {}
void tMqttUpdateInterval()
{
  updateCrepuscolareLedMqtt();
}
/* #endregion */

void setup()
{
  Serial.begin(115200);
  debugLog("Starting Pin Extender");
  Wire.begin();                   // Call the connection Wire
  keypad.begin(makeKeymap(keys)); // Call the connection

  debugLog("Starting Display");
  u8g2.begin();

  debugLog("Starting Timers");
  tDisplay.setCounter(24, 0, 0, tDisplay.COUNT_DOWN, tDisplayComplete);
  tDisplay.setInterval(tDisplayInterval, 1);
  tDisplay.start();

  tReadings.setCounter(24, 0, 0, tReadings.COUNT_DOWN, tReadingsComplete);
  tReadings.setInterval(tReadingsInterval, 1000);
  tReadings.start();

  tMqttUpdater.setCounter(24, 0, 0, tMqttUpdater.COUNT_DOWN, tMqttUpdaterComplete);
  tMqttUpdater.setInterval(tMqttUpdateInterval, 1000);
  tMqttUpdater.start();

  debugLog("Setup Pinout");
  pinMode(LED_INT1_PIN, OUTPUT);
  pinMode(LED_EXT2_PIN, OUTPUT);
  pinMode(LED_INT3_PIN, OUTPUT);
  pinMode(LED_EXT4_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PHOTORESISTOR_PIN, INPUT);

  digitalCommonAnodeWrite(LED_INT1_PIN, LOW);
  digitalCommonAnodeWrite(LED_INT3_PIN, LOW);
  digitalCommonAnodeWrite(LED_EXT2_PIN, LOW);
  digitalCommonAnodeWrite(LED_EXT4_PIN, LOW);

  led1LastState = LOW;
  led2LastState = LOW;
  led3LastState = LOW;
  led4LastState = LOW;

  debugLog("Setup MQTT");
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if (client.connect("casa"))
  {
    client.subscribe(mqttInOutLed1);
    client.subscribe(mqttInOutLed2);
    client.subscribe(mqttInOutLed3);
    client.subscribe(mqttInOutLed4);
    client.subscribe(mqttInOutLightLimit);
    updateCrepuscolareLedMqtt();
    sendMqttValue(mqttInOutLightLimit, lightLimit);
  }
}

void loop()
{
  char key = keypad.getKey();
  if (key)
  {
    if (lastSelectedOption == '*' && key == '*' && configActive == false)
    {
      displayTopRight = "CONF";
      configActive = true;
      displayBottomLeft = "Soglia:";
      displayBottomRight = "";
    }

    if (key == '*' || key == '#')
    {
      lastSelectedOption = key;
    }

    digitalWrite(BUZZER_PIN, HIGH);
    delay(10);
    digitalWrite(BUZZER_PIN, LOW);
    debugLog("Selected Option: " + String(key));
  }

  if (configActive)
  {
    if (key != '*' && key != '#')
    {
      displayBottomRight += String(key);
    }

    if (key == '#')
    {
      // Salvo
      int newLimit = displayBottomRight.toInt();
      if (newLimit != 0)
      {
        lightLimit = newLimit;

        if (newLimit >= 50)
        {
          lightLimit = 50;
        }
      }

      sendMqttValue(mqttInOutLightLimit, lightLimit);
      configActive = false;
      lastSelectedOption = '*'; // torno all'automatico
    }
  }
  else
  {
    switch (key)
    {
    case '1':
      if (led1LastState == HIGH)
      {
        led1LastState = LOW;
        digitalCommonAnodeWrite(LED_INT1_PIN, led1LastState);
        updateCrepuscolareLedMqtt();
      }
      else
      {
        led1LastState = HIGH;
        digitalCommonAnodeWrite(LED_INT1_PIN, led1LastState);
        updateCrepuscolareLedMqtt();
      }
      break;
    case '2':
      if (led2LastState == HIGH)
      {
        led2LastState = LOW;
        digitalCommonAnodeWrite(LED_EXT2_PIN, led2LastState);
        updateCrepuscolareLedMqtt();
      }
      else
      {
        led2LastState = HIGH;
        digitalCommonAnodeWrite(LED_EXT2_PIN, led2LastState);
        updateCrepuscolareLedMqtt();
      }
      break;
    case '3':
      if (led3LastState == HIGH)
      {
        led3LastState = LOW;
        digitalCommonAnodeWrite(LED_INT3_PIN, led3LastState);
        updateCrepuscolareLedMqtt();
      }
      else
      {
        led3LastState = HIGH;
        digitalCommonAnodeWrite(LED_INT3_PIN, led3LastState);
        updateCrepuscolareLedMqtt();
      }
      break;
    case '4':
      if (led4LastState == HIGH)
      {
        led4LastState = LOW;
        digitalCommonAnodeWrite(LED_EXT4_PIN, led4LastState);
        updateCrepuscolareLedMqtt();
      }
      else
      {
        led4LastState = HIGH;
        digitalCommonAnodeWrite(LED_EXT4_PIN, led4LastState);
        updateCrepuscolareLedMqtt();
      }
      break;
    case '0':
      led1LastState = LOW;
      led2LastState = LOW;
      led3LastState = LOW;
      led4LastState = LOW;
      digitalCommonAnodeWrite(LED_INT1_PIN, led1LastState);
      digitalCommonAnodeWrite(LED_EXT2_PIN, led2LastState);
      digitalCommonAnodeWrite(LED_INT3_PIN, led3LastState);
      digitalCommonAnodeWrite(LED_EXT4_PIN, led4LastState);
      updateCrepuscolareLedMqtt();
      break;
    default:
      break;
    }

    // Light modes controlled by keypad
    // It starts with photoresistors!
    switch (lastSelectedOption)
    {
    case '*': // automatic external light on/off based on photoresistors value
      displayTopRight = "AUTO";
      displayBottomLeft = "Soglia Luce:";
      displayBottomRight = String(lightLimit);
      if (analogRead(PHOTORESISTOR_PIN) <= lightLimit)
      {
        led2LastState = HIGH;
        led4LastState = HIGH;
        digitalCommonAnodeWrite(LED_EXT2_PIN, led2LastState);
        digitalCommonAnodeWrite(LED_EXT4_PIN, led4LastState);
      }
      else
      {
        led2LastState = LOW;
        led4LastState = LOW;
        digitalCommonAnodeWrite(LED_EXT2_PIN, led2LastState);
        digitalCommonAnodeWrite(LED_EXT4_PIN, led4LastState);
      }
      tMqttUpdater.run();
      break;
    case '#': // disabled automatic external light and enable mqtt control
      displayTopRight = "MANU";
      displayBottomRight = "";
      displayBottomLeft = "";
      digitalCommonAnodeWrite(LED_INT1_PIN, led1LastState);
      digitalCommonAnodeWrite(LED_EXT2_PIN, led2LastState);
      digitalCommonAnodeWrite(LED_INT3_PIN, led3LastState);
      digitalCommonAnodeWrite(LED_EXT4_PIN, led4LastState);
      tMqttUpdater.run();
      break;
    default:
      tMqttUpdater.stop();
      break;
    }
  }

  if (!client.connected())
  {
    tReadings.stop();
    tMqttUpdater.stop();
    displayTopLeft = "Offline";
    displayBottomRight = "";
    displayBottomLeft = "";

    reconnect();
  }
  else
  {
    tReadings.start();
    // Input - Output

    // Display
    displayTopLeft = "Online";
    displayReadings();
  }
  client.loop();

  tDisplay.run();
  tReadings.run();
  tMqttUpdater.run();
}