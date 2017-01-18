/**The MIT License (MIT)

  Copyright (c) 2016 by Daniel Eichhorn

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  See more at http://blog.squix.org
*/

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <JsonListener.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

//////////////////////////
#include <WiFiClient.h>
ESP8266WebServer server ( 80 );
//////////////////////////

#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "Wire.h"
#include "WundergroundClient.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"
#include "TimeClient.h"
#include "ThingspeakClient.h"


/***************************
   Begin Settings
 **************************/
// Please read http://blog.squix.org/weatherstation-getting-code-adapting-it
// for setup instructions.
// In addition to the "normal" libraries you also need to install the WifiManager fro tzapu

//#define HOSTNAME "ESP8266-OTA-"

#define HOSTNAME "HavaIstasyonuAP"


// Setup
const int UPDATE_INTERVAL_SECS = 30 * 60; // Update every 30 minutes

// Display Settings
const int I2C_DISPLAY_ADDRESS = 0x3c;
//const int SDA_PIN = 0;
//const int SDC_PIN = 2;

const int SDA_PIN = D2;
const int SDC_PIN = D3;



// TimeClient settings
const float UTC_OFFSET = 5;

// Wunderground Settings
const boolean IS_METRIC = true;
const String WUNDERGRROUND_API_KEY = "54ee3d58d054c441";
const String WUNDERGRROUND_LANGUAGE = "EN";
const String WUNDERGROUND_COUNTRY = "PK";
const String WUNDERGROUND_CITY = "Lahore";

//Thingspeak Settings
const String THINGSPEAK_CHANNEL_ID = "67284";
const String THINGSPEAK_API_READ_KEY = "L2VIW20QVNZJBLAK";

// Initialize the oled display for address 0x3c
// sda-pin=14 and sdc-pin=12
SSD1306Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUi   ui( &display );

/***************************
   End Settings
 **************************/

TimeClient timeClient(UTC_OFFSET);

// Set to false, if you prefere imperial/inches, Fahrenheit
WundergroundClient wunderground(IS_METRIC);

ThingspeakClient thingspeak;

// flag changed in the ticker function every 10 minutes
bool readyForWeatherUpdate = false;

String lastUpdate = "--";

Ticker ticker;

//declaring prototypes
void configModeCallback (WiFiManager *myWiFiManager);
void drawProgress(OLEDDisplay *display, int percentage, String label);
void drawOtaProgress(unsigned int, unsigned int);
void updateData(OLEDDisplay *display);
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawThingspeak(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
void setReadyForWeatherUpdate();
int8_t getWifiQuality();


// Add frames
// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
FrameCallback frames[] = { drawDateTime, drawCurrentWeather, drawForecast, drawThingspeak };
int numberOfFrames = 3;

OverlayCallback overlays[] = { drawHeaderOverlay };
int numberOfOverlays = 1;
/////////////////
/////////////////
/////////////////
//String webPage = "";
String Argument_Name, Clients_Response1, Clients_Response2;

void setup() {

  /////////////////////

  /*
    webPage +=  "<p>";

    webPage += "<center>";

    webPage += "<h1>Online Hava Istasyonu </h1>";

    webPage += "<img src='http://arduinom.org/wp-content/uploads/2016/02/logoL.png'>";
    webPage +=  "<p>";
    //webPage += "<img src='http://arduinom.org/wp-content/uploads/2016/12/oledhava1.jpg'>";
    //webPage +=  "<p>";
    webPage += "<form action='post'><p>Hangi Şehir   <input type='text' name='msg' size=15 autofocus> <input type='submit' value='Değiştir'></form>";
    webPage += "<form action='get'>Ülkeyi Tanımla   <input type='text' name='msg' size=15 autofocus> <input type='submit' value='Değiştir'></form>";

    webPage += "</center>";

  */



  ////////////////////////////

  /////////////////////////
  /////////////////////////
  pinMode (D4, OUTPUT);
  digitalWrite(D4, HIGH);
  // Turn On VCC
  //pinMode(D4, OUTPUT);
  //digitalWrite(D4, HIGH);
  Serial.begin(115200);

  // initialize dispaly

  display.init();

  display.clear();
  display.display();
 // ui.init();
 // display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  display.clear();
  display.drawString(64, 1 , "AP Modda Acildi");
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 0 , "_________________");
  display.drawString(64, 18, "HavaIstasyonuAP");
  display.drawString(64, 20 , "_________________");
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 40, "Agina Baglanarak");
  display.drawString(64, 53, "ADIMLARI TAKIP EDINIZ");
  display.display();


  WiFiManager wifiManager;
  wifiManager.autoConnect("HavaIstasyonuAP");
  // Uncomment for testing wifi manager
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);

  //or use this for auto generated name ESP + ChipID
  wifiManager.autoConnect();

  //Manual Wifi
  //WiFi.begin(WIFI_SSID, WIFI_PWD);
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);


  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.clear();
    display.drawString(64, 10, "Kablosuz Aga Baglaniyor...");
    display.drawXbm(46, 30, 8, 8, counter % 3 == 0 ? activeSymbol : inactiveSymbol);
    display.drawXbm(60, 30, 8, 8, counter % 3 == 1 ? activeSymbol : inactiveSymbol);
    display.drawXbm(74, 30, 8, 8, counter % 3 == 2 ? activeSymbol : inactiveSymbol);
    delay(500);
    display.display();

    counter++;
  }


  String ipaddress = WiFi.localIP().toString();

  display.clear();
 //   ui.init();
 // display.flipScreenVertically();
  display.drawString(64, 1 , "Networke Baglandi");
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 18, ipaddress);
  
  display.display();
  delay(5000);



  ui.setTargetFPS(30);

  //Hack until disableIndicator works:
  //Set an empty symbol
  ui.setActiveSymbol(emptySymbol);
  ui.setInactiveSymbol(emptySymbol);

  ui.disableIndicator();

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  ui.setFrames(frames, numberOfFrames);

  ui.setOverlays(overlays, numberOfOverlays);

  // Inital UI takes care of initalising the display too.
  ui.init();

  // Setup OTA
  //  Serial.println("Hostname: " + hostname);
  //ArduinoOTA.setHostname((const char *)hostname.c_str());
  //  ArduinoOTA.onProgress(drawOtaProgress);
  //  ArduinoOTA.begin();

  updateData(&display);

  ticker.attach(UPDATE_INTERVAL_SECS, setReadyForWeatherUpdate);

  ////////////
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
    ArduinoOTA.onProgress(drawOtaProgress);

  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_10);
    display.drawString(64, 10, "Program Guncelleniyor.");
    display.drawString(64, 25, "LUTFEN BEKLEYIN");
    display.drawString(64, 30, "");
    //  display.drawString(64, 40, myWiFiManager->getConfigPortalSSID());
    display.display();
    delay(5000);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  //////////////////////////////
  ////////////////
  //server.on("/", handle_root);
  /* server.on("/temp", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    //    gettemperature();       // read sensor
    //    webString="Temperature: "+String((int)temp_f)+" F";   // Arduino has a hard time with float to string
      server.send(200, "text/plain");            // send to someones browser when asked
    });

    server.on("/humidity", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    //    gettemperature();           // read sensor
      server.send(200, "text/plain");               // send to someones browser when asked
    }); */

  // Next define what the server should do when a client connects
  server.begin();
  server.on("/", HandleClient); // The client connected with no arguments e.g. http:192.160.0.40/
  server.on("/result", ShowClientResponse);



}

void HandleClient() {
  String webpage;
  webpage =  "<html>";
  webpage += "<head><title>Online Hava Istasyonu</title>";
  webpage += "<style>";
//  webpage += "body { background-color: #E6E6FA; font-family: Arial, Helvetica, Sans-Serif; Color: blue;}";
  webpage += "</style>";
  webpage += "</head>";
  webpage += "<body>";
  webpage += "<center>";
  webpage += "<h1><br>Online Hava Istasyonu</h1>";
  webpage += "<img src='http://arduinom.org/wp-content/uploads/2016/02/logoL.png'>";
  webpage += "<p>";
  webpage += "<p>";
  String IPaddress = WiFi.localIP().toString();
 // webpage += "<form action='http://" + IPaddress + "' method='POST'>";
 // webpage += "&nbsp;&nbsp;&nbsp;&nbsp;Lutfen Ulkeyi Girin:<input type='text' name='name_input'><BR>";
 // webpage += "Sehir adini Girin:<input type='text' name='address_input'>&nbsp;<input type='submit' value='Enter'>"; // omit <input type='submit' value='Enter'> for just 'enter'
 // webpage += "</form>";
  webpage += "<center>";
  webpage += "</body>";
  webpage  += "<iframe src='http://arduinom.org/online-micro-hava-istasyonu-v3-0/' frameborder='0' width='880' height='600' scrolling='yes'></iframe>";
  webpage += "</html>";
  server.send(200, "text/html", webpage); // Send a response to the client asking for input
  if (server.args() > 0 ) { // Arguments were received
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      Serial.print(server.argName(i)); // Display the argument
      Argument_Name = server.argName(i);
      if (server.argName(i) == "name_input") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        Clients_Response1 = server.arg(i);
        // e.g. range_maximum = server.arg(i).toInt();   // use string.toInt()   if you wanted to convert the input to an integer number
        // e.g. range_maximum = server.arg(i).toFloat(); // use string.toFloat() if you wanted to convert the input to a floating point number
      }
      if (server.argName(i) == "address_input") {
        Serial.print(" Input received was: ");
        Serial.println(server.arg(i));
        Clients_Response2 = server.arg(i);
        // e.g. range_maximum = server.arg(i).toInt();   // use string.toInt()   if you wanted to convert the input to an integer number
        // e.g. range_maximum = server.arg(i).toFloat(); // use string.toFloat() if you wanted to convert the input to a floating point number
      }
    }
  }
}

void ShowClientResponse() {
  String webpage;
  webpage =  "<html>";
  webpage += "<head><title>Online Hava Istasyonu</title>";
  webpage += "<style>";
  webpage += "body { background-color: #E6E6FA; font-family: Arial, Helvetica, Sans-Serif; Color: blue;}";
  webpage += "</style>";
  webpage += "</head>";
  webpage += "<body>";
  webpage += "<h1><br>Online Hava Istasyonu</h1>";
  webpage += "<p>Girilen Ulke: " + Clients_Response1 + "</p>";
  webpage += "<p>Girilen Sehir: " + Clients_Response2 + "</p>";
  webpage += "</body>";
  webpage += "</html>";
  server.send(200, "text/html", webpage); // Send a response to the client asking for input
}

void loop() {
  server.handleClient();
  //////////////////////////////////////////////////////


  ///////////////////////////////////////////////////////
  if (readyForWeatherUpdate && ui.getUiState()->frameState == FIXED) {
    updateData(&display);
  }

  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    ArduinoOTA.handle();
    delay(remainingTimeBudget);
  }

}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 10, "Ag Bulunamadi!!!");
  display.drawString(64, 20, "Alttaki AP'e Baglanarak");
  display.drawString(64, 30, "Ayarlarini Guncelleyiniz!!!");
  display.drawString(64, 40, myWiFiManager->getConfigPortalSSID());

  display.display();
}

void drawProgress(OLEDDisplay *display, int percentage, String label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);
  display->display();
}

void drawOtaProgress(unsigned int progress, unsigned int total) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 10, "Kablosuz Guncelleme");
  display.drawProgressBar(2, 28, 124, 10, progress / (total / 100));
  display.display();
}

void updateData(OLEDDisplay *display) {
  drawProgress(display, 10, "Saat Guncelleniyor...");
  timeClient.updateTime();
  drawProgress(display, 30, "Servera Baglaniyor");
  wunderground.updateConditions(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
  drawProgress(display, 50, "Hava Bilgisi Aliniyor...");
  wunderground.updateForecast(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
  //  drawProgress(display, 80, "Updating thingspeak...");
  //  thingspeak.getLastChannelItem(THINGSPEAK_CHANNEL_ID, THINGSPEAK_API_READ_KEY);
  lastUpdate = timeClient.getFormattedTime();
  readyForWeatherUpdate = false;
  drawProgress(display, 100, "Tum Bilgiler Alindi...");
  delay(1000);
}



void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  String date = wunderground.getDate();
  int textWidth = display->getStringWidth(date);
  display->drawString(64 + x, 5 + y, date);
  display->setFont(ArialMT_Plain_24);
  String time = timeClient.getFormattedTime();
  textWidth = display->getStringWidth(time);
  display->drawString(64 + x, 15 + y, time);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(60 + x, 5 + y, wunderground.getWeatherText());

  display->setFont(ArialMT_Plain_24);
  String temp = wunderground.getCurrentTemp() + "°C";
  display->drawString(60 + x, 15 + y, temp);
  int tempWidth = display->getStringWidth(temp);

  display->setFont(Meteocons_Plain_42);
  String weatherIcon = wunderground.getTodayIcon();
  int weatherIconWidth = display->getStringWidth(weatherIcon);
  display->drawString(32 + x - weatherIconWidth / 2, 05 + y, weatherIcon);
}


void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawForecastDetails(display, x, y, 0);
  drawForecastDetails(display, x + 44, y, 2);
  drawForecastDetails(display, x + 88, y, 4);
}

void drawThingspeak(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64 + x, 0 + y, "Outdoor");
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 10 + y, thingspeak.getFieldValue(0) + "°C");
  display->drawString(64 + x, 30 + y, thingspeak.getFieldValue(1) + "%");
}

void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  String day = wunderground.getForecastTitle(dayIndex).substring(0, 3);
  day.toUpperCase();
  display->drawString(x + 20, y, day);

  display->setFont(Meteocons_Plain_21);
  display->drawString(x + 20, y + 12, wunderground.getForecastIcon(dayIndex));

  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 34, wunderground.getForecastLowTemp(dayIndex) + "|" + wunderground.getForecastHighTemp(dayIndex));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 54, String(state->currentFrame + 1) + "/" + String(numberOfFrames));

  String time = timeClient.getFormattedTime().substring(0, 5);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(38, 54, time);

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  String temp = wunderground.getCurrentTemp() + "°C";
  display->drawString(90, 54, temp);

  int8_t quality = getWifiQuality();
  for (int8_t i = 0; i < 4; i++) {
    for (int8_t j = 0; j < 2 * (i + 1); j++) {
      if (quality > i * 25 || j == 0) {
        display->setPixel(120 + 2 * i, 63 - j);
      }
    }
  }


  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(Meteocons_Plain_10);
  String weatherIcon = wunderground.getTodayIcon();
  int weatherIconWidth = display->getStringWidth(weatherIcon);
  display->drawString(64, 55, weatherIcon);

  display->drawHorizontalLine(0, 52, 128);

}

// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality() {
  int32_t dbm = WiFi.RSSI();
  if (dbm <= -100) {
    return 0;
  } else if (dbm >= -50) {
    return 100;
  } else {
    return 2 * (dbm + 100);
  }
}

void setReadyForWeatherUpdate() {
  Serial.println("Setting readyForUpdate to true");
  readyForWeatherUpdate = true;
}

void handle_root() {
  // server.send(200, "text/plain", "Hello from the weather esp8266");

  //  server.send(200, "text/html", webPage);


}


