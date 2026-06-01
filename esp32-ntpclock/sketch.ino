/*
====================================================
ESP32 + MAX7219 (4 x 8x8 Matrix) WiFi NTP Clock
====================================================

WIRING

MAX7219    -> ESP32

VCC        -> 5V
GND        -> GND
DIN        -> D13
CLK        -> D14
CS         -> D12

Required Libraries:
- MD_MAX72XX
- MD_Parola
====================================================
*/

#include <WiFi.h>
#include <time.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>

// ====================================================
// USER SETTINGS
// ====================================================

// WiFi Credentials
const char* WIFI_SSID     = "";
const char* WIFI_PASSWORD = "";

// Display Brightness (0-15)
#define DISPLAY_INTENSITY 1

// Time Format
#define USE_24_HOUR false

// Blink Colon
#define BLINK_COLON true

// NTP Sync Interval
#define NTP_SYNC_INTERVAL 3600000UL

// India Time Zone
#define GMT_OFFSET_SEC 19800
#define DAYLIGHT_OFFSET_SEC 0

const char* NTP_SERVER = "pool.ntp.org";

// ====================================================
// MAX7219 SETTINGS
// ====================================================

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define DATA_PIN 13
#define CLK_PIN 14
#define CS_PIN 12

// ====================================================
// DISPLAY OBJECT
// ====================================================

MD_Parola display = MD_Parola(
  HARDWARE_TYPE,
  DATA_PIN,
  CLK_PIN,
  CS_PIN,
  MAX_DEVICES
);

// ====================================================
// VARIABLES
// ====================================================

unsigned long lastNtpSync = 0;
bool colonState = true;

// ====================================================
// WIFI CONNECT
// ====================================================

void connectWiFi()
{
  Serial.print("Connecting to WiFi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// ====================================================
// NTP SYNC
// ====================================================

void syncTime()
{
  Serial.println("Syncing NTP Time...");

  configTime(
    GMT_OFFSET_SEC,
    DAYLIGHT_OFFSET_SEC,
    NTP_SERVER
  );

  struct tm timeinfo;

  if (getLocalTime(&timeinfo))
  {
    Serial.println("Time Sync Successful");
    lastNtpSync = millis();
  }
  else
  {
    Serial.println("Time Sync Failed");
  }
}

// ====================================================
// SETUP
// ====================================================

void setup()
{
  Serial.begin(115200);

  display.begin();

  display.setIntensity(DISPLAY_INTENSITY);

  // Center text across all 4 modules
  display.setTextAlignment(PA_CENTER);

  display.displayClear();

  connectWiFi();
  syncTime();
}

// ====================================================
// LOOP
// ====================================================

void loop()
{
  if (millis() - lastNtpSync >= NTP_SYNC_INTERVAL)
  {
    syncTime();
  }

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo))
  {
    display.displayText(
      "ERROR",
      PA_CENTER,
      0,
      0,
      PA_PRINT,
      PA_NO_EFFECT
    );

    display.displayAnimate();

    delay(1000);
    return;
  }

  int displayHour = timeinfo.tm_hour;

  if (!USE_24_HOUR)
  {
    displayHour %= 12;

    if (displayHour == 0)
      displayHour = 12;
  }

  char timeString[6];

  if (BLINK_COLON)
  {
    sprintf(
      timeString,
      colonState ? "%02d:%02d" : "%02d %02d",
      displayHour,
      timeinfo.tm_min
    );

    colonState = !colonState;
  }
  else
  {
    sprintf(
      timeString,
      "%02d:%02d",
      displayHour,
      timeinfo.tm_min
    );
  }

  display.displayText(
    timeString,
    PA_CENTER,
    0,
    0,
    PA_PRINT,
    PA_NO_EFFECT
  );

  display.displayAnimate();

  delay(1000);
}
