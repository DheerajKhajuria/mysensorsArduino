#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <Time.h>
#include <time.h>
#include <sys/time.h>                   
#include <coredecls.h>                 
#include "DHTesp.h"
#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS3231.h>
#include <JsonListener.h>
#include "OpenWeatherMapCurrent.h"
#include <sys/reent.h>
#include "sntp.h"
#include "Font_Data.h"
#include "sys_var_single.h"

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   12
#define DATA_PIN  15
#define CS_PIN    13
#define MAX_DISPLAY 3

RtcDS3231<TwoWire> Rtc(Wire);
DHTesp dht;
WiFiClient client;
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
OpenWeatherMapCurrent OWClient;

String OPEN_WEATHER_MAP_APP_ID = "9b4c8911e2ce469d030399c3a02fa564";
String OPEN_WEATHER_MAP_LOCATION_ID = "1270642";
String OPEN_WEATHER_MAP_LANGUAGE = "en";
boolean IS_METRIC = true;

// =======================================================================
// CHANGE YOUR CONFIG HERE:
// =======================================================================
const char* ssid     = "H-703";     // SSID of local network
const char* password = "dk007DK5";   // Password on network
// =======================================================================

#define SPEED_TIME 75
#define PAUSE_TIME 0

// Global message buffers shared by Wifi and Scrolling functions
#define BUF_SIZE  256
char curMessage[BUF_SIZE];

String date;
timeval cbtime;      // time set in callback
bool cbtime_set = false;
bool dots= false;
bool showClockTime = true;
int ntpRetries = 10;
long dotTime = 0;
long clkTime = 0;
uint8_t display=0;

void time_is_set(void) {
  gettimeofday(&cbtime, NULL);
  cbtime_set = true;
  Serial.println("------------------ settimeofday() was called ------------------");
}

////////////////////////////////////////////////////////
#define TZ              5       // (utc+) TZ in hours
#define DST_MN          30      // BUG?? use 60mn for summer time in some countries

////////////////////////////////////////////////////////

#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)

uint8_t  inFX, outFX;
textEffect_t  effect[] =
{
  PA_PRINT,
  PA_SCROLL_LEFT,
  PA_WIPE,
  PA_SCROLL_UP_LEFT,
  PA_SCROLL_UP,
  PA_OPENING_CURSOR,
  PA_GROW_UP,
  PA_MESH,
  PA_SCROLL_UP_RIGHT,
  PA_BLINDS,
  PA_CLOSING,
  PA_RANDOM,
  PA_GROW_DOWN,
  PA_SCROLL_DOWN_LEFT,
  PA_WIPE_CURSOR,
  PA_DISSOLVE,
  PA_OPENING,
  PA_CLOSING_CURSOR,
  PA_SCROLL_DOWN_RIGHT,
  PA_SCROLL_RIGHT,
  PA_SLICE,
  PA_SCROLL_DOWN,
};

uint8_t PM[] = {8, 124, 20, 124, 96, 16, 112, 16, 112};   
uint8_t AM[] = {8, 124, 20, 28, 96, 16, 112, 16, 112};
uint8_t degreeC[] = { 5, 3, 59, 68, 68, 68 };
uint8_t blinkChar[] = { 1, 80};

uint8_t smallDigit[10][4] =
{
  3, 124, 68, 124,    // 140"
  3, 0, 8, 124,       // 141"
  3, 116, 84, 92,     // 142"
  3, 84, 84, 124,     // 143"
  3, 28, 16, 124,     // 144"
  3, 92, 84, 116,     // 145"
  3, 124, 84, 116,    // 146"
  3, 4, 4, 124,       // 147"
  3, 124, 84, 124,    // 148"
  3, 28, 20, 124,     // 149"
};

// =======================================================================

void setup() {
  Serial.begin(115200);
  configTime(TZ_SEC, DST_SEC,"pool.ntp.org","asia.pool.ntp.org");
  dht.setup(2, DHTesp::DHT11); // Connect DHT sensor to GPIO 17
  P.begin();
  P.setIntensity(3); 
  P.print("Init...");
  P.setInvert(false);
  //P.displaySuspend(false);
  //P.setFont(_sys_var_single);

  P.addChar('~', PM);
  P.addChar('^', AM);
  P.addChar('&', blinkChar);
  P.addChar('#', degreeC);

  for(int i =0 ; i < sizeof(smallDigit); i++)
  {
    P.addChar(140+i, smallDigit[i]); 
  }
  curMessage[0] = '\0';
  P.displayText(curMessage, PA_LEFT, SPEED_TIME, PAUSE_TIME, effect[inFX], effect[outFX]);

  // NTP clock
  settimeofday_cb(time_is_set);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected: "); Serial.println(WiFi.localIP());

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("clock");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  while(!cbtime_set && ntpRetries > 0)
  {
    Serial.println("Waiting for NTP time..");
    --ntpRetries;
    delay(1000);
  }
  Rtc.Begin();

  if (!Rtc.GetIsRunning())
  {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  if (ntpRetries <=0 && Rtc.IsDateTimeValid())
  {
    //set time from RTC.
    Serial.println("NTP sync failed trying RTC!");
    RtcDateTime now = Rtc.GetDateTime();
    printDateTime(now);
    Serial.println();
    time_t rtc = now.Epoch32Time();
    timeval tv = { rtc, 0 };
    timezone tz = { 0 , 0 };
    settimeofday(&tv, &tz);
    sntp_stop();
  }
  else
  {
    time_t ntpSyncTime = time(nullptr);
    Serial.println("NTP sync success setting RTC time!");
    RtcDateTime newRTCTime =  RtcDateTime(ntpSyncTime-(c_Epoch32OfOriginYear));
    Serial.print("RTC time ");
    printDateTime(Rtc.GetDateTime());
    Serial.println("\nNTP Sync time ");
    Serial.println(ctime(&ntpSyncTime));
    if( newRTCTime > Rtc.GetDateTime())
    {
      Serial.println("RTC time updated!");
      Rtc.SetDateTime(newRTCTime);
    }
  }
  // never assume the Rtc was last configured by you, so
  // just clear them to your needed state
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 
  updateOpenWeatherData();
}
time_t now;

void  Blink()
{
  if(showClockTime)
  {
    P._D.setPoint(1,20,dots);
    P._D.setPoint(6,20,dots);
  }
}
void showClock()
{  
  Blink();
  if (P.displayAnimate()) // animates and returns true when an animation is completed
  {
    showClockTime= true;
    now = time(nullptr); 
    struct tm * tm =  localtime(&now);
    int hour=tm->tm_hour ;                       
    if(hour >=13 && hour <=24)
    {
      hour=hour-12;
    }
    if(hour == 0) 
    {
      hour =12;
    }
    if(display==MAX_DISPLAY)
    {
      sprintf(curMessage,"%02d%02d%c", hour, tm->tm_min, tm->tm_hour >= 12 ? '^': '~');  
    }
    else
    {
      int sec = tm->tm_sec ; 
      int secondDigit =(sec%10)+140;
      sec = sec/10;
      int firstDigit =(sec%10)+140;
      sprintf(curMessage,"%02d%02d%c%c", hour, tm->tm_min,firstDigit,secondDigit);  
    }
    P.setTextAlignment(PA_LEFT);
    P.setFont(_sys_var_single);
    P.setPause(PAUSE_TIME);
    P.setSpeed(SPEED_TIME); 
    P.setTextEffect(PA_PRINT, PA_NO_EFFECT);
    P.displayReset();
  }
}
static const char wday_name[][4] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static const char mon_name[][4] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

uint8_t dateFormat=0;
void showDate()
{  
  showClockTime=false;
  P.displayClear();
  now = time(nullptr); 
  struct tm * tm =  localtime(&now);
  dateFormat = (++dateFormat) % 2;
  outFX = (++outFX) % ARRAY_SIZE(effect);
  if (dateFormat)
  { 
    P.setPause(4000);
    P.setFont(_sys_var_single_modified);
    sprintf(curMessage, "%02d.%02d.%02d",tm->tm_mday,tm->tm_mon+1,1900+tm->tm_year-2000) ;
    P.setTextEffect(PA_DISSOLVE,effect[outFX]); 
  }else
  {
    P.setPause(1500);
    P.setFont(_sys_var_single);
    sprintf(curMessage, "%s %02d %s",wday_name[tm->tm_wday], tm->tm_mday, mon_name[tm->tm_mon]);
    P.setTextEffect(PA_SCROLL_LEFT,effect[outFX]); 
  }
  P.displayReset();
}

void showLocalTemp()
{  
  showClockTime=false;
  P.setPause(4000);
  P.displayClear();
  outFX = (++outFX) % ARRAY_SIZE(effect);
  P.setFont(_sys_var_single_modified);
  sprintf(curMessage, "%02.0f%c %02.0f% %", dht.getTemperature(),'#',dht.getHumidity()) ;
  P.setTextEffect(PA_SCROLL_DOWN,effect[outFX]); 
  P.displayReset();
}

OpenWeatherMapCurrentData data;
void updateOpenWeatherData()
{
  OWClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  OWClient.setMetric(IS_METRIC);
  OWClient.updateCurrentById(&data, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
}
uint8_t weatherInfo=0;
void showWeatherInfo()
{  
  showClockTime=false;
  P.setPause(1500);
  P.displayClear();
  P.setFont(_sys_var_single);
  outFX = (++outFX) % ARRAY_SIZE(effect);
  weatherInfo = (++weatherInfo) % 5;
  switch(weatherInfo)
  {
  case 0: 
    { 
      sprintf(curMessage,"%s %s",data.cityName.c_str(),data.main.c_str()); 
      P.setTextEffect(PA_SCROLL_LEFT,effect[outFX]); 
      break;
    }
  case 1:
    {
      sprintf(curMessage,"%0.1f%c",data.temp,'#'); 
      P.setTextEffect(PA_SCROLL_DOWN,effect[outFX]); 
      break;
    }
  case 2:
    {
      sprintf(curMessage,"H %d%c",data.humidity,'%');
      P.setTextEffect(PA_OPENING,effect[outFX]); 
      break;  
    }
  case 3:
    {
      time_t time = TZ_SEC + DST_SEC + data.sunrise;
      struct tm * tm =  localtime(&time);
      sprintf(curMessage, "Sunrise %02d:%02d",tm->tm_hour,tm->tm_min) ;
      P.setTextEffect(PA_SCROLL_LEFT,effect[outFX]); 
      break;    
    }
  case 4: 
    {
      time_t time = TZ_SEC + DST_SEC + data.sunset;
      struct tm * tm =  localtime(&time);
      sprintf(curMessage,"Sunset %02d:%02d",tm->tm_hour,tm->tm_min);
      P.setTextEffect(PA_SCROLL_LEFT,effect[outFX]); 
      break;       
    }
  }
  P.displayReset();
}

void loop()
{
  ArduinoOTA.handle();
  if(millis()-clkTime > 25000)
  {  display >= MAX_DISPLAY ? display = 1: display++;
    P.setTextAlignment(PA_CENTER);
    switch(display)
    { 
      case 1:  showLocalTemp(); updateOpenWeatherData(); break; 
      case 2:  showDate();  break;
      case 3:  showWeatherInfo(); break; 
    }
    clkTime = millis();
  }

  if(millis()-dotTime > 950 )
  { 
    showClockTime ? dots ^=1:dots= false;
    dotTime = millis();
  }
  showClock() ;
}

#define countof(a) (sizeof(a) / sizeof(a[0]))
void printDateTime(const RtcDateTime& dt)
{
  char datestring[20];

  snprintf_P(datestring, 
  countof(datestring),
  PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
  dt.Month(),
  dt.Day(),
  dt.Year(),
  dt.Hour(),
  dt.Minute(),
  dt.Second() );
  Serial.print(datestring);
}
// =======================================================================
