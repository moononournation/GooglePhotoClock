/*******************************************************************************
 * Google Photo Clock
 * This is a simple IoT photo frame sample
 * Please find more details at instructables:
 * https://www.instructables.com/id/Google-Photo-Clock/
 * 
 * Setup steps:
 * 1. Fill your own SSID_NAME, SSID_PASSWORD and URL_TEMPLATE
 * 2. Change your LCD parameters in Arduino_GFX setting
 ******************************************************************************/

/* WiFi settings */
#define SSID_NAME "YourAP"
#define SSID_PASSWORD "PleaseInputYourPasswordHere"
/* Google Photo settings */
// replace your Google Photo share link
#define GOOGLE_PHOTO_SHARE_LINK "https://photos.app.goo.gl/j9kVKWsfWq9N1jD88"
#define PHOTO_URL_PREFIX "https://lh3.googleusercontent.com/"
#define SEEK_PATTERN "id=\"_ij\""
#define SEARCH_PATTERN "\",[\"" PHOTO_URL_PREFIX
#define PHOTO_LIMIT 20                                     // read first 20 photos to the list, ESP32 can add more
#define PHOTO_ID_SIZE 141                                  // the photo ID should be 140 charaters long and then add a zero-tail
#define PHOTO_FILE_BUFFER 92160                            // 90 KB, a 320 x 480 Google JPEG compressed photo size (320 x 480 x 3 bytes / 5). Only ESP32 have enough RAM to allocate this buffer
#define HTTP_TIMEOUT 60000                                 // in ms, wait a while for server processing
#define PHOTO_URL_TEMPLATE PHOTO_URL_PREFIX "%s=w%d-h%d-c" // photo id, display width and height
/* NTP settings */
#define NTP_SERVER "pool.ntp.org"
#if defined(ESP32)
#define GMT_OFFSET_SEC 28800L  // Timezone +0800
#define DAYLIGHT_OFFSET_SEC 0L // no daylight saving
#elif defined(ESP8266)
#define TZ "HKT-8"
#endif
/*******************************************************************************
 * Start of Arduino_GFX setting
 ******************************************************************************/
#include "SPI.h"
#include "Arduino_HWSPI.h"
#include "Arduino_ESP32SPI.h"
#include "Arduino_SWSPI.h"
#include "Arduino_GFX.h"            // Core graphics library
#include "Arduino_Canvas.h"         // Canvas (framebuffer) library
#include "Arduino_Canvas_Indexed.h" // Indexed Color Canvas (framebuffer) library
#include "Arduino_HX8347C.h"        // Hardware-specific library for HX8347C
#include "Arduino_HX8352C.h"        // Hardware-specific library for HX8352C
#include "Arduino_HX8357B.h"        // Hardware-specific library for HX8357B
#include "Arduino_ILI9225.h"        // Hardware-specific library for ILI9225
#include "Arduino_ILI9341.h"        // Hardware-specific library for ILI9341
#include "Arduino_ILI9481_18bit.h"  // Hardware-specific library for ILI9481
#include "Arduino_ILI9486_18bit.h"  // Hardware-specific library for ILI9486
#include "Arduino_SEPS525.h"        // Hardware-specific library for SEPS525
#include "Arduino_SSD1283A.h"       // Hardware-specific library for SSD1283A
#include "Arduino_SSD1331.h"        // Hardware-specific library for SSD1331
#include "Arduino_SSD1351.h"        // Hardware-specific library for SSD1351
#include "Arduino_ST7735.h"         // Hardware-specific library for ST7735
#include "Arduino_ST7789.h"         // Hardware-specific library for ST7789
#include "Arduino_ST7796.h"         // Hardware-specific library for ST7796

#if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5STACK_FIRE)
#define TFT_BL 32
#include "Arduino_ILI9341_M5STACK.h"
Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(27 /* DC */, 14 /* CS */, SCK, MOSI, MISO);
Arduino_ILI9341_M5STACK *gfx = new Arduino_ILI9341_M5STACK(bus, 33 /* RST */, 1 /* rotation */);
#elif defined(ARDUINO_ODROID_ESP32)
#define TFT_BL 14
Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(21 /* DC */, 5 /* CS */, SCK, MOSI, MISO);
Arduino_ILI9341 *gfx = new Arduino_ILI9341(bus, -1 /* RST */, 3 /* rotation */);
// Arduino_ST7789 *gfx = new Arduino_ST7789(bus,  -1 /* RST */, 1 /* rotation */, true /* IPS */);
#elif defined(ARDUINO_T) // TTGO T-Watch
#define TFT_BL 12
Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(27 /* DC */, 5 /* CS */, 18 /* SCK */, 19 /* MOSI */, -1 /* MISO */);
Arduino_ST7789 *gfx = new Arduino_ST7789(bus, -1 /* RST */, 2 /* rotation */, true /* IPS */, 240, 240, 0, 80);
#else /* not a specific hardware */

#if defined(ESP32)
#define TFT_CS 5
// #define TFT_CS -1 // for display without CS pin
#define TFT_DC 16
// #define TFT_DC 27
// #define TFT_DC -1 // for display without DC pin (9-bit SPI)
#define TFT_RST 17
// #define TFT_RST 33
#define TFT_BL 22
#elif defined(ESP8266)
#define TFT_CS 15
#define TFT_DC 5
#define TFT_RST -1
// #define TFT_BL 4
#else
#define TFT_CS 20
#define TFT_DC 19
#define TFT_RST 18
#define TFT_BL 10
#endif

/*
 * Step 1: Initize one databus for your display
*/

// General software SPI
// Arduino_DataBus *bus = new Arduino_SWSPI(TFT_DC, TFT_CS, 18 /* SCK */, 23 /* MOSI */, -1 /* MISO */);

// General hardware SPI
Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS);

// ESP32 hardware SPI, more customizable parameters
// Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, 18 /* SCK */, 23 /* MOSI */, -1 /* MISO */, VSPI /* spi_num */);

/*
 * Step 2: Initize one driver for your display
*/

// Canvas (framebuffer)
// Arduino_ST7789 *output_display = new Arduino_ST7789(bus, TFT_RST, 0 /* rotation */, true /* IPS */);
// 16-bit color Canvas (240x320 resolution only works for ESP32 with PSRAM)
// Arduino_Canvas *gfx = new Arduino_Canvas(240, 320, output_display);
// Indexed color Canvas, mask_level: 0-2, larger mask level mean less color variation but can have faster index mapping
// Arduino_Canvas_Indexed *gfx = new Arduino_Canvas_Indexed(240, 320, output_display, MAXMASKLEVEL /* mask_level */);

// HX8347C IPS LCD 240x320
// Arduino_HX8347C *gfx = new Arduino_HX8347C(bus, TFT_RST, 0 /* rotation */, true /* IPS */);

// HX8352C IPS LCD 240x400
// Arduino_HX8352C *gfx = new Arduino_HX8352C(bus, TFT_RST, 0 /* rotation */, true /* IPS */);

// HX8357B IPS LCD 320x480
// Arduino_HX8357B *gfx = new Arduino_HX8357B(bus, TFT_RST, 0 /* rotation */, true /* IPS */);

// ILI9225 LCD 176x220
// Arduino_ILI9225 *gfx = new Arduino_ILI9225(bus, TFT_RST);

// ILI9341 LCD 240x320
Arduino_ILI9341 *gfx = new Arduino_ILI9341(bus, TFT_RST);

// ILI9481 LCD 320x480
// Arduino_ILI9481_18bit *gfx = new Arduino_ILI9481_18bit(bus, TFT_RST);

// ILI9486 LCD 320x480
// Arduino_ILI9486_18bit *gfx = new Arduino_ILI9486_18bit(bus, TFT_RST);

// SEPS525 OLED 160x128
// Arduino_SEPS525 *gfx = new Arduino_SEPS525(bus, TFT_RST);

// SSD1283A OLED 130x130
// Arduino_SSD1283A *gfx = new Arduino_SSD1283A(bus, TFT_RST);

// SSD1331 OLED 96x64
// Arduino_SSD1331 *gfx = new Arduino_SSD1331(bus, TFT_RST);

// SSD1351 OLED 128x128
// Arduino_SSD1351 *gfx = new Arduino_SSD1351(bus, TFT_RST);

// ST7735 LCD
// 1.8" REDTAB 128x160
// Arduino_ST7735 *gfx = new Arduino_ST7735(bus, TFT_RST);
// 1.8" BLACKTAB 128x160
// Arduino_ST7735 *gfx = new Arduino_ST7735(bus, TFT_RST, 0 /* rotation */, false /* IPS */, 128 /* width */, 160 /* height */, 2 /* col offset 1 */, 1 /* row offset 1 */, 2 /* col offset 2 */, 1 /* row offset 2 */, false /* BGR */);
// 1.8" GREENTAB A 128x160
// Arduino_ST7735 *gfx = new Arduino_ST7735(bus, TFT_RST, 0 /* rotation */, false /* IPS */, 128 /* width */, 160 /* height */, 2 /* col offset 1 */, 1 /* row offset 1 */, 2 /* col offset 2 */, 1 /* row offset 2 */);
// 1.8" GREENTAB B 128x160
// Arduino_ST7735 *gfx = new Arduino_ST7735(bus, TFT_RST, 0 /* rotation */, false /* IPS */, 128 /* width */, 160 /* height */, 2 /* col offset 1 */, 3 /* row offset 1 */, 2 /* col offset 2 */, 1 /* row offset 2 */);
// 1.8" Wide angle LCD 128x160
// Arduino_ST7735 *gfx = new Arduino_ST7735(bus, TFT_RST, 2 /* rotation */, false /* IPS */, 128 /* width */, 160 /* height */, 0 /* col offset 1 */, 0 /* row offset 1 */, 0 /* col offset 2 */, 0 /* row offset 2 */, false /* BGR */);
// 1.5" GREENTAB B 128x128
// Arduino_ST7735 *gfx = new Arduino_ST7735(bus, TFT_RST, 0 /* rotation */, false /* IPS */, 128 /* width */, 128 /* height */, 2 /* col offset 1 */, 3 /* row offset 1 */, 2 /* col offset 2 */, 1 /* row offset 2 */);
// 1.5" GREENTAB C 128x128
// Arduino_ST7735 *gfx = new Arduino_ST7735(bus, TFT_RST, 0 /* rotation */, false /* IPS */, 128 /* width */, 128 /* height */, 0 /* col offset 1 */, 32 /* row offset 1 */);
// 0.96" IPS LCD 80x160
// Arduino_ST7735 *gfx = new Arduino_ST7735(bus, TFT_RST, 3 /* rotation */, true /* IPS */, 80 /* width */, 160 /* height */, 26 /* col offset 1 */, 1 /* row offset 1 */, 26 /* col offset 2 */, 1 /* row offset 2 */);

// ST7789 LCD
// 2.4" LCD 240x320
// Arduino_ST7789 *gfx = new Arduino_ST7789(bus, TFT_RST);
// 2.4" IPS LCD 240x320
// Arduino_ST7789 *gfx = new Arduino_ST7789(bus, TFT_RST, 0 /* rotation */, true /* IPS */);
// 1.3"/1.5" square IPS LCD 240x240
// Arduino_ST7789 *gfx = new Arduino_ST7789(bus, TFT_RST, 2 /* rotation */, true /* IPS */, 240 /* width */, 240 /* height */, 0 /* col offset 1 */, 80 /* row offset 1 */);
// 1.14" IPS LCD 135x240 TTGO T-Display
// Arduino_ST7789 *gfx = new Arduino_ST7789(bus, TFT_RST, 0 /* rotation */, true /* IPS */, 135 /* width */, 240 /* height */, 53 /* col offset 1 */, 40 /* row offset 1 */, 52 /* col offset 2 */, 40 /* row offset 2 */);

// ST7796 LCD
// 4" LCD 320x480
// Arduino_ST7796 *gfx = new Arduino_ST7796(bus, TFT_RST);
// 4" IPS LCD 320x480
// Arduino_ST7796 *gfx = new Arduino_ST7796(bus, TFT_RST, 0 /* rotation */, true /* IPS */);

#endif /* not a specific hardware */
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

/* WiFi and HTTPS */
#if defined(ESP32)
#include "esp_jpg_decode.h"
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
// HTTPS howto: https://techtutorialsx.com/2017/11/18/esp32-arduino-https-get-request/
const char *rootCACertificate =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4G\n"
    "A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNp\n"
    "Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1\n"
    "MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEG\n"
    "A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n"
    "hvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6ErPL\n"
    "v4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8\n"
    "eoLrvozps6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklq\n"
    "tTleiDTsvHgMCJiEbKjNS7SgfQx5TfC4LcshytVsW33hoCmEofnTlEnLJGKRILzd\n"
    "C9XZzPnqJworc5HGnRusyMvo4KD0L5CLTfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pa\n"
    "zq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6CygPCm48CAwEAAaOBnDCB\n"
    "mTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUm+IH\n"
    "V2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5n\n"
    "bG9iYWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG\n"
    "3lm0mi3f3BmGLjANBgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4Gs\n"
    "J0/WwbgcQ3izDJr86iw8bmEbTUsp9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO\n"
    "291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu01yiPqFbQfXf5WRDLenVOavS\n"
    "ot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG79G+dwfCMNYxd\n"
    "AfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7\n"
    "TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg==\n"
    "-----END CERTIFICATE-----\n";
WiFiMulti WiFiMulti;
WiFiClientSecure *client = new WiFiClientSecure;
#elif defined(ESP8266)
#include "esp_jpg_decode.h"
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <CertStoreBearSSL.h>
ESP8266WiFiMulti WiFiMulti;
BearSSL::WiFiClientSecure *client;
BearSSL::CertStore certStore;
#endif

/* time library */
#include <time.h>
static time_t now;

/* HTTP */
const char *headerkeys[] = {"Location"};
HTTPClient photoHttps;
WiFiClient *photoHttpsStream;

/* Google photo */
char photoIdList[PHOTO_LIMIT][PHOTO_ID_SIZE];
// char *photoIdList[] = {"tdPp9UaNKVtixRBHtOZC285fsmlfsYcPKzE_IF-9GUxlVPbsTnV2LM3Vjyzdn139T8hrk5hJVBfYnwyRmtQMnC4Zznk6fC7_SNCbefhXm4GxLgMvj3gejDraZUW_x5zB83q6B1r-Jvk"};
uint8_t *photoBuf;

/* variables */
static int w, h, timeX, timeY, len, offset, photoCount;
static uint8_t textSize;
static unsigned long next_show_millis = 0;

void ntpGetTime()
{
  // Initialize NTP settings
#if defined(ESP32)
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
#elif defined(ESP8266)
  configTime(TZ, NTP_SERVER);
#endif

  Serial.print(F("Waiting for NTP time sync: "));
  gfx->setTextColor(BLUE);
  gfx->print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2)
  {
    delay(500);
    Serial.print('.');
    gfx->print('.');
    yield();
    nowSecs = time(nullptr);
  }
  Serial.println(asctime(localtime(&nowSecs)));
  gfx->println(asctime(localtime(&nowSecs)));
}

void printTime()
{
  now = time(nullptr);
  const tm *tm = localtime(&now);
  int hour = tm->tm_hour;
  int min = tm->tm_min;
  // print time with border
  gfx->setTextSize(textSize);
  gfx->setCursor(timeX - 2, timeY - 2);
  gfx->setTextColor(DARKGREY);
  gfx->printf("%02d:%02d", hour, min);
  gfx->setCursor(timeX + 2, timeY + 2);
  gfx->setTextColor(BLACK);
  gfx->printf("%02d:%02d", hour, min);
  gfx->setCursor(timeX, timeY);
  gfx->setTextColor(WHITE);
  gfx->printf("%02d:%02d", hour, min);
}

void setup()
{
  Serial.begin(115200);

  // init display
  gfx->begin();
  gfx->fillScreen(BLACK);
  w = gfx->width();
  h = gfx->height();
  textSize = w / 6 / 5;
  timeX = (w - (textSize * 5 * 6)) / 2;
  timeY = h - (textSize * 8) - 10;

  gfx->setTextSize(1);
  gfx->setCursor(0, 0);
  gfx->setTextColor(RED);
  gfx->println(F("Google Photo Clock"));
  Serial.println(F("Google Photo Clock"));
  gfx->setTextColor(YELLOW);
  gfx->print(F("Screen: "));
  Serial.print(F("Screen: "));
  gfx->print(w);
  Serial.print(w);
  gfx->print('x');
  Serial.print('x');
  gfx->println(h);
  Serial.println(h);
  gfx->setTextColor(GREENYELLOW);
  gfx->print(F("Clock text size: "));
  Serial.print(F("Clock text size: "));
  gfx->println(textSize);
  Serial.println(textSize);

  // init WiFi
  gfx->setTextColor(GREEN);
  gfx->print(F("Connecting to WiFi: "));
  Serial.print(F("Connecting to WiFi: "));
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(SSID_NAME, SSID_PASSWORD);
  while ((WiFiMulti.run() != WL_CONNECTED))
  {
    delay(500);
    gfx->print('.');
    Serial.print('.');
  }
  gfx->println(F(" done."));
  Serial.println(F(" done."));

  // init time
  ntpGetTime();

  // init HTTPClient
#if defined(ESP32)
  client->setCACert(rootCACertificate);

  // set WDT timeout a little bit longer than HTTP timeout
  esp_task_wdt_init((HTTP_TIMEOUT / 1000) + 1, true);
  enableLoopWDT();
#elif defined(ESP8266)
  SPIFFS.begin();
  int numCerts = certStore.initCertStore(SPIFFS, "/certs.idx", "/certs.ar");
  gfx->setTextColor(MAGENTA);
  gfx->print(F("Number of CA certs read: "));
  Serial.print(F("Number of CA certs read: "));
  gfx->println(numCerts);
  Serial.println(numCerts);
  client = new BearSSL::WiFiClientSecure();
  client->setCertStore(&certStore);
#endif

  // allocate photo file buffer
  photoBuf = (uint8_t *)malloc(PHOTO_FILE_BUFFER);

  // print time on black screen before down load image
  printTime();

#ifdef TFT_BL
  // finally turn on display backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
#endif
}

void loop()
{
  if (WiFiMulti.run() != WL_CONNECTED)
  {
    // wait for WiFi connection
    delay(500);
  }
  else if (millis() < next_show_millis)
  {
    delay(1000);
  }
  else
  {
    next_show_millis = ((millis() / 60000L) + 1) * 60000L; // next minute

    if (!photoCount)
    {
      HTTPClient https;
      https.collectHeaders(headerkeys, sizeof(headerkeys) / sizeof(char *));

      Serial.println(F(GOOGLE_PHOTO_SHARE_LINK));
      gfx->setTextSize(1);
      gfx->setCursor(0, 64);
      gfx->setTextColor(RED);
      gfx->println(F(GOOGLE_PHOTO_SHARE_LINK));
      Serial.println(F(GOOGLE_PHOTO_SHARE_LINK));
      gfx->println(F("[HTTPS] begin..."));
      Serial.println(F("[HTTPS] begin..."));
      https.begin(*client, F(GOOGLE_PHOTO_SHARE_LINK));
      https.setTimeout(HTTP_TIMEOUT);

      gfx->setTextColor(YELLOW);
      gfx->println(F("[HTTPS] GET..."));
      Serial.println(F("[HTTPS] GET..."));
      int httpCode = https.GET();

      gfx->setTextColor(GREENYELLOW);
      gfx->print(F("[HTTPS] return code: "));
      Serial.print(F("[HTTPS] return code: "));
      gfx->println(httpCode);
      Serial.println(httpCode);

      // redirect
      if (httpCode == HTTP_CODE_FOUND)
      {
        char redirectUrl[256];
        strcpy(redirectUrl, https.header((size_t)0).c_str());
        https.end();
        gfx->setTextColor(GREEN);
        gfx->print(F("redirectUrl: "));
        Serial.print(F("redirectUrl: "));
        gfx->println(redirectUrl);
        Serial.println(redirectUrl);

        gfx->setTextColor(BLUE);
        gfx->println(F("[HTTPS] begin..."));
        Serial.println(F("[HTTPS] begin..."));
        https.begin(*client, redirectUrl);
        https.setTimeout(HTTP_TIMEOUT);

        gfx->setTextColor(MAGENTA);
        gfx->println(F("[HTTPS] GET..."));
        Serial.println(F("[HTTPS] GET..."));
        httpCode = https.GET();

        gfx->setTextColor(RED);
        gfx->println(F("[HTTPS] GET... code: "));
        Serial.println(F("[HTTPS] GET... code: "));
        gfx->println(httpCode);
        Serial.println(httpCode);
      }

      if (httpCode != HTTP_CODE_OK)
      {
        gfx->setTextColor(YELLOW);
        gfx->print(F("[HTTPS] GET... failed, error: "));
        Serial.print(F("[HTTPS] GET... failed, error: "));
        gfx->println(https.errorToString(httpCode));
        Serial.println(https.errorToString(httpCode));
        delay(9000); // don't repeat the wrong thing too fast
        // return;
      }
      else
      {
        // HTTP header has been send and Server response header has been handled

        //find photo ID leading pattern: ",["https://lh3.googleusercontent.com/
        photoCount = 0;
        int wait_count = 0;
        bool foundStartingPoint = false;
        WiFiClient *stream = https.getStreamPtr();
        while ((photoCount < PHOTO_LIMIT) && (wait_count < 5))
        {
          if (!stream->available())
          {
            gfx->setTextColor(YELLOW);
            gfx->println(F("Wait stream->available()"));
            Serial.println(F("Wait stream->available()"));
            ++wait_count;
            delay(200);
          }
          else
          {
            if (!foundStartingPoint)
            {
              // hack
              Serial.println(F("finding seek pattern: " SEEK_PATTERN));
              gfx->println(F("finding seek pattern: " SEEK_PATTERN));
              if (stream->find(SEEK_PATTERN))
              {
                Serial.println(F("found seek pattern: " SEEK_PATTERN));
                gfx->println(F("found seek pattern: " SEEK_PATTERN));
                foundStartingPoint = true;
              }
            }
            else
            {
              if (stream->find(SEARCH_PATTERN))
              {
                int i = -1;
                char c = stream->read();
                while (c != '\"')
                {
                  photoIdList[photoCount][++i] = c;
                  c = stream->read();
                }
                photoIdList[photoCount][++i] = 0; // zero tail
                Serial.println(photoIdList[photoCount]);
                gfx->println(photoIdList[photoCount]);
                ++photoCount;
              }
            }
          }

#if defined(ESP32)
          // notify WDT still working
          feedLoopWDT();
#elif defined(ESP8266)
          yield();
#endif
        }
        Serial.print(photoCount);
        gfx->print(photoCount);
        Serial.println(F(" photo ID added."));
        gfx->println(F(" photo ID added."));
      }
      https.end();
    }

    if (photoCount)
    {
      char photoUrl[256];
      // UNCOMMENT FOR DEBUG PHOTO LIST
      // for (int i = 0; i < photoCount; i++)
      // {
      //   sprintf(photoUrl, PHOTO_URL_TEMPLATE, photoIdList[i], w, h);
      //   Serial.printf("%d: %s\n", i, photoUrl);
      // }

      // setup url query value with LCD dimension
      int randomIdx = random(photoCount);
      sprintf(photoUrl, PHOTO_URL_TEMPLATE, photoIdList[randomIdx], w, h);
      Serial.print(F("Random selected photo #"));
      Serial.print(randomIdx);
      Serial.print(':');
      Serial.println(photoUrl);

      Serial.println(F("[HTTP] begin..."));
      photoHttps.begin(*client, photoUrl);
      photoHttps.setTimeout(HTTP_TIMEOUT);
      Serial.println(F("[HTTP] GET..."));
      int httpCode = photoHttps.GET();

      Serial.print(F("[HTTP] GET... code: "));
      Serial.println(httpCode);
      // HTTP header has been send and Server response header has been handled
      if (httpCode != HTTP_CODE_OK)
      {
        Serial.print(F("[HTTP] GET... failed, error: "));
        Serial.println(photoHttps.errorToString(httpCode));
        delay(9000); // don't repeat the wrong thing too fast
      }
      else
      {
        // get lenght of document (is -1 when Server sends no Content-Length header)
        len = photoHttps.getSize();
        Serial.print(F("[HTTP] size: "));
        Serial.println(len);

        if (len <= 0)
        {
          Serial.print(F("[HTTP] Unknow content size: "));
          Serial.println(len);
        }
        else
        {
          // get tcp stream
          photoHttpsStream = photoHttps.getStreamPtr();

          if (photoBuf)
          {
            // JPG decode option 1: buffer_reader, use much more memory but faster
            size_t r = 0;
            while (r < len)
            {
              r += photoHttpsStream->readBytes(photoBuf + r, (len - r));
              Serial.print(F("Photo buffer read: "));
              Serial.print(r);
              Serial.print('/');
              Serial.println(len);
            }
            esp_jpg_decode(len, JPG_SCALE_NONE, buffer_reader, tft_writer, NULL /* arg */);
          }
          else
          {
            // JPG decode option 2: http_stream_reader, decode on the fly but slower
            esp_jpg_decode(len, JPG_SCALE_NONE, http_stream_reader, tft_writer, NULL /* arg */);
          }
        }
      }
      photoHttps.end();
    }

    // overlay current time on the photo
    printTime();
  }

#if defined(ESP32)
  // notify WDT still working
  feedLoopWDT();
#elif defined(ESP8266)
  yield();
#endif
}

static size_t http_stream_reader(void *arg, size_t index, uint8_t *buf, size_t len)
{
  if (buf)
  {
    // Serial.printf("[HTTP] read: %d\n", len);
    size_t a = photoHttpsStream->available();
    size_t r = 0;
    while (r < len)
    {
      r += photoHttpsStream->readBytes(buf + r, min((len - r), a));
      delay(50);
    }

    return r;
  }
  else
  {
    // Serial.printf("[HTTP] skip: %d\n", len);
    int l = len;
    while (l--)
    {
      photoHttpsStream->read();
    }
    return len;
  }
}

static size_t buffer_reader(void *arg, size_t index, uint8_t *buf, size_t len)
{
  if (buf)
  {
    memcpy(buf, photoBuf + index, len);
  }
  return len;
}

static bool tft_writer(void *arg, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t *data)
{
  if (data)
  {
    // Serial.printf("%d, %d, %d, %d\n", x, y, w, h);
    gfx->draw24bitRGBBitmap(x, y, data, w, h);
  }

#if defined(ESP32)
  // notify WDT still working
  feedLoopWDT();
#elif defined(ESP8266)
  yield();
#endif

  return true; // Continue to decompression
}
