
// Edit CORS AsyncEventSource.cpp on line 349  addHeader("Access-Control-Allow-Origin","*"); 
#include <Arduino.h>
#include <U8g2lib.h>
#include <Time.h>
#include <FS.h>
#include <SPIFFS.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS A10

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

// Import required libraries
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"


// Replace with your network credentials
//const char* ssid = "Sivak";
//const char* password = "xxx";

/* Put IP Address details */
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
// Set LED GPIO
const int ledPin = 32;
// Stores LED state
String ledState;
int ADC_VALUE = 0;

String networkname = "";

// 3 WIFI  Auto Selection
const char* WIFI_SSID = "firstwifi";
const char* WIFI_PWD = "firstpassword";

const char* WIFI_SSID2 = "2wifi";
const char* WIFI_PWD2 = "2wifipassword";

const char* WIFI_SSID3 = "3wifi";
const char* WIFI_PWD3 = "3wifipassword";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

//https://www.online-utility.org/image/convert/to/XBM
//http://javl.github.io/image2cpp/
const uint8_t wifi[] PROGMEM =
{
  0x80, 0x0F, 0x00, 0xF0, 0x7F, 0x00, 0x38, 0xE0, 0x00, 0x8C, 0x8F, 0x01,
  0xE6, 0x3F, 0x03, 0x32, 0x60, 0x02, 0x13, 0x40, 0x06, 0x09, 0x86, 0x04,
  0x89, 0x8F, 0x04, 0x89, 0x8F, 0x04, 0x89, 0x8F, 0x04, 0x09, 0x07, 0x04,
  0x00, 0x07, 0x00, 0x02, 0x07, 0x02, 0x80, 0x0F, 0x00, 0x80, 0x0D, 0x00,
  0x80, 0x18, 0x00, 0xC0, 0x18, 0x00, 0xC0, 0x18, 0x00, 0x60, 0x30, 0x00,
  0x60, 0x30, 0x00, 0x20, 0x20, 0x00, 0xF0, 0x7F, 0x00, 0xF0, 0x71, 0x00,
  0x08, 0x80, 0x00

};
const uint8_t thermometer[] PROGMEM =
{
  0x00, 0x70, 0x00, 0xCC, 0x00, 0x8C, 0x00, 0x86, 0x00, 0x41, 0x80, 0x65,
  0xC0, 0x12, 0x60, 0x19, 0xB0, 0x04, 0x48, 0x02, 0x98, 0x01, 0x9C, 0x01,
  0x5E, 0x00, 0x1A, 0x00, 0x0D, 0x00, 0x02, 0x00
};

const uint8_t flame[] PROGMEM =
{
  0x00, 0x7E, 0x00, 0xC0, 0xFF, 0x03, 0xE0, 0xD3, 0x07, 0x70, 0x00, 0x0E,
  0xB8, 0x98, 0x1D, 0x9C, 0x88, 0x39, 0xCE, 0x8C, 0x70, 0xC6, 0x88, 0x70,
  0x86, 0x88, 0x61, 0x87, 0x99, 0xE1, 0x87, 0x19, 0xE1, 0x03, 0x11, 0xC3,
  0x03, 0x33, 0xE3, 0x07, 0x31, 0xC3, 0x86, 0x11, 0xE1, 0x6F, 0xDF, 0x77,
  0xFE, 0xFF, 0x7F, 0xFE, 0xFF, 0x7F, 0xFC, 0xFF, 0x3F, 0xF8, 0xFF, 0x1F,
  0xF0, 0xFF, 0x1F, 0xE0, 0xFF, 0x07, 0xC0, 0xFF, 0x03, 0x00, 0x7E, 0x00
};

float Temperature = 0;
float TemperatureRoom=99;
float VoltRoom=0;
bool heating = false;
float currentSetpoint = 5.0;
int touchcounter = 0;
int motor=1;
double _vs[101];                 //Array with voltage - charge definitions

/**
 * Performs a binary search to find the index corresponding to a voltage.
 * The index of the array is the charge %
*/
int getChargeLevel(double volts){
  int idx = 50;
  int prev = 0;
  int half = 0;
  if (volts >= 4.2){
    return 100;
  }
  if (volts <= 3.2){
    return 0;
  }
  while(true){
    half = abs(idx - prev) / 2;
    prev = idx;
    if(volts >= _vs[idx]){
      idx = idx + half;
    }else{
      idx = idx - half;
    }
    if (prev == idx){
      break;
    }
  }
  return idx;
}

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}



/*
  Method to print the touchpad by which ESP32
  has been awaken from sleep
*/
void print_wakeup_touchpad() {
  //touch_pad_t pin;
  touch_pad_t touchPin;
  touchPin = esp_sleep_get_touchpad_wakeup_status();

  switch (touchPin)
  {
    case 0  : Serial.println("Touch detected on GPIO 4"); break;
    case 1  : Serial.println("Touch detected on GPIO 0"); break;
    case 2  : Serial.println("Touch detected on GPIO 2"); break;
    case 3  : Serial.println("Touch detected on GPIO 15"); break;
    case 4  : Serial.println("Touch detected on GPIO 13"); break;
    case 5  : Serial.println("Touch detected on GPIO 12"); break;
    case 6  : Serial.println("Touch detected on GPIO 14"); break;
    case 7  : Serial.println("Touch detected on GPIO 27"); break;
    case 8  : Serial.println("Touch detected on GPIO 33"); break;
    case 9  : Serial.println("Touch detected on GPIO 32"); break;
    default : Serial.println("Wakeup not by touchpad"); break;
  }
}

// Replaces placeholder with LED state value
String processor(const String& var) {
  Serial.println(var);
  if (var == "STATE") {
    if (digitalRead(ledPin)) {
      ledState = "ON";
    }
    else {
      ledState = "OFF";
    }
    Serial.print(ledState);
    return ledState;
  }
  return String();
}

const char* wl_status_to_string(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD: return "WL_NO_SHIELD";
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(36, INPUT);
  pinMode(32, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(35, INPUT);
  //       analogSetAttenuation(26,ADC_0db);
  ////digitalWrite(32,LOW);
  // analogSetSamples(12);
  analogSetClockDiv(32);
  analogSetPinAttenuation(35, ADC_11db);

   ledcSetup(0, 120, 10);
   ledcAttachPin(13, 0);
   ledcWrite(0, 256); //33%
  //analogSetPinAttenuation(34,ADC_0db);
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);

  digitalWrite(32, HIGH);
  delay(2000);
  digitalWrite(32, LOW);

  //parâmetros
  //  beta=(log(RT1/RT2))/((1/T11)-(1/T21));
  //  Rinf=R0*exp(-beta/T00);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  WiFi.mode(WIFI_AP);
  WiFi.persistent(false);
  WiFi.softAP("ESPThermostatv3", "12345678");
  WiFi.softAPConfig(local_ip, gateway, subnet);

  u8g2.clearBuffer();
  //u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFont(u8g2_font_8x13B_tf);//u8g2_font_ncenB14_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
 //// u8g2.drawStr(0, 0, "Hladanie Wifi");
  ////u8g2.sendBuffer();
  ////int n = WiFi.scanNetworks();

  // Serial.println("scan done");

  ////if (n == 0) {

    // Serial.println("no networks found");

  ////} else {

    // Serial.print(n);

    //Serial.println(" networks found");

   /* for (int i = 0; i < n; ++i) {

      // Print SSID and RSSI for each network found

      //Serial.print(i + 1);

      //Serial.print(": ");

      //Serial.print(WiFi.SSID(i));
      if (WiFi.SSID(i) == "firstwifi") networkname = "firstwifiname";
      if (WiFi.SSID(i) == "2wifi") networkname = "2wifiname";
      if (WiFi.SSID(i) == "3wifi") networkname = "3wifiname";
      //Serial.print(" (");

      //Serial.print(WiFi.RSSI(i));

      //Serial.println(")");

      // Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");

      //delay(10);

    }

  }

  if (networkname == "firstwifiname")
    WiFi.begin(WIFI_SSID, WIFI_PWD);
  if (networkname == "2wifiname")
    WiFi.begin(WIFI_SSID2, WIFI_PWD2);
  if (networkname == "3wifiname")
    WiFi.begin(WIFI_SSID3, WIFI_PWD3);
  int counter = 0;

  u8g2.setCursor(0, 15);
  if (networkname != "") {
    u8g2.print(networkname);  //nocheck

    u8g2.sendBuffer();
    u8g2.setCursor(0, 30);
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      Serial.print(".");

      u8g2.print( ".");
      u8g2.sendBuffer();

      counter++;
      if (counter > 100) break;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.print("Connected Wifi...");
      Serial.println(networkname);
      // Print ESP32 Local IP Address
      Serial.println(WiFi.localIP());
    }
    else
    {
      //WiFi.disconnect();

      //WiFi.disconnect();

      // WiFi.mode(WIFI_AP);
      // delay(100);
      //WiFi.begin();

    }
  }
  else
  {
    u8g2.print("Nenajdena Wifi!!!");  //nocheck

    u8g2.sendBuffer();
  }
*/


  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  File root = SPIFFS.open("/");
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      //    if(0){ //levels
      //      listDir(SPIFFS, file.name(), 0); //levels
      // }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
  Serial.print("Wifi Status: ");
  Serial.println(WiFi.status());
  if (WiFi.status() == 3)
  {
  /*  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/index.htm", "text/html");
    });*/

         server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
     
      request->send(SPIFFS, "/indexT.html", "text/html");
    });

    server.on("/js/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/js/bootstrap.bundle.min.js", "text/javascript");
    });

    server.on("/js/jquery-3.4.1.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/js/jquery-3.4.1.min.js", "text/javascript");
    });

    server.on("/js/bootstrap.min.js.map", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/js/bootstrap.min.js.map", "text/json");
    });

    server.on("/js/chartist.min.js.map", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/js/chartist.min.js.map", "text/json");
    });

    server.on("/js/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/js/bootstrap.min.js", "text/javascript");
    });

    server.on("/js/chartist.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/js/chartist.min.js", "text/javascript");
    });

    server.on("/css/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/css/bootstrap.min.css", "text/css");
    });

    server.on("/css/styles.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/css/styles.css", "text/css");
    });

    server.on("/images/atom196.png", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/images/atom196.png", "image/png");
    });

    server.on("favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/favicon.ico", "image/png");
    });

    server.on("/images/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/favicon.ico", "image/png");
    });

    server.on("/css/bootstrap.min.css.map", HTTP_GET, [](AsyncWebServerRequest * request) {
     
      request->send(SPIFFS, "/css/bootstrap.min.css.map", "text/json");
    });



    server.on("/css/chartist.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(SPIFFS, "/css/chartist.min.css", "text/css");
    });

    //-----------------------------------------------
    server.on("/getadc", HTTP_GET, [](AsyncWebServerRequest * request) {

      request->send(200, "text/plain", (String)ADC_VALUE);

    });

server.on("/getmotor", HTTP_GET, [](AsyncWebServerRequest * request) {

      request->send(200, "text/plain", (String)motor);

    });
    
    server.on("/getvolt", HTTP_GET, [](AsyncWebServerRequest * request) {

      request->send(200, "text/plain", (String)VoltRoom);

    });

        server.on("/getvoltperc", HTTP_GET, [](AsyncWebServerRequest * request) {

      request->send(200, "text/plain", (String)getChargeLevel(VoltRoom));

    });

    
    
    server.on("/gettemperature", HTTP_GET, [](AsyncWebServerRequest * request) {
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (String)Temperature);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    //  request->send(200, "text/plain", (String)Temperature);

    });

    server.on("/getroomtemperature", HTTP_GET, [](AsyncWebServerRequest * request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (String)TemperatureRoom);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  //  request->addHeader("Access-Control-Allow-Origin", "*"));
    //  request->send(200, "text/plain", (String)TemperatureRoom);

    });

    server.on("/getheating", HTTP_GET, [](AsyncWebServerRequest * request) {

      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (String)heating);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    //  request->send(200, "text/plain", (String)heating);

    });

    server.on("/getsetpoint", HTTP_GET, [](AsyncWebServerRequest * request) {

      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (String)currentSetpoint);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
     // request->send(200, "text/plain", (String)currentSetpoint);

    });

    server.on("/getwifimode", HTTP_GET, [](AsyncWebServerRequest * request) {
      
      request->send(200, "text/plain", (String)WiFi.getMode());

    });

    server.on("/getwifiip", HTTP_GET, [](AsyncWebServerRequest * request) {
      
      request->send(200, "text/plain", (String)WiFi.localIP().toString());
    });

    server.on("/getwifiSSID", HTTP_GET, [](AsyncWebServerRequest * request) {
      
      request->send(200, "text/plain", (String)WiFi.SSID());
    });

    server.on("/getwifistate", HTTP_GET, [](AsyncWebServerRequest * request) {
      
      request->send(200, "text/plain", (String)wl_status_to_string(WiFi.status()));
    });

    server.on("/getfreeheap", HTTP_GET, [](AsyncWebServerRequest * request) {
      
      request->send(200, "text/plain", (String)ESP.getFreeHeap());
    });


    server.on("/setcurrentsetpoint", HTTP_GET, [](AsyncWebServerRequest * request) {
      int paramsNr = request->params();
      Serial.println(paramsNr);

      for (int i = 0; i < paramsNr; i++) {

        AsyncWebParameter* p = request->getParam(i);
        currentSetpoint = p->value().toFloat();
        
        Serial.print("Param name: ");
        Serial.println(p->name());
        Serial.print("Param value: ");
        Serial.println(p->value());
        Serial.println("------");
      }
      //request->addHeader("Access-Control-Allow-Origin", "*"));
       AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (String)currentSetpoint);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
      //request->send(200, "text/plain", String(currentSetpoint));
    });

 server.on("/setmotor", HTTP_GET, [](AsyncWebServerRequest * request) {
      
      int paramsNr = request->params();
      Serial.println(paramsNr);

      for (int i = 0; i < paramsNr; i++) {

        AsyncWebParameter* p = request->getParam(i);
        motor = p->value().toInt();
        
        Serial.print("Param name: ");
        Serial.println(p->name());
        Serial.print("Param value: ");
        Serial.println(p->value());
        Serial.println("------");
      }
       AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (String)motor);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
     // request->send(200, "text/plain", String(motor));
    });

    server.on("/sendtemp", HTTP_POST, [](AsyncWebServerRequest * request) {
      
      int paramsNr = request->params();
      Serial.println(paramsNr);

      for (int i = 0; i < paramsNr; i++) {

        AsyncWebParameter* p = request->getParam(i);
       // currentSetpoint = p->value().toFloat();
       
        Serial.println("Temp received... ");
        Serial.print("Param name: ");
        Serial.println(p->name());
        Serial.print("Param value: ");
        Serial.println(p->value());
        if (i==0) TemperatureRoom=p->value().toFloat();
         if (i==2) VoltRoom=p->value().toFloat();
        Serial.println("------");
      }
       request->send(200, "text/plain", "OK");
    });

  }
  if (WiFi.status() != 3)
  {
     // server.serveStatic("/", SPIFFS, "/index.html", "max-age=86400");

server.serveStatic("/", SPIFFS, "/indexT.html", "max-age=86400");
      
  //server.serveStatic("/index.html", SPIFFS, "/index.html", "max-age=86400");
    server.serveStatic("/index.html", SPIFFS, "/indexT.html", "max-age=86400");
    /*
    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
      
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/index.html", "text/html", false, processor);
      File file = SPIFFS.open("/index.hmtl");
      response->addHeader("Content-Length", (String) file.size ());
      request->send(response);
      file.close();
    });*/

    server.on("/js/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/js/bootstrap.bundle.min.js", "text/javascript", false, processor);
      File file = SPIFFS.open("/js/bootstrap.bundle.min.js");
      response->addHeader("Content-Length", (String) file.size ());
      request->send(response);
      file.close();
    });

    server.on("/js/jquery-3.4.1.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/js/jquery-3.4.1.min.js", "text/javascript", false, processor);
      File file = SPIFFS.open("/js/jquery-3.4.1.min.js");
      response->addHeader("Content-Length", (String) file.size ());
      request->send(response);
      file.close();
    });

    server.on("/js/bootstrap.min.js.map", HTTP_GET, [](AsyncWebServerRequest * request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/js/bootstrap.min.js.map", "text/json", false, processor);
      File file = SPIFFS.open("/js/bootstrap.min.js.map");
      response->addHeader("Content-Length", (String) file.size ());
      request->send(response);
      file.close();
    });
    //-----------------------------------------------
    server.on("/js/chartist.min.js.map", HTTP_GET, [](AsyncWebServerRequest * request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/js/chartist.min.js.map", "text/json", false, processor);
      File file = SPIFFS.open("/js/chartist.min.js.map");
      response->addHeader("Content-Length", (String) file.size ());
      request->send(response);
      file.close();
    });

    server.on("/js/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/js/bootstrap.min.js", "text/javascript", false, processor);
      File file = SPIFFS.open("/js/bootstrap.min.js");
      response->addHeader("Content-Length", (String) file.size ());
      request->send(response);
      file.close();
    });

    server.on("/js/chartist.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/js/chartist.min.js", "text/javascript", false, processor);
      File file = SPIFFS.open("/js/chartist.min.js");
      response->addHeader("Content-Length", (String) file.size ());
      request->send(response);
      file.close();
    });

    server.on("/css/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/css/bootstrap.min.css", "text/css", false, processor);
      File file = SPIFFS.open("/css/bootstrap.min.css");
      response->addHeader("Content-Length", (String) file.size ());
      request->send(response);
      file.close();
    });

    server.on("/css/styles.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/css/styles.css", "text/css", false, processor);
      File file = SPIFFS.open("/css/styles.css");
      response->addHeader("Content-Length", (String) file.size ());
      request->send(response);
      file.close();
    });

    server.on("/images/atom196.png", HTTP_GET, [](AsyncWebServerRequest * request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/images/atom196.png", "image/png", false, processor);
      File file = SPIFFS.open("/images/atom196.png");
      response->addHeader("Content-Length", (String) file.size ());
      request->send(response);
      file.close();
    });

    server.on("favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/favicon.ico", "image/png", false, processor);
      File file = SPIFFS.open("/favicon.ico");
      response->addHeader("Content-Length", (String) file.size ());
      request->send(response);
      file.close();
    });

    server.on("/images/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/favicon.ico", "image/png", false, processor);
      File file = SPIFFS.open("/favicon.ico");
      response->addHeader("Content-Length", (String) file.size ());
      request->send(response);
      file.close();
    });

    server.on("/css/bootstrap.min.css.map", HTTP_GET, [](AsyncWebServerRequest * request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/css/bootstrap.min.css.map", "text/json", false, processor);
      File file = SPIFFS.open("/css/bootstrap.min.css.map");
      response->addHeader("Content-Length", (String) file.size ());
      request->send(response);
      file.close();
    });

    server.on("/css/chartist.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/css/chartist.min.css", "text/css", false, processor);
      File file = SPIFFS.open("/css/chartist.min.css");
      response->addHeader("Content-Length", (String) file.size ());
      request->send(response);
      file.close();
    });



    //-----------------------------------------------
    server.on("/getadc", HTTP_GET, [](AsyncWebServerRequest * request) {

      request->send(200, "text/plain", (String)ADC_VALUE);

    });

    //VoltRoom
    server.on("/getvolt", HTTP_GET, [](AsyncWebServerRequest * request) {

      request->send(200, "text/plain", (String)VoltRoom);

    });

    server.on("/getvoltperc", HTTP_GET, [](AsyncWebServerRequest * request) {

      request->send(200, "text/plain", (String)getChargeLevel(VoltRoom));

    });

    

     server.on("/sendtemp", HTTP_POST, [](AsyncWebServerRequest * request) {
      
      int paramsNr = request->params();
      Serial.println(paramsNr);

      for (int i = 0; i < paramsNr; i++) {

        AsyncWebParameter* p = request->getParam(i);
       // currentSetpoint = p->value().toFloat();
        
        Serial.println("Temp received... ");
        Serial.print("Param name: ");
        Serial.println(p->name());
        Serial.print("Param value: ");
        Serial.println(p->value());
         if (i==0) TemperatureRoom=p->value().toFloat();
         if (i==2) VoltRoom=p->value().toFloat();
        Serial.println("------");
      }
      request->send(200, "text/plain", "OK");
    });
    //-----------------------------------------------
    server.on("/gettemperature", HTTP_GET, [](AsyncWebServerRequest * request) {

       AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (String)Temperature);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
     // request->send(200, "text/plain", (String)Temperature);

    });

        server.on("/getroomtemperature", HTTP_GET, [](AsyncWebServerRequest * request) {
            AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (String)TemperatureRoom);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
         // request->addHeader("Access-Control-Allow-Origin", "*"));
     // request->send(200, "text/plain", (String)TemperatureRoom);

    });

      server.on("/getmotor", HTTP_GET, [](AsyncWebServerRequest * request) {
          AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (String)motor);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
     // request->send(200, "text/plain", (String)motor);

    });

    server.on("/getheating", HTTP_GET, [](AsyncWebServerRequest * request) {
       AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (String)heating);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
     // request->send(200, "text/plain", (String)heating);

    });

    server.on("/getsetpoint", HTTP_GET, [](AsyncWebServerRequest * request) {
       AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (String)currentSetpoint);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
     // request->send(200, "text/plain", (String)currentSetpoint);

    });

    server.on("/getwifimode", HTTP_GET, [](AsyncWebServerRequest * request) {
      
      request->send(200, "text/plain", (String)WiFi.getMode());

    });

    server.on("/getwifiip", HTTP_GET, [](AsyncWebServerRequest * request) {
      
      request->send(200, "text/plain", (String)WiFi.softAPIP().toString());
    });

    server.on("/getwifiSSID", HTTP_GET, [](AsyncWebServerRequest * request) {
      
      request->send(200, "text/plain", "ESPThermostatv3");
    });

    server.on("/getwifistate", HTTP_GET, [](AsyncWebServerRequest * request) {
      
      request->send(200, "text/plain", (String)wl_status_to_string(WiFi.status()));
    });

    server.on("/getfreeheap", HTTP_GET, [](AsyncWebServerRequest * request) {
      
      request->send(200, "text/plain", (String)ESP.getFreeHeap());
    });

    server.on("/setcurrentsetpoint", HTTP_GET, [](AsyncWebServerRequest * request) {
      
      int paramsNr = request->params();
      Serial.println(paramsNr);

      for (int i = 0; i < paramsNr; i++) {

        AsyncWebParameter* p = request->getParam(i);
        currentSetpoint = p->value().toFloat();
       
        Serial.print("Param name: ");
        Serial.println(p->name());
        Serial.print("Param value: ");
        Serial.println(p->value());
        Serial.println("------");
      }
     // request->addHeader("Access-Control-Allow-Origin", "*"));
     AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (String)currentSetpoint);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
      // request->send(200, "text/plain", String(currentSetpoint));
    });

server.on("/setmotor", HTTP_GET, [](AsyncWebServerRequest * request) {
      
      int paramsNr = request->params();
      Serial.println(paramsNr);

      for (int i = 0; i < paramsNr; i++) {

        AsyncWebParameter* p = request->getParam(i);
        motor = p->value().toInt();
       
        Serial.print("Param name: ");
        Serial.println(p->name());
        Serial.print("Param value: ");
        Serial.println(p->value());
        Serial.println("------");
      }
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (String)motor);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
      // request->send(200, "text/plain", String(motor));
    });

  }

  // locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");


  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");


  Serial.print("Device 0 Address: ");
  for (uint8_t i = 0; i < 8; i++)
  {
    if (insideThermometer[i] < 16) Serial.print("0");
    Serial.print(insideThermometer[i], HEX);
  }
  // printAddress();
  Serial.println();

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 9);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC);
  Serial.println();

  // Start server
  server.begin();
  //Setup interrupt on Touch Pad 3 (GPIO15)
  touchAttachInterrupt(T6, callback, 40);

  //Configure Touchpad as wakeup source
  esp_sleep_enable_touchpad_wakeup();
   //Liion voltages....
 _vs[0] = 3.200; 
    _vs[1] = 3.250; _vs[2] = 3.300; _vs[3] = 3.350; _vs[4] = 3.400; _vs[5] = 3.450;
    _vs[6] = 3.500; _vs[7] = 3.550; _vs[8] = 3.600; _vs[9] = 3.650; _vs[10] = 3.700;
    _vs[11] = 3.703; _vs[12] = 3.706; _vs[13] = 3.710; _vs[14] = 3.713; _vs[15] = 3.716;
    _vs[16] = 3.719; _vs[17] = 3.723; _vs[18] = 3.726; _vs[19] = 3.729; _vs[20] = 3.732;
    _vs[21] = 3.735; _vs[22] = 3.739; _vs[23] = 3.742; _vs[24] = 3.745; _vs[25] = 3.748;
    _vs[26] = 3.752; _vs[27] = 3.755; _vs[28] = 3.758; _vs[29] = 3.761; _vs[30] = 3.765;
    _vs[31] = 3.768; _vs[32] = 3.771; _vs[33] = 3.774; _vs[34] = 3.777; _vs[35] = 3.781;
    _vs[36] = 3.784; _vs[37] = 3.787; _vs[38] = 3.790; _vs[39] = 3.794; _vs[40] = 3.797;
    _vs[41] = 3.800; _vs[42] = 3.805; _vs[43] = 3.811; _vs[44] = 3.816; _vs[45] = 3.821;
    _vs[46] = 3.826; _vs[47] = 3.832; _vs[48] = 3.837; _vs[49] = 3.842; _vs[50] = 3.847;
    _vs[51] = 3.853; _vs[52] = 3.858; _vs[53] = 3.863; _vs[54] = 3.868; _vs[55] = 3.874;
    _vs[56] = 3.879; _vs[57] = 3.884; _vs[58] = 3.889; _vs[59] = 3.895; _vs[60] = 3.900;
    _vs[61] = 3.906; _vs[62] = 3.911; _vs[63] = 3.917; _vs[64] = 3.922; _vs[65] = 3.928;
    _vs[66] = 3.933; _vs[67] = 3.939; _vs[68] = 3.944; _vs[69] = 3.950; _vs[70] = 3.956;
    _vs[71] = 3.961; _vs[72] = 3.967; _vs[73] = 3.972; _vs[74] = 3.978; _vs[75] = 3.983;
    _vs[76] = 3.989; _vs[77] = 3.994; _vs[78] = 4.000; _vs[79] = 4.008; _vs[80] = 4.015;
    _vs[81] = 4.023; _vs[82] = 4.031; _vs[83] = 4.038; _vs[84] = 4.046; _vs[85] = 4.054;
    _vs[86] = 4.062; _vs[87] = 4.069; _vs[88] = 4.077; _vs[89] = 4.085; _vs[90] = 4.092;
    _vs[91] = 4.100; _vs[92] = 4.111; _vs[93] = 4.122; _vs[94] = 4.133; _vs[95] = 4.144;
    _vs[96] = 4.156; _vs[97] = 4.167; _vs[98] = 4.178; _vs[99] = 4.189; _vs[100] = 4.200;

}

void callback() {
  //placeholder callback function
  Serial.println("Touch 6 callback...");
}

void loop() {
  //  server.handleClient();
  // put your main code here, to run repeatedly:
  u8g2.clearBuffer();
  //u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFont(u8g2_font_8x13B_tf);//u8g2_font_ncenB14_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
 // u8g2.drawStr(0, 0, "Termostat");
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(5, 55);
  if (WiFi.status() != 3){
    u8g2.print(WiFi.softAPIP());
      if (WiFi.softAPgetStationNum()>0)
      {
    u8g2.print(",");
  u8g2.print(WiFi.softAPgetStationNum());
  //  Serial.print("Stations connected: ");
 // Serial.println(WiFi.softAPgetStationNum());
      }
  }
  if (WiFi.status() == 3)
    u8g2.print(WiFi.localIP());

  // u8g2.drawStr(0,12,WiFi.localIP());
  ////  u8g2.setFont(u8g2_font_unifont_t_symbols);
  //// u8g2.setFontPosTop();
  // u8g2.drawUTF8(0, 24, "🌡");

  ////u8g2.drawGlyph(0, 24, 0x2103);  //stupen celsius

  ////u8g2.drawGlyph(10, 24, 0x23f0);  //clock
  ////u8g2.drawGlyph(25, 24, 0x2610);  //nocheck
  ////u8g2.drawGlyph(50, 24, 0x2611);  //check
  ////u8g2.drawGlyph(75, 24, 0x2612);  //cancell check
  //u8g2.drawGlyph(100, 24, 0x2615);  //heat salka

  // int temp;
  // method 2 - faster
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempC(insideThermometer);
  if (tempC == -127) tempC = 99;
  Temperature = tempC;
  //Serial.print("Temp C: ");
  //Serial.println(tempC);

  //Preasure 2.96 ATM / Volt
  /* int sensorVal=analogRead(34A1);
    float voltage = (sensorVal*1.1)/3096.0;*/
  //    Serial.print("Volts: ");
  //    Serial.print(voltage);
  /*
    float pressure_pascal = (3.0*((float)voltage-0.475))*1000000.0;                       //calibrate here
    float pressure_bar = pressure_pascal/10e5;
    float pressure_psi = pressure_bar*14.5038;
    Serial.print("Pressure = ");
    Serial.print(pressure_bar);
    Serial.print(" bars  ");
    Serial.print ("psi ");
    Serial.print (pressure_psi);
    Serial.println();

  */

  u8g2.setFont(u8g2_font_6x10_tf);//u8g2_font_8x13B_tf);//u8g2_font_ncenB14_tf);

  //u8g2.drawStr(0, 38, "Touch: ");
  //   u8g2.setCursor(35,38);

  u8g2.setFont(u8g2_font_unifont_t_symbols);
  u8g2.setFontPosTop();



  /*
    if (touchRead(T4)>100)
    u8g2.drawGlyph(25, 30, 0x2610);  //nocheck
    else
    u8g2.drawGlyph(25, 30, 0x2611);  //check

        if (touchRead(T5)>100)
    u8g2.drawGlyph(50, 30, 0x2610);  //nocheck
    else
    u8g2.drawGlyph(50, 30, 0x2611);  //check

    if (touchRead(T6)>100)
    u8g2.drawGlyph(75, 30, 0x2610);  //nocheck
    else
    u8g2.drawGlyph(75, 30, 0x2611);  //check

  */

  // draw a header frame
  //u8g2.drawRFrame(1,44,128,5,3);

  u8g2.setFont(u8g2_font_8x13B_tf);//u8g2_font_ncenB14_tf);

 u8g2.setCursor(0, 0);
  u8g2.print(VoltRoom, 2);
 u8g2.print(" -> ");
 u8g2.print(getChargeLevel(VoltRoom));
 u8g2.print("%");
  u8g2.setCursor(16, 15);
  u8g2.print(TemperatureRoom, 2);

   u8g2.setCursor(80, 15);
  u8g2.print(tempC, 2);
  //
  // u8g2.setFontPosTop();
  u8g2.drawXBM(0, 15, 16, 16, (const uint8_t *) thermometer);
  // u8g2.drawUTF8(0, 50, "🌡");
  u8g2.setFont(u8g2_font_unifont_t_symbols);
  u8g2.drawGlyph(57, 15, 0x2103);  //stupen celsius

u8g2.drawGlyph(120, 15, 0x2103);  //stupen celsius

  u8g2.setFont(u8g2_font_6x10_tf);//u8g2_font_8x13B_tf);//u8g2_font_ncenB14_tf);
  u8g2.setCursor(90, 0);
  u8g2.print(currentSetpoint, 2);

  int valtemp = map(round(TemperatureRoom), 5, 40, 1, 126);
  //Serial.println(valadc);
  u8g2.drawRFrame(1, 34, 128, 5, 3);
  u8g2.drawBox (2, 35, (int)valtemp, 3);
  //19px=10c
  //55px=20c
  //91px=30c
  //127px=40c
  u8g2.drawLine(19, 38, 19, 44);
  u8g2.drawLine(55, 38, 55, 44);
  u8g2.drawLine(91, 38, 91, 44);
  u8g2.drawLine(127, 38, 127, 44);

  u8g2.drawLine(73, 38, 73, 41);
  u8g2.drawLine(109, 38, 109, 41);
  u8g2.drawLine(37, 38, 37, 41);


  ADC_VALUE = analogRead(35);
  //Serial.print("ADC VALUE = ");
  //Serial.println(ADC_VALUE);
  //delay(1000);
  //int voltage_value = (ADC_VALUE * 3.3 ) / (4095);
  //Serial.print("Voltage = ");
  //Serial.print(voltage_value);
  //Serial.println("volts");
  int valadc = map(ADC_VALUE, 0, 4095, 1, 126);
  //Serial.println(valadc);
  u8g2.drawRFrame(1, 47, 128, 5, 3);
  u8g2.drawBox (2, 48, (int)valadc, 3);
  u8g2.setFont(u8g2_font_6x10_tf);//u8g2_font_8x13B_tf);//u8g2_font_ncenB14_tf);
  u8g2.setCursor(100, 54);
  u8g2.print(ADC_VALUE, HEX);


  if (TemperatureRoom < (currentSetpoint - 0.5))
  {

    heating = true;
    digitalWrite(32, HIGH);
  }
  if (TemperatureRoom > (currentSetpoint + 0.5))
  {
    heating = false;
    digitalWrite(32, LOW);
  }

  if (heating)  u8g2.drawXBM(104, 27, 24, 24, (const uint8_t *) flame);

  if (motor==0) digitalWrite(34,LOW);
  if (motor==1) {
   // analogWriteResolution(34, 10);
   // analogWriteFrequency(34, 120);
   ledcWrite(0, 338);
    //analogWrite(34,338); //33%
  }
  if (motor==2) {
  //  analogWriteResolution(34, 10);
   // analogWriteFrequency(34, 120);
   ledcWrite(0, 512);
  //  analogWrite(34,512); //50%
  }
  if (motor==3) {
   // analogWriteResolution(34, 10);
   // analogWriteFrequency(34, 120);
   ledcWrite(0, 767);
    //analogWrite(34,767); //75%
  }
  if (motor==4) {
  //  analogWriteResolution(34, 10);
   // analogWriteFrequency(34, 120);
   ledcWrite(0, 1023);
   // analogWrite(34,1023); //100%
  }
  
  if (WiFi.status() == 3)   u8g2.drawXBM(107, 0, 24, 24, (const uint8_t *) wifi);
  // display.setFont(ArialMT_Plain_16);
  //display.drawString(0, 0, String(tempC));
  //display.setFont(ArialMT_Plain_10);
  ////u8g2.setCursor(70,100);
  ////u8g2.print(currentSetpoint,2);

  /*
    OneWire ow(16);

    uint8_t address[8];
    uint8_t count = 0;


    if (ow.search(address))
    {
      Serial.print("\nuint8_t pin");
      Serial.print(16, DEC);
      Serial.println("[][8] = {");
      do {
        count++;
        Serial.println("  {");
        for (uint8_t i = 0; i < 8; i++)
        {
          Serial.print("0x");
          if (address[i] < 0x10) Serial.print("0");
          Serial.print(address[i], HEX);
          if (i < 7) Serial.print(", ");
        }
        Serial.println("  },");
      } while (ow.search(address));

      Serial.println("};");
      Serial.print("// nr devices found: ");
      Serial.println(count);


    //u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setFont(u8g2_font_8x13B_tf);//u8g2_font_ncenB14_tf);

    for (uint8_t i = 0; i < 8; i++)
        {

        u8g2.drawStr(0+(i*10), 50, "0x");
          Serial.print("0x");
          if (address[i] < 0x10) u8g2.drawStr(0+(i*10)+10, 50,"0");
          u8g2.print(address[i],HEX);
          if (i < 7) u8g2.drawStr(0+(i*10)+30, 50,", ");
        }
    }
    else
    {
     // u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setFont(u8g2_font_8x13B_tf);//u8g2_font_ncenB14_tf);

        u8g2.drawStr(0, 50, "No Temp sensor !");

        }
  */
  u8g2.sendBuffer();
  // deley between each page
  delay(1000);
  if (touchRead(T6) < 50) touchcounter++;
  else
    touchcounter = 0;

  if (touchcounter >= 5)
  {


    u8g2.setFont(u8g2_font_8x13B_tf);//u8g2_font_ncenB14_tf);
    u8g2.setDrawColor(0);
    u8g2.drawStr(0, 0, "Termostat v 3");
    u8g2.setDrawColor(1);
    u8g2.drawStr(0, 0, "Vypinanie !!!");
    u8g2.sendBuffer();
    //Go to sleep now
    Serial.println("Going to sleep now");
    delay(2000);
    heating = false;
    digitalWrite(32, LOW);
    u8g2.clear();
    esp_deep_sleep_start();
  }

}
