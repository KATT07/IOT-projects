/*
====================================================
ESP32 + MAX7219 (4 x 8x8 Matrix) WiFi NTP Clock
Tasmota-Style Thin 5x7 Digits
====================================================

MAX7219 -> ESP32

VCC -> 5V
GND -> GND
DIN -> D13
CLK -> D14
CS  -> D12

Required Library:
MD_MAX72XX
====================================================
*/

#include <WiFi.h>
#include <time.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// ====================================================
// USER SETTINGS
// ====================================================

const char* WIFI_SSID     = "";
const char* WIFI_PASSWORD = "";

#define DISPLAY_INTENSITY 1      // 0-15
#define USE_24_HOUR false
#define BLINK_COLON true

#define NTP_SYNC_INTERVAL 3600000UL  // 1 hour

#define GMT_OFFSET_SEC      19800    // IST
#define DAYLIGHT_OFFSET_SEC 0

const char* NTP_SERVER = "pool.ntp.org";

// ====================================================
// MAX7219 SETTINGS
// ====================================================

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES   4

#define DATA_PIN 13
#define CLK_PIN  14
#define CS_PIN   12

// ====================================================
// DISPLAY LAYOUT
// ====================================================

// FC16 chain order on this display is reversed.
// These positions were tuned for the current hardware.

const uint8_t POS_M2 = 1;
const uint8_t POS_M1 = 9;
const uint8_t POS_H2 = 18;
const uint8_t POS_H1 = 26;

const uint8_t COLON_LEFT  = 15;
const uint8_t COLON_RIGHT = 16;

// ====================================================
// DISPLAY OBJECT
// ====================================================

MD_MAX72XX mx(
  HARDWARE_TYPE,
  DATA_PIN,
  CLK_PIN,
  CS_PIN,
  MAX_DEVICES
);

// ====================================================
// THIN 5x7 FONT
// Each byte = one column
// ====================================================

const uint8_t PROGMEM font[10][5] =
{
  {0x3E,0x51,0x49,0x45,0x3E}, // 0
  {0x00,0x42,0x7F,0x40,0x00}, // 1
  {0x42,0x61,0x51,0x49,0x46}, // 2
  {0x21,0x41,0x45,0x4B,0x31}, // 3
  {0x18,0x14,0x12,0x7F,0x10}, // 4
  {0x27,0x45,0x45,0x45,0x39}, // 5
  {0x3C,0x4A,0x49,0x49,0x30}, // 6
  {0x01,0x71,0x09,0x05,0x03}, // 7
  {0x36,0x49,0x49,0x49,0x36}, // 8
  {0x06,0x49,0x49,0x29,0x1E}  // 9
};

// ====================================================
// GLOBALS
// ====================================================

unsigned long lastNtpSync = 0;
bool colonState = true;

// ====================================================
// WIFI
// ====================================================

void connectWiFi()
{
  Serial.print("Connecting");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ====================================================
// NTP
// ====================================================

void syncTime()
{
  configTime(
    GMT_OFFSET_SEC,
    DAYLIGHT_OFFSET_SEC,
    NTP_SERVER
  );

  struct tm timeinfo;

  if (getLocalTime(&timeinfo))
  {
    lastNtpSync = millis();
    Serial.println("NTP Sync OK");
  }
}

// ====================================================
// DRAW DIGIT
// ====================================================

void drawDigit(uint8_t digit, uint8_t x)
{
  for (uint8_t col = 0; col < 5; col++)
  {
    // Reverse columns because display is mirrored
    uint8_t columnData = pgm_read_byte(&font[digit][4 - col]);

    for (uint8_t row = 0; row < 7; row++)
    {
      bool pixel = columnData & (1 << row);
      mx.setPoint(row, x + col, pixel);
    }
  }
}

// ====================================================
// DRAW COLON
// ====================================================

void drawColon(bool visible)
{
  if (!visible) return;

  mx.setPoint(2, COLON_LEFT,  true);
  mx.setPoint(4, COLON_LEFT,  true);

  mx.setPoint(2, COLON_RIGHT, true);
  mx.setPoint(4, COLON_RIGHT, true);
}

// ====================================================
// SETUP
// ====================================================

void setup()
{
  Serial.begin(115200);

  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, DISPLAY_INTENSITY);
  mx.clear();

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
    delay(1000);
    return;
  }

  int displayHour = timeinfo.tm_hour;

  if (!USE_24_HOUR)
  {
    displayHour = ((displayHour - 1) % 12) + 1;
  }

  const uint8_t h1 = displayHour / 10;
  const uint8_t h2 = displayHour % 10;

  const uint8_t m1 = timeinfo.tm_min / 10;
  const uint8_t m2 = timeinfo.tm_min % 10;

  mx.clear();

  // Display order is reversed due to FC16 chain orientation
  drawDigit(m2, POS_M2);
  drawDigit(m1, POS_M1);

  drawDigit(h2, POS_H2);
  drawDigit(h1, POS_H1);

  drawColon(!BLINK_COLON || colonState);

  if (BLINK_COLON)
  {
    colonState = !colonState;
  }

  mx.update();

  delay(1000);
}
