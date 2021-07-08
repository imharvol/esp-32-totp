#include "constants.h"

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);  

Base32 base32;

unsigned int last_step = 0;

void setup()
{
  Serial.begin(115200);

  // Screen Setup
  tft.init(SCREEN_WIDTH, SCREEN_HEIGHT, SPI_MODE2);
  tft.setTextWrap(true);
  tft.fillScreen(ST77XX_BLACK);

  // Print "Setup" on the center of the screen
  char setup_text[] = "Setup";
  tft.setTextSize(5);
  int16_t setup_x1, setup_y1;
  uint16_t setup_w, setup_h;
  tft.getTextBounds(setup_text, 0, 0, &setup_x1, &setup_y1, &setup_w, &setup_h);
  int setup_x_pos = (tft.width() - setup_w) / 2;
  int setup_y_pos = (tft.height() - setup_h) / 2;
  tft.setCursor(setup_x_pos, setup_y_pos);
  tft.println(setup_text);

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

  delay(update_delay);
}
