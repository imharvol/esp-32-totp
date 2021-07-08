// https://www.rfc-editor.org/rfc/rfc4226
// https://www.rfc-editor.org/rfc/rfc6238
// https://en.wikipedia.org/wiki/Google_Authenticator

#include "mbedtls/md.h" // https://github.com/ARMmbed/mbedtls/blob/development/include/mbedtls/md.h

#include <WiFi.h>
#include "time.h"
#include "Base32.h" // https://github.com/NetRat/Base32

// Screen
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

const short SHA1_SIZE = 20;
const short TOTP_TIME_STEP = 30;
const short TOTP_CODE_DIGITS = 6;

const char* ssid       = "";
const char* password   = "";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

#define TFT_CS    10
#define TFT_RST   26
#define TFT_DC    27
const unsigned int SCREEN_WIDTH = 240;
const unsigned int SCREEN_HEIGHT= 240;
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);  

Base32 base32;

unsigned int last_step = 0;

byte* hmac (const byte *key, const byte *input, const int key_length, const int input_length, const int hmac_size = SHA1_SIZE) {
    byte *output = new byte[hmac_size];
    
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA1;
    
    mbedtls_md_init(&ctx);
    if (mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1)   != 0) Serial.println("Error (mbedtls_md_setup)");
    if (mbedtls_md_hmac_starts(&ctx, (const byte *)key, key_length)     != 0) Serial.println("Error (mbedtls_md_hmac_starts)");
    if (mbedtls_md_hmac_update(&ctx, (const byte *)input, input_length) != 0) Serial.println("Error (mbedtls_md_hmac_update)");
    if (mbedtls_md_hmac_finish(&ctx, output)                            != 0) Serial.println("Error (mbedtls_md_hmac_finish)");
    mbedtls_md_free(&ctx);
    
    return output;
}

void printHmac (const byte *hmac, const int hmac_size = SHA1_SIZE) {
  for(int i= 0; i< hmac_size; i++) {
    char str[3];
    sprintf(str, "%02x", (int)hmac[i]);
    Serial.print(str);
  }
  Serial.println();
}

unsigned int hotp (const byte *key, unsigned long counter, const int key_length) {
  short input_length = 8;
  // Put counter into "text byte array". I don't quite understand this part. Specially I don't understand why I can't change the value input_length to other than 8.
  byte* input = new byte[input_length];
  for (int i = input_length - 1; i >= 0; i--) {
    input[i] = (byte) (counter & 0xff);
    counter >>= input_length;
  }

  byte *output = hmac((const byte *)key, (const byte *)input, key_length, input_length);

  int truncateOffset = output[SHA1_SIZE-1] & 0xf;
  int bin_code = (output[truncateOffset] & 0x7f) << 24
    | (output[truncateOffset+1] & 0xff) << 16
    | (output[truncateOffset+2] & 0xff) << 8
    | (output[truncateOffset+3] & 0xff);

  delete output;

  return bin_code%(int)pow(10, TOTP_CODE_DIGITS);
}

unsigned int totp (const byte *key, const int key_length) {
  time_t now;
  unsigned long timestamp = time(&now);
  
  unsigned long counter = timestamp/TOTP_TIME_STEP;
  
  return hotp(key, counter, key_length);
}

void setup()
{
  Serial.begin(115200);

  // Screen Setup
  tft.init(SCREEN_WIDTH, SCREEN_HEIGHT, SPI_MODE2);
  tft.setTextWrap(true);
  tft.fillScreen(ST77XX_BLACK);

  // Print "Setup" on the center of the screen
  tft.setTextSize(5);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds("Setup", 20, 20, &x1, &y1, &w, &h);
  int xPos = (tft.width() - w) / 2;
  int yPos = (tft.height() - h) / 2;
  tft.setCursor(xPos, yPos);
  tft.println("Setup");
  
  // Connect to WIFI
  Serial.printf("Connecting to %s ...", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");

  // Get and set the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) Serial.println("Failed to obtain time");
  
  // Disconnect WIFI as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // Ready to use
  tft.fillScreen(ST77XX_BLACK);
}

void loop()
{
  char key_base32[] = "KEY";
  
  time_t now;
  unsigned long timestamp = time(&now);
  unsigned current_step = timestamp/30;
  
  if (current_step != last_step) {
    last_step = current_step;
    tft.fillScreen(ST77XX_BLACK);

    byte* key_bytes;
    int key_length = base32.fromBase32((byte *)key_base32, sizeof(key_base32)-1, (byte*&)key_bytes); // For some reason, this only works by using sizeof(key)-1
    int totp_output = totp((const byte *)key_bytes, key_length);
  
    tft.setCursor(0, 0);
    tft.setTextSize(4);
    tft.println(totp_output);
  }

  // Progress bar
  int progress_bar_height = 10;
  int progress_bar_width = ((float)(timestamp%TOTP_TIME_STEP) / (float)TOTP_TIME_STEP) * (float)tft.width();
  tft.writeFillRect(0, tft.height()-progress_bar_height, progress_bar_width, progress_bar_height, 0xF8);
  tft.writeLine(0, tft.height()-progress_bar_height-1, tft.width(), tft.height()-progress_bar_height-1, 0xFFFFFF);

  delay(1000);
}
