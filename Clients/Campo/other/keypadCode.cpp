#include <Keypad.h>
#include <Countimer.h>
#include <U8glib.h>

#define DEBUGLN(A) Serial.println(A);
#define DEBUG(A) Serial.print(A);

#define LED_RED_PIN 12
#define LED_GREEN_PIN 11
#define LED_BLUE_PIN 10

#define LED_HIGH LOW
#define LED_LOW HIGH

#define BATTERY_VOLTAGE_PIN A0
#define BATTERY_BASE_VOLTAGE 3.3
#define BATTERY_ANALOG_VALUE 330

//auto redLed = JLed(LED_RED_PIN).LowActive();
//auto greenLed = JLed(LED_GREEN_PIN).LowActive();
//auto blueLed = JLed(LED_BLUE_PIN).LowActive();

Countimer tDown;
Countimer tBattery;
Countimer tDisplay;

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};
byte rowPins[ROWS] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8};    //connect to the column pinouts of the keypad

String codeTmp = "";
String code = "";
String codeVerify = "";

String timerValueHoursTmp = "";
String timerValueMinutesTmp = "";
String timerValueSecondsTmp = "";

String displayTopLeft = "";
String displayTopRight = "";
String displayBottomLeft = "";
String displayBottomRight = "";

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// display
U8GLIB_SSD1306_128X32 u8g(U8G_I2C_OPT_NONE);

//

int buzzerPin = 9;

int codeMaxLength = 4;

int codeMinLength = 4;

bool verifyMode = false;

bool isDisarmed = false;

uint16_t timerValueHours = 0;
uint16_t timerValueMinutes = 0;
uint16_t timerValueSeconds = 0;

bool timerEnd = false;

bool tmpReset = false;

bool setHoursMode = true;
bool setMinutesMode = true;
bool setSecondsMode = true;
bool timerConfigured = false;

bool tmpTimerAccept = false;

void tDownComplete()
{
  resetLed();
  DEBUGLN("Timer Terminato");
  displayBottomLeft = "Press * to reset";
  displayTopLeft = "!EXPLODED!";
  timerEnd = true;
  digitalWrite(LED_RED_PIN, LED_HIGH);
  tone(buzzerPin, 4000, 1000);
}

void tDownInterval()
{
  resetLed();
  digitalWrite(LED_GREEN_PIN, LED_HIGH);
  tone(buzzerPin, 4000, 50);

  // Da scrivere a schermo
  DEBUGLN(tDown.getCurrentTime());
  displayTopLeft = tDown.getCurrentTime();
}

void tBatteryComplete() {}

void tBatteryInterval()
{
  //DEBUGLN(String(analogRead(BATTERY_VOLTAGE_PIN)) + " - " + String(getBatteryVoltage()) + "V - " + String(getBatteryPercentage()) + "%");
  //// Da scrivere a schermo
  //DEBUGLN("Battery: " + String(getBatteryPercentage()) + "%");
  displayTopRight = String(String(getBatteryPercentage()) + "%");
}

void drawDisplay(char topLeft[], char topRight[], char bottomLeft[], char bottomRight[])
{
  // graphic commands to redraw the complete screen should be placed here
  u8g.setFont(u8g_font_unifont);
  //u8g.setFont(u8g_font_osb21);
  u8g.drawStr(0, 11, topLeft);
  u8g.drawStr(104, 11, topRight);
  u8g.drawStr(0, 32, bottomLeft);
  u8g.drawStr(80, 32, bottomRight);
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

  u8g.firstPage();
  do
  {
    drawDisplay(displayTopLeftBuf, displayTopRightBuf, displayBottomLeftBuf, displayBottomRightBuf);
  } while (u8g.nextPage());
}

double mapf(double val, double in_min, double in_max, double out_min, double out_max)
{
  return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup()
{

  if (u8g.getMode() == U8G_MODE_R3G3B2)
  {
    u8g.setColorIndex(255); // white
  }
  else if (u8g.getMode() == U8G_MODE_GRAY2BIT)
  {
    u8g.setColorIndex(3); // max intensity
  }
  else if (u8g.getMode() == U8G_MODE_BW)
  {
    u8g.setColorIndex(1); // pixel on
  }
  else if (u8g.getMode() == U8G_MODE_HICOLOR)
  {
    u8g.setHiColorByRGB(255, 255, 255);
  }

  tDisplay.setCounter(24, 0, 0, tDisplay.COUNT_DOWN, tDisplayComplete);
  tDisplay.setInterval(tDisplayInterval, 1);
  tDisplay.start();

  tBattery.setCounter(24, 0, 0, tBattery.COUNT_DOWN, tBatteryComplete);
  tBattery.setInterval(tBatteryInterval, 1000);
  tBattery.start();

  Serial.begin(9600);
  pinMode(buzzerPin, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);

  // Il Led ad anodo comune
  digitalWrite(LED_RED_PIN, LED_LOW);
  digitalWrite(LED_GREEN_PIN, LED_LOW);
  digitalWrite(LED_BLUE_PIN, LED_LOW);

  //tDown.setCounter(0, 0, startValue, tDown.COUNT_DOWN, tDownComplete);

  DEBUGLN("Programma Avviato");
  DEBUGLN("Configurare il timer");
  DEBUGLN("Ore: ");

  displayTopLeft = "Prog Start";

  tone(buzzerPin, 4000, 50);
  delay(100);
  tone(buzzerPin, 4000, 50);
  delay(100);
  tone(buzzerPin, 4000, 50);

  displayTopLeft = "Config Timer";
  displayBottomLeft = "Ore:";

  resetLed();
  digitalWrite(LED_BLUE_PIN, LED_HIGH);

  pinMode(BATTERY_VOLTAGE_PIN, INPUT);
}

double getBatteryVoltage()
{
  return mapf(analogRead(BATTERY_VOLTAGE_PIN), 0, BATTERY_ANALOG_VALUE, 0, BATTERY_BASE_VOLTAGE);
}

uint8_t getBatteryPercentage()
{

  return map(analogRead(BATTERY_VOLTAGE_PIN), 0, BATTERY_ANALOG_VALUE, 0, 100);
}

void loop()
{
  tDown.run();
  tBattery.run();
  tDisplay.run();
  char key = keypad.getKey();

  if (key != NO_KEY)
  {
    if (!timerConfigured)
    {
      // se ho appena acceso il dispositivo devo inserire il valore del countdown
      if (key == '*')
      {
        timerValueHoursTmp = "";
        timerValueMinutesTmp = "";
        timerValueSecondsTmp = "";
        setHoursMode = true;
        setMinutesMode = true;
        setSecondsMode = true;
        DEBUGLN("Configurazione Timer Annullata");
        tone(buzzerPin, 1000, 50);
        delay(100);
        tone(buzzerPin, 1000, 50);
        delay(100);
        tone(buzzerPin, 1000, 50);
        timerConfigured = false;
        DEBUGLN("Configurare il timer");
        DEBUGLN("Ore: ");
        displayTopLeft = "Config Timer";
        displayBottomLeft = "Ore: ";
        displayBottomRight = "";
      }
      else if (key == '#')
      {

        // Reset Error Message
        displayBottomRight = "";

        if (setHoursMode)
        {

          if (timerValueHoursTmp.toInt() <= 24 && timerValueHoursTmp.toInt() >= 0 && timerValueHoursTmp != "")
          {
            DEBUGLN();
            DEBUGLN("Ore Impostate: " + timerValueHoursTmp);
            tone(buzzerPin, 2100, 50);
            delay(100);
            tone(buzzerPin, 2100, 50);
            setHoursMode = false;
            DEBUGLN("Minuti:");
            displayBottomLeft = String("Min: ");
          }
          else
          {
            displayBottomLeft = "Ore: ";
            displayBottomRight = "ERR#01";
            timerValueHoursTmp = "";
            DEBUGLN("Errore: La giornata dura massimo 24h");
          }
        }
        else if (setMinutesMode)
        {
          if (timerValueMinutesTmp.toInt() <= 59 && timerValueMinutesTmp.toInt() >= 0 && timerValueMinutesTmp != "")
          {
            DEBUGLN();
            DEBUGLN("Minuti Impostati: " + timerValueMinutesTmp);
            tone(buzzerPin, 2100, 50);
            delay(100);
            tone(buzzerPin, 2100, 50);
            setMinutesMode = false;
            DEBUGLN("Secondi:");
            displayBottomLeft = String("Sec: ");
          }
          else
          {
            timerValueMinutesTmp = "";
            displayBottomLeft = String("Min: ");
            displayBottomRight = "ERR#01";
            DEBUGLN("Errore: Ci sono massimo 60m in 1 ora");
          }
        }
        else if (setSecondsMode)
        {
          if (timerValueSecondsTmp.toInt() <= 59 && timerValueSecondsTmp.toInt() >= 0 && timerValueSecondsTmp != "")
          {
            DEBUGLN();
            DEBUGLN("Secondi Impostati: " + timerValueSecondsTmp);
            tone(buzzerPin, 2100, 50);
            delay(100);
            tone(buzzerPin, 2100, 50);
            setSecondsMode = false;
            DEBUGLN("Premere 2 volte # per accettare e * per annullare: " + timerValueHoursTmp + "h: " + timerValueMinutesTmp + "m: " + timerValueSecondsTmp + "s");
            displayBottomLeft = String(timerValueHoursTmp + ":" + timerValueMinutesTmp + ":" + timerValueSecondsTmp + " - OK?");
          }
          else
          {
            displayBottomLeft = String("Sec: ");
            timerValueSecondsTmp = "";
            displayBottomRight = "ERR#01";
            DEBUGLN("Errore: Ci sono massimo 60s in 1 minuto");
          }
        }
        else
        {
          if (key == '#')
          {
            uint16_t timerValueHours = timerValueHoursTmp.toInt();
            uint16_t timerValueMinutes = timerValueMinutesTmp.toInt();
            uint16_t timerValueSeconds = timerValueSecondsTmp.toInt();
            DEBUGLN("Configurazione Salvata: " + timerValueHoursTmp + "h: " + timerValueMinutesTmp + "m: " + timerValueSecondsTmp + "s");
            tDown.setCounter(timerValueHours, timerValueMinutes, timerValueSeconds, tDown.COUNT_DOWN, tDownComplete);
            tDown.setInterval(tDownInterval, 1000);
            tDown.stop();
            DEBUGLN("Timer Impostato: " + String(tDown.getCurrentTime()));
            tmpTimerAccept = false;
            tone(buzzerPin, 4000, 50);
            delay(100);
            tone(buzzerPin, 4000, 50);
            delay(100);
            tone(buzzerPin, 4000, 50);
            timerConfigured = true;
            DEBUGLN("Inserire Il Codice");
            displayTopLeft = "Config Bomb";
            displayBottomLeft = "Code: ";
            resetLed();
            digitalWrite(LED_BLUE_PIN, LED_HIGH);
          }
        }
      }
      else
      {

        // clear error message
        displayBottomRight = "";

        if (setHoursMode)
        {
          timerValueHoursTmp += key;
          DEBUG(key);
          displayBottomLeft = String("Ore: " + timerValueHoursTmp);
          tone(buzzerPin, 2000, 50);
        }
        else if (setMinutesMode)
        {

          timerValueMinutesTmp += key;
          DEBUG(key);
          displayBottomLeft = String("Min: " + timerValueMinutesTmp);
          tone(buzzerPin, 2000, 50);
        }
        else
        {

          timerValueSecondsTmp += key;
          DEBUG(key);
          displayBottomLeft = String("Sec: " + timerValueSecondsTmp);
          tone(buzzerPin, 2000, 50);
        }
      }
    }
    else
    {
      if (!timerEnd)
      {
        if (key == '*' && !verifyMode && !isDisarmed)
        {
          displayBottomRight = "";
          resetDevice();
          digitalWrite(LED_BLUE_PIN, LED_HIGH);
          DEBUGLN("Codice Eliminato");
          displayTopLeft = "Config Bomb";
          displayBottomLeft = "Code: ";
          tone(buzzerPin, 1000, 50);
          delay(100);
          tone(buzzerPin, 1000, 50);
          delay(100);
          tone(buzzerPin, 1000, 50);
        }
        else if (key == '#')
        {
          displayBottomRight = "";
          if (codeTmp.length() >= codeMinLength)
          {
            if (!verifyMode)
            {
              code = codeTmp;
              DEBUGLN("Codice Salvato, Timer Partito");
              DEBUGLN("Inserisci Password");
              displayBottomLeft = "Code: ";
              tone(buzzerPin, 4000, 50);
              verifyMode = true;

              // Faccio partire il contatore
              tDown.start();
            }
            else
            {
              if (code == codeVerify)
              {
                resetDevice();
                DEBUGLN("Codice Corretto");
                DEBUGLN("Reset Dispositivo");
                DEBUGLN("Inserire Il Codice");
                displayTopLeft = "!DISARMED!";
                displayBottomLeft = "Press * to reset";
                resetLed();
                tDown.stop();
                verifyMode = false;
                tone(buzzerPin, 4000, 50);
                delay(100);
                tone(buzzerPin, 4000, 200);
                resetLed();
                digitalWrite(LED_BLUE_PIN, LED_HIGH);
              }
              else
              {
                codeVerify = "";
                displayTopLeft = "!EXPLODED!";
                resetLed();
                digitalWrite(LED_RED_PIN, LED_HIGH);
                tDown.stop();
                verifyMode = false;
                displayBottomLeft = "Press * to reset";
                DEBUGLN("Codice Errato: Riprova");
                tone(buzzerPin, 1000, 200);
              }
            }
          }
          else
          {
            codeTmp = "";
            DEBUGLN("Codice Troppo Corto: Riprova");
            displayBottomRight = "ERR#02";
            tone(buzzerPin, 1000, 200);
          }
        }
        else
        {
          displayBottomRight = "";
          if (!verifyMode)
          {
            // Se il codice � ancora da salvare inserisco i caratteri
            if (codeTmp.length() < codeMaxLength)
            {
              codeTmp += key;
              DEBUG(key);
              displayBottomLeft = "Code: " + codeTmp;
              tone(buzzerPin, 2000, 50);
            }
            else
            {
              tone(buzzerPin, 1000, 200);
            }
          }
          else
          {
            // Se ho salvato il codice entro in modalit� verifica
            if (codeVerify.length() < codeMaxLength)
            {
              codeVerify += key;
              DEBUG(key);
              displayBottomLeft = "Code: " + codeVerify;
              tone(buzzerPin, 2000, 50);
            }
            else
            {
              tone(buzzerPin, 1000, 200);
            }
          }
        }
      }
      else
      {
        displayBottomRight = "";
        if (key == '*')
        {
          resetDevice();
          DEBUGLN("Programma resettato");
          DEBUGLN("Inserire Il Codice");
          displayTopLeft = "Config Bomb";
          displayBottomLeft = "Code: ";
          tone(buzzerPin, 4000, 50);
          delay(100);
          tone(buzzerPin, 4000, 50);
          delay(100);
          tone(buzzerPin, 4000, 50);
          resetLed();
          digitalWrite(LED_BLUE_PIN, LED_HIGH);
        }
        else
        {
          DEBUGLN("Input non valido: premere * poi # per resettare");
          displayBottomRight = "ERR#03";
          tone(buzzerPin, 1000, 200);
        }
      }
    }
  }
}

void resetDevice()
{
  code = "";
  codeVerify = "";
  codeTmp = "";
  verifyMode = false;
  tmpReset = false;
  tDown.stop();
  timerEnd = false;
  resetLed();
}

void resetLed()
{
  digitalWrite(LED_RED_PIN, LED_LOW);
  digitalWrite(LED_GREEN_PIN, LED_LOW);
  digitalWrite(LED_BLUE_PIN, LED_LOW);
}
