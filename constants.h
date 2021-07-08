#include <WiFi.h>
#include "time.h"
#include "Base32.h" // https://github.com/NetRat/Base32

#include "mbedtls/md.h" // https://github.com/ARMmbed/mbedtls/blob/development/include/mbedtls/md.h

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// WIFI
const char* ssid       = "";
const char* password   = "";

// TOTP
const short SHA1_SIZE = 20;
const short TOTP_TIME_STEP = 30;
const short TOTP_CODE_DIGITS = 6;

// Time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

// TFT Screen
#define TFT_CS    10
#define TFT_RST   26
#define TFT_DC    27
const unsigned int SCREEN_WIDTH = 240;
const unsigned int SCREEN_HEIGHT= 240;

const unsigned int update_delay = 1000;
