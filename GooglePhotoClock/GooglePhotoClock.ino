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
#define PHOTO_LIMIT 10                                     // read first 10 photos to the list, ESP32 can add more
#define PHOTO_ID_SIZE 159                                  // the photo ID should be 158 charaters long and then add a zero-tail
#define HTTP_TIMEOUT 60000                                 // in ms, wait a while for server processing
#define HTTP_WAIT_COUNT 10                                 // number of times wait for next HTTP packet trunk
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
 * 
 * Arduino_GFX try to find the settings depends on selected board in Arduino IDE
 * Or you can define the display dev kit not in the board list
 * Defalult pin list for non display dev kit:
 * Arduino Nano, Micro and more: TFT_CS:  9, TFT_DC:  8, TFT_RST:  7, TFT_BL:  6
 * ESP32 various dev board     : TFT_CS:  5, TFT_DC: 27, TFT_RST: 33, TFT_BL: 22
 * ESP8266 various dev board   : TFT_CS: 15, TFT_DC:  4, TFT_RST:  2, TFT_BL:  5
 * Raspberry Pi Pico dev board : TFT_CS: 17, TFT_DC: 27, TFT_RST: 26, TFT_BL: 28
 * RTL872x various dev board   : TFT_CS: 18, TFT_DC: 17, TFT_RST:  2, TFT_BL: 23
 * Seeeduino XIAO dev board    : TFT_CS:  3, TFT_DC:  2, TFT_RST:  1, TFT_BL:  0
 * Teensy 4.1 dev board        : TFT_CS: 39, TFT_DC: 41, TFT_RST: 40, TFT_BL: 22
 ******************************************************************************/
#include <Arduino_GFX_Library.h>

/* More dev device declaration: https://github.com/moononournation/Arduino_GFX/wiki/Dev-Device-Declaration */
#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *gfx = create_default_Arduino_GFX();
#else /* !defined(DISPLAY_DEV_KIT) */

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus *bus = create_default_Arduino_DataBus();

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
Arduino_GFX *gfx = new Arduino_ST7796(bus, TFT_RST, 0 /* rotation */, false /* IPS */);

#endif /* !defined(DISPLAY_DEV_KIT) */
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

/* file system */
#include <LittleFS.h>

/* time library */
#include <time.h>
static time_t now;

/* HTTP */
const char *headerkeys[] = {"Location"};

/* Google photo */
char photoIdList[PHOTO_LIMIT][PHOTO_ID_SIZE];
uint8_t *photoBuf;

/* variables */
static int w, h, timeX, timeY, len, offset, photoCount;
static uint8_t textSize;
static unsigned long next_show_millis = 0;
static bool shownPhoto = false;

void ntpGetTime()
{
  // Initialize NTP settings
#if defined(ESP32)
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
#elif defined(ESP8266)
  configTime(TZ, NTP_SERVER);
#endif

  gfx->setTextColor(GREEN);
  gfx->print(F("Waiting for NTP time sync: "));
  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2)
  {
    delay(500);
    Serial.print('.');
    gfx->print('.');
    yield();
    nowSecs = time(nullptr);
  }
  gfx->println(F(" done."));
  Serial.println(F(" done."));
  gfx->setTextColor(BLUE);
  gfx->println(asctime(localtime(&nowSecs)));
  Serial.println(asctime(localtime(&nowSecs)));
}

void printTime()
{
  now = time(nullptr);
  const tm *tm = localtime(&now);
  int hour = tm->tm_hour;
  int min = tm->tm_min;
  // set text size with pixel margin
  gfx->setTextSize(textSize, textSize, 2);
  if (!shownPhoto)
  {
    // print time with black background
    gfx->setCursor(timeX, timeY);
    gfx->setTextColor(WHITE, BLACK);
    gfx->printf("%02d:%02d", hour, min);
  }
  else
  {
    // print time with border
    gfx->setCursor(timeX - 1, timeY - 1);
    gfx->setTextColor(DARKGREY);
    gfx->printf("%02d:%02d", hour, min);
    gfx->setCursor(timeX + 2, timeY + 2);
    gfx->setTextColor(BLACK);
    gfx->printf("%02d:%02d", hour, min);
    gfx->setCursor(timeX, timeY);
    gfx->setTextColor(WHITE);
    gfx->printf("%02d:%02d", hour, min);
  }
}

void setup()
{
  Serial.begin(115200);

  // init display
  gfx->begin();
  gfx->fillScreen(BLACK);
#ifdef TFT_BL
  // turn on display backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
#endif

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
  gfx->setTextColor(ORANGE);
  gfx->print(F("Screen: "));
  Serial.print(F("Screen: "));
  gfx->print(w);
  Serial.print(w);
  gfx->print('x');
  Serial.print('x');
  gfx->println(h);
  Serial.println(h);
  gfx->setTextColor(YELLOW);
  gfx->print(F("Clock text size: "));
  Serial.print(F("Clock text size: "));
  gfx->println(textSize);
  Serial.println(textSize);

  // init WiFi
  gfx->setTextColor(GREENYELLOW);
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

  // init LittleFS
  if (!LittleFS.begin())
  {
    gfx->print(F("LittleFS init failed!"));
    Serial.print(F("LittleFS init failed!"));
  }
  else
  {
    // init HTTPClient
#if defined(ESP32)
    client->setCACert(rootCACertificate);

    // set WDT timeout a little bit longer than HTTP timeout
    esp_task_wdt_init((HTTP_TIMEOUT / 1000) + 1, true);
    enableLoopWDT();
#elif defined(ESP8266)
    File file = LittleFS.open("/certs.ar", "r");
    if (!file)
    {
      gfx->print(F("File \"/certs.ar\" not found!"));
      Serial.print(F("File \"/certs.ar\" not found!"));
    }
    else
    {
      file.close();
      int numCerts = certStore.initCertStore(LittleFS, "/certs.idx", "/certs.ar");
      gfx->setTextColor(MAGENTA);
      gfx->print(F("Number of CA certs read: "));
      Serial.print(F("Number of CA certs read: "));
      gfx->println(numCerts);
      Serial.println(numCerts);
      client = new BearSSL::WiFiClientSecure();
      client->setCertStore(&certStore);
    }
#endif
  }

  // allocate photo file buffer
  photoBuf = (uint8_t *)malloc(w * h * 3 / 5);
  if (!photoBuf)
  {
    Serial.println(F("photoBuf allocate failed!"));
  }

  // print time on black screen before down load image
  printTime();
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
    HTTPClient https;

    if (!photoCount)
    {
      https.collectHeaders(headerkeys, sizeof(headerkeys) / sizeof(char *));

      gfx->setTextSize(1);
      gfx->setCursor(0, 64);
      gfx->setTextColor(RED);
      gfx->println(F(GOOGLE_PHOTO_SHARE_LINK));
      Serial.println(F(GOOGLE_PHOTO_SHARE_LINK));
      gfx->setTextColor(ORANGE);
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
        gfx->print(F("[HTTPS] GET... code: "));
        Serial.print(F("[HTTPS] GET... code: "));
        gfx->println(httpCode);
        Serial.println(httpCode);
      }

      if (httpCode != HTTP_CODE_OK)
      {
        gfx->setTextColor(ORANGE);
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
        stream->setTimeout(10);
        while ((photoCount < PHOTO_LIMIT) && (wait_count < HTTP_WAIT_COUNT))
        {
          if (!stream->available())
          {
            gfx->print('.');
            Serial.println(F("Wait stream->available()"));
            ++wait_count;
            delay(200);
          }
          else
          {
            if (!foundStartingPoint)
            {
              gfx->setTextColor(ORANGE);
              gfx->println(F("finding seek pattern: " SEEK_PATTERN));
              Serial.println(F("finding seek pattern: " SEEK_PATTERN));
              if (stream->find(SEEK_PATTERN))
              {
                gfx->setTextColor(YELLOW);
                gfx->println(F("found seek pattern: " SEEK_PATTERN));
                Serial.println(F("found seek pattern: " SEEK_PATTERN));
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
                gfx->setTextColor(GREENYELLOW);
                gfx->print('*');
                Serial.println(photoIdList[photoCount]);
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
        gfx->setTextColor(GREEN);
        gfx->print(photoCount);
        gfx->println(F(" photo ID added."));
        Serial.print(photoCount);
        Serial.println(F(" photo ID added."));
#if defined(ESP32)
#elif defined(ESP8266)
        // clean up cache files
        char filename[16];
        for (int i = 0; i < photoCount; i++)
        {
          sprintf(filename, "/%d.jpg", i);
          if (File file = LittleFS.open(filename, "r"))
          {
            file.close();
            LittleFS.remove(filename);
          }
        }
#endif
      }
      https.end();
    }
    else // photoCount > 0
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
      File cachePhoto;
      char filename[16];
      sprintf(filename, "/%d.jpg", randomIdx);
      if (!(cachePhoto = LittleFS.open(filename, "r")))
      {
        sprintf(photoUrl, PHOTO_URL_TEMPLATE, photoIdList[randomIdx], w, h);
        Serial.print(F("Random selected photo #"));
        Serial.print(randomIdx);
        Serial.print(':');
        Serial.println(photoUrl);

        Serial.println(F("[HTTP] begin..."));
        https.begin(*client, photoUrl);
        https.setTimeout(HTTP_TIMEOUT);
        Serial.println(F("[HTTP] GET..."));
        int httpCode = https.GET();

        Serial.print(F("[HTTP] GET... code: "));
        Serial.println(httpCode);
        // HTTP header has been send and Server response header has been handled
        if (httpCode != HTTP_CODE_OK)
        {
          Serial.print(F("[HTTP] GET... failed, error: "));
          Serial.println(https.errorToString(httpCode));
          delay(9000); // don't repeat the wrong thing too fast
        }
        else
        {
          // get lenght of document (is -1 when Server sends no Content-Length header)
          len = https.getSize();
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
            WiFiClient *photoHttpsStream = https.getStreamPtr();

            if (photoBuf)
            {
              // JPG decode option 1: buffer_reader, use much more memory but faster
              size_t reads = 0;
              while (reads < len)
              {
                size_t r = photoHttpsStream->readBytes(photoBuf + reads, (len - reads));
                reads += r;
                Serial.print(F("Photo buffer read: "));
                Serial.print(reads);
                Serial.print('/');
                Serial.println(len);
              }
              esp_jpg_decode(len, JPG_SCALE_NONE, buffer_reader, tft_writer, NULL /* arg */);
            }
            else
            {
              // JPG decode option 2: download to cache file
              Serial.print(F("Cache photo: "));
              Serial.println(filename);
              cachePhoto = LittleFS.open(filename, "w");
              if (cachePhoto)
              {
                uint8_t buf[512];
                size_t reads = 0;
                while (reads < len)
                {
                  size_t r = photoHttpsStream->readBytes(buf, min(sizeof(buf), len - reads));
                  reads += r;
                  cachePhoto.write(buf, r);
                  // Serial.print(F("Photo file write: "));
                  // Serial.print(reads);
                  // Serial.print('/');
                  // Serial.println(len);
                }
                cachePhoto.close();

                cachePhoto = LittleFS.open(filename, "r");
              }
              else
              {
                // JPG decode option 3: stream_reader, decode on the fly but slower
                esp_jpg_decode(len, JPG_SCALE_NONE, stream_reader, tft_writer, &photoHttpsStream /* arg */);
              }
            }
          }
        }
        https.end();
      }
      if (cachePhoto)
      {
        Serial.print(F("Load cache photo: "));
        Serial.println(filename);
        esp_jpg_decode(cachePhoto.size(), JPG_SCALE_NONE, stream_reader, tft_writer, &cachePhoto /* arg */);
        cachePhoto.close();
      }
      shownPhoto = true;
      next_show_millis = ((millis() / 60000L) + 1) * 60000L; // next minute
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

static size_t stream_reader(void *arg, size_t index, uint8_t *buf, size_t len)
{
  Stream *s = (Stream *)arg;
  if (buf)
  {
    // Serial.printf("[HTTP] read: %d\n", len);
    size_t a = s->available();
    size_t r = 0;
    while (r < len)
    {
      r += s->readBytes(buf + r, min((len - r), a));
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
      s->read();
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
