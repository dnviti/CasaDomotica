/*
 Name:		ArduinoCampoSolare
 Created:	2/8/2021 7:22:50 PM
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

// Update these with values suitable for your network.

const char *ssid = "WiFi-IoT";
const char *password = "M03qE8fQkmtOvslbkjexRHR0";
const char *mqtt_server = "192.168.10.103";

#define DEBUGLN(A) Serial.println(A);
#define DEBUG(A) Serial.print(A);
#define DEBUG_ENABLED 0

#define LED_OUTPUT_PIN D4
#define LED_INPUT_PIN D5
#define BUTTON_PIN D6
#define VOLTMETER_PIN A0

Countimer tDisplay;
Countimer tReadings;
Countimer tButton;

String displayTopLeft = "";
String displayTopRight = "";
String displayBottomLeft = "";
String displayBottomRight = "";

double eolicoVoltage;
double eolicoEfficiency;

float temperature;
float pressure;
float altitude;

int displayState = 0;
int lastButtonState;    // the previous state of button
int currentButtonState; // the current state of button

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_BMP280 bmp;

// display
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/SCL, /* data=*/SDA); // pin remapping with ESP8266 HW I2C

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
long int value = 0;

// Topics

// Input
const char *mqttInLed = "campo/led";

// Output
const char *mqttOutVoltage = "campo/volt";
const char *mqttOutAmperage = "campo/ampere";
const char *mqttOutTemperature = "campo/temp";
const char *mqttOutPressure = "campo/press";
const char *mqttOutAltitude = "campo/alt";

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

// double mapf(double val, double in_min, double in_max, double out_min, double out_max)
// {
//   return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
// }

double getEolicoVoltage()
{
  double res = analogRead(VOLTMETER_PIN) * (5.0 / 1023.0);
  return res;
}

double getEolicoAmpere()
{
  double res = analogRead(VOLTMETER_PIN) * (205.0 / 1023.0);
  return res;
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

void toggleLed(uint8_t ledValue)
{
  // Switch on the LED if an 1 was received as first character
  digitalCommonAnodeWrite(LED_OUTPUT_PIN, ledValue);
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

  toggleLed(String((char)payload[0]).toInt());
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    debugLog("Attempting MQTT connection...", false);
    // Create a random client ID
    String clientId = "ESP8266Campo-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), "Campo", ""))
    {
      debugLog("connected");
      // ... and resubscribe
      client.subscribe(mqttInLed);
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
  eolicoVoltage = getEolicoVoltage();
  eolicoEfficiency = getEolicoAmpere();

  temperature = bmp.readTemperature();
  pressure = bmp.readPressure() / 101325;
  altitude = bmp.readAltitude(1019);

  sendMqttValue(mqttOutVoltage, eolicoVoltage);
  sendMqttValue(mqttOutAmperage, eolicoEfficiency);

  sendMqttValue(mqttOutTemperature, temperature);
  sendMqttValue(mqttOutPressure, pressure);
  sendMqttValue(mqttOutAltitude, altitude);
}

void tButtonComplete() {}
void tButtonInterval()
{
  currentButtonState = digitalRead(BUTTON_PIN);
  if (currentButtonState == HIGH && lastButtonState == LOW)
  {
    lastButtonState = digitalRead(BUTTON_PIN);
    debugLog(String(currentButtonState));

    if (displayState == 4)
      displayState = 0;
    else
      displayState += 1;
  }
  else
  {
    lastButtonState = digitalRead(BUTTON_PIN);
  }
}

void displayReadings()
{
  switch (displayState)
  {
  case 0:
    displayBottomRight = "V";
    displayBottomLeft = eolicoVoltage;
    break;
  case 1:
    displayBottomRight = "mA";
    displayBottomLeft = eolicoEfficiency;
    break;
  case 2:
    displayBottomRight = "C";
    displayBottomLeft = temperature;
    break;
  case 3:
    displayBottomRight = "atm";
    displayBottomLeft = pressure;
    break;
  case 4:
    displayBottomRight = "m";
    displayBottomLeft = altitude;
    break;
  default:
    break;
  }
}

void setup()
{

  u8g2.begin();

  tButton.setCounter(24, 0, 0, tButton.COUNT_DOWN, tButtonComplete);
  tButton.setInterval(tButtonInterval, 1);
  tButton.start();

  tDisplay.setCounter(24, 0, 0, tDisplay.COUNT_DOWN, tDisplayComplete);
  tDisplay.setInterval(tDisplayInterval, 1);
  tDisplay.start();

  tReadings.setCounter(24, 0, 0, tReadings.COUNT_DOWN, tReadingsComplete);
  tReadings.setInterval(tReadingsInterval, 1000);
  tReadings.start();

  pinMode(LED_OUTPUT_PIN, OUTPUT); // Initialize the 4 pin as an output
  pinMode(LED_INPUT_PIN, INPUT);
  pinMode(VOLTMETER_PIN, INPUT); // Legge il voltaggio e l'amperaggio de
  pinMode(BUTTON_PIN, INPUT);    // Legge il voltaggio e l'amperaggio de
  currentButtonState = digitalRead(BUTTON_PIN);
  Serial.begin(115200);
  setup_wifi();
  setupBmp280();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if (client.connect("campo"))
  {
    client.subscribe(mqttInLed);
  }

  // Setto il valore del pin del led per condividerlo con tutti all'avvio
  digitalCommonAnodeWrite(LED_INPUT_PIN, LOW);
  sendMqttValue(mqttInLed, LOW);
}

void loop()
{
  client.loop();

  tButton.run();
  tReadings.run();

  if (!client.connected())
  {
    tButton.stop();
    tReadings.stop();
    displayTopLeft = "Offline";
    displayBottomRight = "";
    displayBottomLeft = "";

    reconnect();
  }
  else
  {
    tButton.start();
    tReadings.start();
    // Input - Output

    // Display
    displayTopLeft = "Online";
    displayReadings();
  }
  tDisplay.run();
}