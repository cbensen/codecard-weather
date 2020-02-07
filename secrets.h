#pragma once

// secrets.h
// Define your WIFI and OpenWeatherMap API key and location in this file

#define WIFI_SSID     ""
#define WIFI_PASSWORD ""

#define OWM_KEY ""
#define OWM_LOCATION ""
//example
//#define OWM_LOCATION "New York,US"

// update the weather at this interval, in minutes
#define UPDATE_INTERVAL 30

// Set to true to show temperatures in Celsius, false for Fahrenheit
#define OWM_METRIC true

/*
Arabic - ar, Bulgarian - bg, Catalan - ca, Czech - cz, German - de, Greek - el,
English - en, Persian (Farsi) - fa, Finnish - fi, French - fr, Galician - gl,
Croatian - hr, Hungarian - hu, Italian - it, Japanese - ja, Korean - kr,
Latvian - la, Lithuanian - lt, Macedonian - mk, Dutch - nl, Polish - pl,
Portuguese - pt, Romanian - ro, Russian - ru, Swedish - se, Slovak - sk,
Slovenian - sl, Spanish - es, Turkish - tr, Ukrainian - ua, Vietnamese - vi,
Chinese Simplified - zh_cn, Chinese Traditional - zh_tw.
*/
#define OWM_LANGUAGE "en"
