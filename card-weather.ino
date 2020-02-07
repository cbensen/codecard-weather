//
// Oracle code Card Weather Station
// a weather station with the current weather conditions and forecast for your area.
//
// Ported to the Oracle Code Card from the Adafruit's tutorial by Massimo Castelli.
//
// The tutorial is in the Adafruit Learning System at https://learn.adafruit.com/epaper-weather-station/
// Original code nuder MIT License, Adafruit Industries, author Dan Cogliano
//
// 1. Install the Code Card firmware
// 2. Use the serial programmer type the following:
//    ssid=<your wifi network ssid"
//    password=<your wifi network password"
//    fingerprinta1=<the key provided by open weather>
//    fingerprinta2=<the city you want to get the weather for>
// 3. Upload this sketch

#include "time.h"

#include <ESP8266WiFi.h>
// #include <pgmspace.h>
#include <EEPROM.h>
#include <GxEPD2_BW.h>        // Download/Clone and put in Arduino Library https://github.com/ZinggJM/GxEPD2

#include <ArduinoJson.h>        //https://github.com/bblanchon/ArduinoJson

#include "secrets.h"
#include "OpenWeatherMap.h"

#include "Fonts/meteocons48pt7b.h"
#include "Fonts/meteocons24pt7b.h"
#include "Fonts/meteocons20pt7b.h"
#include "Fonts/meteocons16pt7b.h"

#include "Fonts/moon_phases20pt7b.h"
#include "Fonts/moon_phases36pt7b.h"

#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

#define BAUD_SPEED 74880
#define WAKE_PIN 16
// Button pins
#define BUTTON1_PIN 10   //10
#define BUTTON2_PIN 12    //12
int button;

const int maxValue = 200;
const int eepromSize = 4096;

String ssid     = WIFI_SSID;
String password = WIFI_PASSWORD;
String key = OWM_KEY;
String location = OWM_LOCATION;


GxEPD2_BW<GxEPD2_270, GxEPD2_270::HEIGHT> gfx(GxEPD2_270(/*CS=D8*/ 2, /*DC=D3*/ 0, /*RST=D4*/ 4, /*BUSY=D2*/ 5)); // 2.7" b/w 264x176

AirliftOpenWeatherMap owclient(&Serial);
OpenWeatherMapCurrentData owcdata;
OpenWeatherMapForecastData owfdata[3];

  const char *moonphasenames[29] = {
    "New Moon",
    "Waxing Crescent",
    "Waxing Crescent",
    "Waxing Crescent",
    "Waxing Crescent",
    "Waxing Crescent",
    "Waxing Crescent",
    "Quarter",
    "Waxing Gibbous",
    "Waxing Gibbous",
    "Waxing Gibbous",
    "Waxing Gibbous",
    "Waxing Gibbous",
    "Waxing Gibbous",
    "Full Moon",
    "Waning Gibbous",
    "Waning Gibbous",
    "Waning Gibbous",
    "Waning Gibbous",
    "Waning Gibbous",
    "Waning Gibbous",
    "Last Quarter",
    "Waning Crescent",
    "Waning Crescent",
    "Waning Crescent",
    "Waning Crescent",
    "Waning Crescent",
    "Waning Crescent",
    "Waning Crescent"
};

/*-------------------------------------------------------------------------*/
int8_t readButtons(void) {

  int btn1State = LOW;
  int btn2State = LOW;
  unsigned long startTime;
  unsigned long currentTime;

  btn1State = digitalRead(BUTTON1_PIN);
  btn2State = digitalRead(BUTTON2_PIN);
  startTime = millis();
  delay(100);

  while (digitalRead(BUTTON1_PIN) == HIGH || digitalRead(BUTTON2_PIN) == HIGH){
    startTime++;
    if (startTime > (100000 * 8)) { break; }
  }
  
  if (btn1State == HIGH && btn2State == HIGH){
    Serial.println("both buttons pressed");
    return 5;
  }
  
  if (startTime < 250000) {
    if (btn1State == HIGH){
      Serial.println("a short press");
      return 1;
    }else if (btn2State == HIGH){
      Serial.println("b short press");
      return 2;
    }
  }else  {
    if (btn1State == HIGH){
      Serial.println("a long press");
      return 3;
    }else if (btn2State == HIGH){
      Serial.println("b long press");
      return 4;
    }
  }
}

/*-------------------------------------------------------------------------*/
void shutdown() {
  gfx.powerOff();
  long unsigned int ii = ESP.deepSleepMax()/1000/1000;
  Serial.println(ii);
  Serial.println("Shutting down...");
  Serial.flush();

  digitalWrite(WAKE_PIN, LOW);
  // ESP.deepSleepInstant((10*1000*1000), WAKE_RF_DEFAULT); // Will not wake up in the Oracle Code Card w/o HW hacking
  // Serial.println("This should never get printed");
}

/*-------------------------------------------------------------------------*/
bool wifi_connect() {
  Serial.println("Connecting to WiFi... ");  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("Connecting to '");
  Serial.print(ssid);
  Serial.print("' ");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    if((millis() - start) > 15000) { //try to connect for 15 seconds.
      Serial.println();
      Serial.print("Could not connect to '");
      Serial.print(ssid);
      Serial.println("'. Please check your ssid and password values.");
      return false;
    }
  }
/*
  WiFi.mode(WIFI_STA);
  if(WiFi.begin(WIFI_SSID, WIFI_PASSWORD) == WL_CONNECT_FAILED)
  {
    Serial.println("WiFi connection failed!");
    displayError("WiFi connection failed!");
    return false;
  }

  int wifitimeout = 15;
  int wifistatus; 
  while ((wifistatus = WiFi.status()) != WL_CONNECTED && wifitimeout > 0) {
    delay(1000);
    Serial.print(".");
    wifitimeout--;
  }
  if(wifitimeout == 0)
  {
    Serial.println("WiFi connection timeout with error " + String(wifistatus));
    displayError("WiFi connection timeout with error " + String(wifistatus));

    return false;
  }
  */

  Serial.println("Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); 
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  return true;
}


/*-------------------------------------------------------------------------*/
bool connectWifi() {
  if (WiFi.status() == WL_CONNECTED) WiFi.disconnect(true);
  return wifi_connect();
}

/*-------------------------------------------------------------------------*/
void disconnectWifi() {
  WiFi.disconnect(true);
  Serial.println("Wifi disconnected.");
}

/*-------------------------------------------------------------------------*/
void restartWifi() {
  Serial.println("Restarting...");
  WiFi.disconnect(true);
  wifi_connect();
}

/*-------------------------------------------------------------------------*/
void wget(String &url, int port, char *buff)
{
  int pos1 = url.indexOf("/",0);
  int pos2 = url.indexOf("/",8);
  String host = url.substring(pos1+2,pos2);
  String path = url.substring(pos2);
  Serial.println("to wget(" + host + "," + path + "," + port + ")");
  wget(host, path, port, buff);
}

/*-------------------------------------------------------------------------*/
void wget(String &host, String &path, int port, char *buff)
{ 
  //WiFiSSLClient client;
  WiFiClient client;

  client.stop();
  if (client.connect(host.c_str(), port)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    client.println(String("GET ") + path + String(" HTTP/1.0"));
    client.println("Host: " + host);
    client.println("Connection: close");
    client.println();

    uint32_t bytes = 0;
    int capturepos = 0;
    bool capture = false;
    int linelength = 0;
    char lastc = '\0';

    while (client.connected() || client.available())
    {
      if (client.available())
      {
        String line = client.readStringUntil('\n');
        //Serial.print("Got line: "); Serial.println(line);

// get content lenght
        if (line.startsWith("Content-Length:")) {
            capturepos = (line.substring(16)).toInt();
            //Serial.print("content: "); Serial.println(capturepos);
        }
        else if (line.startsWith("{\"co")) {
// get json content
          capturepos++; // empirico, non capisco xche' ma cosi non taglia l'ultimo carattere... boh?!?!
          line.getBytes( (unsigned char*)buff, capturepos );
          //Serial.print("got json ending with <"); Serial.print(buff[capturepos-3]);Serial.print(buff[capturepos-2]); Serial.print(buff[capturepos-1]); Serial.println(">");
        }
  
        // if the server's disconnected, stop the client:
        if (!client.connected()) {
  
          Serial.println("disconnecting from server.");
          client.stop();
          buff[capturepos] = '\0';
          Serial.println("captured " + String(capturepos) + " bytes");
          break;
        }
      } 
    }
  }
  else
  {
    Serial.println("problem connecting to " + host + ":" + String(port));
    buff[0] = '\0';
  }
}

/*-------------------------------------------------------------------------*/
int getStringLength(String s)
{
  int16_t  x = 0, y = 0;
  uint16_t w, h;
  gfx.getTextBounds(s, 0, 0, &x, &y, &w, &h);
  return w + x;
}

/*-------------------------------------------------------------------------
return value is percent of moon cycle ( from 0.0 to 0.999999), i.e.:

0.0: New Moon
0.125: Waxing Crescent Moon
0.25: Quarter Moon
0.375: Waxing Gibbous Moon
0.5: Full Moon
0.625: Waning Gibbous Moon
0.75: Last Quarter Moon
0.875: Waning Crescent Moon

*/
float getMoonPhase(time_t tdate)
{

  time_t newmoonref = 1263539460; //known new moon date (2010-01-15 07:11)
  // moon phase is 29.5305882 days, which is 2551442.82048 seconds
  float phase = abs( tdate - newmoonref) / (double)2551442.82048;
  phase -= (int)phase; // leave only the remainder
  if(newmoonref > tdate)
  phase = 1 - phase;
  return phase;
}

/*-------------------------------------------------------------------------*/
void displayError(String str)
{
    // show error on display
    Serial.println(str);

    gfx.setTextColor(GxEPD_BLACK);
    // gfx.powerUp();
    gfx.fillScreen(GxEPD_WHITE);
    gfx.setTextWrap(true);
    gfx.setCursor(10,60);
    gfx.setFont(&FreeSans9pt7b);
    gfx.print(str);
    gfx.display();

}

/*-------------------------------------------------------------------------*/
void displayHeading(OpenWeatherMapCurrentData &owcdata)
{

  time_t local = owcdata.observationTime + owcdata.timezone;
  struct tm *timeinfo = gmtime(&local);
  char datestr[80];
  // date
  //strftime(datestr,80,"%a, %d %b %Y",timeinfo);
  strftime(datestr,80,"%a, %b %d",timeinfo);
  gfx.setFont(&FreeSans18pt7b);
  gfx.setCursor((gfx.width()-getStringLength(datestr))/2,30);
  gfx.print(datestr);
  
  // city
  strftime(datestr,80,"%A",timeinfo);
  gfx.setFont(&FreeSansBold12pt7b);
  gfx.setCursor((gfx.width()-getStringLength(owcdata.cityName))/2,60);
  gfx.print(owcdata.cityName);
}

/*-------------------------------------------------------------------------*/
void displayForecastDays(OpenWeatherMapCurrentData &owcdata, OpenWeatherMapForecastData owfdata[], int count = 3)
{
  for(int i=0; i < count; i++)
  {
    // day

    time_t local = owfdata[i].observationTime + owcdata.timezone;
    struct tm *timeinfo = gmtime(&local);
    char strbuff[80];
    strftime(strbuff,80,"%I",timeinfo);
    String datestr = String(atoi(strbuff));
    strftime(strbuff,80,"%p",timeinfo);
    // convert AM/PM to lowercase
    strbuff[0] = tolower(strbuff[0]);
    strbuff[1] = tolower(strbuff[1]);
    datestr = datestr + " " + String(strbuff);
    gfx.setFont(&FreeSans9pt7b);
    gfx.setCursor(i*gfx.width()/3 + (gfx.width()/3-getStringLength(datestr))/2,94);
    gfx.print(datestr);
    
    // weather icon
    String wicon = owclient.getMeteoconIcon(owfdata[i].icon);
    gfx.setFont(&meteocons20pt7b);
    gfx.setCursor(i*gfx.width()/3 + (gfx.width()/3-getStringLength(wicon))/2,134);
    gfx.print(wicon);
  
    // weather main description
    gfx.setFont(&FreeSans9pt7b);
    gfx.setCursor(i*gfx.width()/3 + (gfx.width()/3-getStringLength(owfdata[i].main))/2,154);
    gfx.print(owfdata[i].main);

    // temperature
    int itemp = (int)(owfdata[i].temp + .5);
    int color = GxEPD_BLACK;

    gfx.setTextColor(color);
    gfx.setFont(&FreeSans9pt7b);
    gfx.setCursor(i*gfx.width()/3 + (gfx.width()/3-getStringLength(String(itemp)))/2,172);
    gfx.print(itemp);
    gfx.drawCircle(i*gfx.width()/3 + (gfx.width()/3-getStringLength(String(itemp)))/2 + getStringLength(String(itemp)) + 6,163,3,color);
    gfx.drawCircle(i*gfx.width()/3 + (gfx.width()/3-getStringLength(String(itemp)))/2 + getStringLength(String(itemp)) + 6,163,2,color); 
    gfx.setTextColor(GxEPD_BLACK);   
  }  
}

/*-------------------------------------------------------------------------*/
void displayForecast(OpenWeatherMapCurrentData &owcdata, OpenWeatherMapForecastData owfdata[], int count = 3)
{
  // gfx.powerUp();
  gfx.fillScreen(GxEPD_WHITE); 

  gfx.setTextColor(GxEPD_BLACK);
  displayHeading(owcdata);

  displayForecastDays(owcdata, owfdata, count);
  gfx.display();
  gfx.powerOff();
}

/*-------------------------------------------------------------------------*/
void displayAllWeather(OpenWeatherMapCurrentData &owcdata, OpenWeatherMapForecastData owfdata[], int count = 3)
{
  // gfx.powerUp();
  gfx.fillScreen(GxEPD_WHITE); 

  gfx.setTextColor(GxEPD_BLACK);

  // date string
  time_t local = owcdata.observationTime + owcdata.timezone;
  struct tm *timeinfo = gmtime(&local);
  char datestr[80];
  // date
  //strftime(datestr,80,"%a, %d %b %Y",timeinfo);
  strftime(datestr,80,"%a, %b %d %Y %H:%M",timeinfo);
  gfx.setFont(&FreeSans9pt7b);
  gfx.setCursor((gfx.width()-getStringLength(datestr))/2,14);
  gfx.print(datestr);
  
  // weather icon
  String wicon = owclient.getMeteoconIcon(owcdata.icon);
  gfx.setFont(&meteocons24pt7b);
  gfx.setCursor((gfx.width()/3-getStringLength(wicon))/2,56);
  gfx.print(wicon);

  // weather main description
  gfx.setFont(&FreeSans9pt7b);
  gfx.setCursor((gfx.width()/3-getStringLength(owcdata.main))/2,72);
  gfx.print(owcdata.main);

  // temperature
  gfx.setFont(&FreeSansBold24pt7b);
  int itemp = owcdata.temp + .5;
  int color = GxEPD_BLACK;

  gfx.setTextColor(color);
  gfx.setCursor(gfx.width()/3 + (gfx.width()/3-getStringLength(String(itemp)))/2,58);
  gfx.print(itemp);
  gfx.setTextColor(GxEPD_BLACK);

  // draw temperature degree as a circle (not available as font character
  gfx.drawCircle(gfx.width()/3 + (gfx.width()/3 + getStringLength(String(itemp)))/2 + 8, 58-30,4,color);
  gfx.drawCircle(gfx.width()/3 + (gfx.width()/3 + getStringLength(String(itemp)))/2 + 8, 58-30,3,color);

  // draw moon
  // draw Moon Phase
  float moonphase = getMoonPhase(owcdata.observationTime);
  int moonage = 29.5305882 * moonphase;
  //Serial.println("moon age: " + String(moonage));
  // convert to appropriate icon
  String moonstr = String((char)((int)'A' + (int)(moonage*25./30)));
  gfx.setFont(&moon_phases20pt7b);
  // font lines look a little thin at this size, drawing it a few times to thicken the lines
  gfx.setCursor(2*gfx.width()/3 + (gfx.width()/3-getStringLength(moonstr))/2,56);
  gfx.print(moonstr);  
  gfx.setCursor(2*gfx.width()/3 + (gfx.width()/3-getStringLength(moonstr))/2+1,56);
  gfx.print(moonstr);  
  gfx.setCursor(2*gfx.width()/3 + (gfx.width()/3-getStringLength(moonstr))/2,56-1);
  gfx.print(moonstr);  

  // draw moon phase name
  int currentphase = moonphase * 28. + .5;
  gfx.setFont(); // system font (smallest available)
  gfx.setCursor(2*gfx.width()/3 + max(0,(gfx.width()/3 - getStringLength(moonphasenames[currentphase]))/2),62);
  gfx.print(moonphasenames[currentphase]);


  displayForecastDays(owcdata, owfdata, count);
  gfx.display();
  gfx.powerOff();  
}

/*-------------------------------------------------------------------------*/
void displayCurrentConditions(OpenWeatherMapCurrentData &owcdata)
{
  // gfx.powerUp();
  gfx.fillScreen(GxEPD_WHITE);

  gfx.setTextColor(GxEPD_BLACK);
  displayHeading(owcdata);

  // weather icon
  String wicon = owclient.getMeteoconIcon(owcdata.icon);
  gfx.setFont(&meteocons48pt7b);
  gfx.setCursor((gfx.width()/2-getStringLength(wicon))/2,156);
  gfx.print(wicon);

  // weather main description
  gfx.setFont(&FreeSans9pt7b);
  gfx.setCursor(gfx.width()/2 + (gfx.width()/2-getStringLength(owcdata.main))/2,160);
  gfx.print(owcdata.main);

  // temperature
  gfx.setFont(&FreeSansBold24pt7b);
  int itemp = owcdata.temp + .5;
  int color = GxEPD_BLACK;

  gfx.setTextColor(color);
  gfx.setCursor(gfx.width()/2 + (gfx.width()/2-getStringLength(String(itemp)))/2,130);
  gfx.print(itemp);
  gfx.setTextColor(GxEPD_BLACK);
  
  // draw temperature degree as a circle (not available as font character
  gfx.drawCircle(gfx.width()/2 + (gfx.width()/2 + getStringLength(String(itemp)))/2 + 10, 130-26,4,color);
  gfx.drawCircle(gfx.width()/2 + (gfx.width()/2 + getStringLength(String(itemp)))/2 + 10, 130-26,3,color);
  
  gfx.display();
  gfx.powerOff();
}

/*-------------------------------------------------------------------------*/
void displaySunMoon(OpenWeatherMapCurrentData &owcdata)
{
  
  // gfx.powerUp();
  gfx.fillScreen(GxEPD_WHITE);
 
  gfx.setTextColor(GxEPD_BLACK);
  displayHeading(owcdata);

  // draw Moon Phase
  float moonphase = getMoonPhase(owcdata.observationTime);
  int moonage = 29.5305882 * moonphase;
  // convert to appropriate icon
  String moonstr = String((char)((int)'A' + (int)(moonage*25./30)));
  gfx.setFont(&moon_phases36pt7b);
  gfx.setCursor((gfx.width()/3-getStringLength(moonstr))/2,140);
  gfx.print(moonstr);

  // draw moon phase name
  int currentphase = moonphase * 28. + .5;
  gfx.setFont(&FreeSans9pt7b);
  gfx.setCursor(gfx.width()/3 + max(0,(gfx.width()*2/3 - getStringLength(moonphasenames[currentphase]))/2),110);
  gfx.print(moonphasenames[currentphase]);

  // draw sunrise/sunset

  // sunrise/sunset times
  // sunrise

  time_t local = owcdata.sunrise + owcdata.timezone + 30;  // round to nearest minute
  struct tm *timeinfo = gmtime(&local);
  char strbuff[80];
  strftime(strbuff,80,"%I",timeinfo);
  String datestr = String(atoi(strbuff));
  strftime(strbuff,80,":%M %p",timeinfo);
  datestr = datestr + String(strbuff) + " - ";
  // sunset
  local = owcdata.sunset + owcdata.timezone + 30; // round to nearest minute
  timeinfo = gmtime(&local);
  strftime(strbuff,80,"%I",timeinfo);
  datestr = datestr + String(atoi(strbuff));
  strftime(strbuff,80,":%M %p",timeinfo);
  datestr = datestr + String(strbuff);

  gfx.setFont(&FreeSans9pt7b);
  int datestrlen = getStringLength(datestr);
  int xpos = (gfx.width() - datestrlen)/2;
  gfx.setCursor(xpos,166);
  gfx.print(datestr);

  // draw sunrise icon
  // sun icon is "B"
  String wicon = "B";
  gfx.setFont(&meteocons16pt7b);
  gfx.setCursor(xpos - getStringLength(wicon) - 12,174);
  gfx.print(wicon);

  // draw sunset icon
  // sunset icon is "A"
  wicon = "A";
  gfx.setCursor(xpos + datestrlen + 12,174);
  gfx.print(wicon);

  gfx.display();
  gfx.powerOff();

}

/*-------------------------------------------------------------------------*/
void setup() {
  pinMode(WAKE_PIN, OUTPUT);
  digitalWrite(WAKE_PIN, HIGH); //immediately set wake pin to HIGH to keep the chip enabled
      
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  button = readButtons();

  Serial.begin(BAUD_SPEED);
  gfx.init();
  Serial.println("ePaper display initialized");
  gfx.setRotation(3);
  gfx.setTextWrap(false);

  EEPROM.begin(eepromSize);
  char arrayToStore[100];
  ssid = EEPROM.get(0*maxValue, arrayToStore);
  password = EEPROM.get(1*maxValue, arrayToStore);
  key = EEPROM.get(6*maxValue, arrayToStore);
  location = EEPROM.get(7*maxValue, arrayToStore);

  Serial.println("ssid = " + ssid); //ssid
  Serial.println("password = " + password); //password
  Serial.println("key = " + key); //fingerprinta1
  Serial.println("location = " + location); //fingerprinta2
}

/*-------------------------------------------------------------------------*/
void loop() {

  static char data[4000];
  static bool firsttime = true;

  displayError("Fetching new weather data...");

// here if the card had an RTC I would check for going online only after an UPDATE_INTERVAL; since I've got no way of getting a time w/o going online, I can fetch the data as well.
  if (firsttime)
  {
    Serial.println("getting weather data");
    firsttime = false;

    int retry = 6;

    while(!connectWifi())
    {
      delay(5000);
      retry--;
      if(retry < 0)
      {
        displayError("Can not connect to WiFi, press a button to restart");
        shutdown();  
      }
    }
    String urlc = owclient.buildUrlCurrent(key, location);
    retry = 6;
    do
    {
      retry--;
      wget(urlc,80,data);
      if(strlen(data) == 0 && retry < 0)
      {
        displayError("Can not get weather data, press a button to restart");
        shutdown();      
      }
    }
    while(strlen(data) == 0);
    //Serial.println("data wt retrieved:");
    //Serial.println(data);
    retry = 6;
    while(!owclient.updateCurrent(owcdata,data))
    {
      retry--;
      if(retry < 0)
      {
        displayError(owclient.getError());
        shutdown();
      }
      delay(5000);
    }
  
    String urlf = owclient.buildUrlForecast(key, location);
    wget(urlf,80,data);
    //Serial.println("data fc retrieved:");
    //Serial.println(data);
    if(!owclient.updateForecast(owfdata[0],data,0))
    {
      displayError(owclient.getError());
      shutdown();
    }
    if(!owclient.updateForecast(owfdata[1],data,2))
    {
      displayError(owclient.getError());
      shutdown();
    }
    if(!owclient.updateForecast(owfdata[2],data,4))
    {
      displayError(owclient.getError());
      shutdown();
    }
    
    disconnectWifi();
    
  } else {
    // here, if the card had an RTC, I would fetch the data from flash instead of going online, alas, it remains empty.
  }

  Serial.print("Button "); Serial.print(button); Serial.println(" pressed");
  switch(button)
  {
    case 0:   // default action on hard reset
    case 1:   // button a short   
      displayAllWeather(owcdata,owfdata,3);
      break;
    case 2:   // button b short
      displayCurrentConditions(owcdata);
      break;
    case 3:   // button a long
      displaySunMoon(owcdata);
      break;
    case 4:   // button b long
      displayForecast(owcdata,owfdata,3);
      break;
  }

  shutdown();
}
