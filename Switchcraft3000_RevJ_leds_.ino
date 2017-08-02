
#include <DNSServer.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

#include <ESP8266mDNS.h>

#include <ESP8266WebServer.h>

#include <WiFiManager.h>

#include <EEPROM.h>

ADC_MODE(ADC_VCC);

char thingspeak_key[40] = "";
char thingspeak_field[5] = "";
unsigned long sleepTime = 0;  //Note: If timer runs out and no reset, it will consume a LOT of power
int redLed = 12;
int greenLed = 13;


String var = "10";
//const char* Tssid = "yourssid";
//const char* Tpassword = "yourpw";
const char* host =  "api.thingspeak.com"; //"btn-log.herokuapp.com";
//const char* privateKey = "yourkey";

bool shouldSaveConfig = false;
bool productionMode = false;

//default custom static IP
char static_ip[16] = "10.0.1.67";
char static_gw[16] = "10.0.1.1";
char static_sn[16] = "255.255.255.0";

void upldTngspk(String var, String keyString, String batLevel);

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());   //boots as AP

  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void blinkLeds(int led, int blinkCount, float intervalSec){
  int counter=0;
  while(counter < blinkCount){
    digitalWrite(led, 0);
    delay(intervalSec*1000);
    digitalWrite(led, 1);
    delay(intervalSec*1000);
    counter+=1;
  }
}

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
  
   
}


signed int setupDone = 0;

void setup() {
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  digitalWrite(redLed, 1);
  digitalWrite(greenLed, 1);


  
  Serial.begin(9600);  //debug
  EEPROM.begin(200);
  delay(10);
  Serial.println("start");     //debug
  

//  EEPROM.write(0, 0);
//    EEPROM.commit();   //CLEARING EEPROM WITHOUT NORMAL RESET BUTTON
//    delay(500);

//for (int i = 0 ; i < 200 ; i++) {
//    EEPROM.write(i, 0);
//  }
//  EEPROM.commit();
//  delay(500);

  setupDone = int(EEPROM.read(0));
  //Serial.println(setupDone);  //debug

 
  bool firstSetup = false;

  // if setup done and need to set up again
  pinMode(2, INPUT);
  delay(200);
 //pinMode(LED_BUILTIN, OUTPUT);

  bool runSetup = false;
  //Serial.println(digitalRead(2));   //debug
  if(digitalRead(2) == LOW) {
    delay(3000);
    if(digitalRead(2) == LOW) {
      runSetup = true;
    }
  }
  
  if(setupDone != 5 || runSetup) {  //SETUP LOOP
    Serial.println("in setup");
    blinkLeds(redLed, 2, 0.2);  //blink red led 2 x 0.2 sec
    firstSetup = true;
//    Serial.println("Reading EEPROM thingspeak key");
//  String savedKey = "";
//  for (int i = 1; i < 30; ++i)
//    {
//      savedKey += char(EEPROM.read(i));
//    }
//   
//  Serial.print("Saved thingspeak key: ");
//  Serial.println(savedKey);

  
      //set static ip
    IPAddress _ip,_gw,_sn;
    _ip.fromString(static_ip);
    _gw.fromString(static_gw);
    _sn.fromString(static_sn);

//  WiFiClient  client;
  
    WiFiManagerParameter custom_thingspeak_key("thingspeak_key", "button id (no spaces)", thingspeak_key, 40);
    //WiFiManagerParameter custom_thingspeak_field("thingspeak_field", "thingspeak field #", thingspeak_field, 5);
    
    WiFiManager wifiManager;
  
    wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
    
    wifiManager.setAPCallback(configModeCallback); 
    wifiManager.setSaveConfigCallback(saveConfigCallback);
  
    wifiManager.resetSettings();
    
    wifiManager.addParameter(&custom_thingspeak_key);
    //wifiManager.addParameter(&custom_thingspeak_field);
    
    wifiManager.autoConnect("NewButton", "password");

    if (!wifiManager.autoConnect("NewButton", "password")) {
      Serial.println("failed to connect and hit timeout..maybe.");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }
    strcpy(thingspeak_key, custom_thingspeak_key.getValue());
    //strcpy(thingspeak_field, custom_thingspeak_field.getValue());
    Serial.println("before save config and shouldsave =");
    Serial.println(shouldSaveConfig);
    
    if(shouldSaveConfig) {

      //writing channel key to eeprom
      //String fieldString(thingspeak_field);
      String keyString(thingspeak_key);   
      if (thingspeak_key != "") {
        Serial.println("clearing eeprom");
        for (int i = 1; i < 200; i++) { EEPROM.write(i, 0); }
        Serial.println(keyString);
              
        Serial.println("writing eeprom thingspeak key:");
        for (int i = 1; i <= keyString.length(); i++)
          {
            EEPROM.write(i, keyString[i-1]);
            Serial.print("Wrote: ");
            Serial.println(keyString[i-1]); 
          }
  //      for (int i = 50; i < (52+fieldString.length()); i++)
  //      {
  //          EEPROM.write(i, fieldString[i-50]);
  //          Serial.print("Wrote: ");
  //          Serial.println(fieldString[i-50]); 
  //      }
        
        //content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        //statusCode = 200;
        Serial.println("success");  //debug
      } else {
        //content = "{\"Error\":\"404 not found\"}";
        //statusCode = 404;
        Serial.println("Sending 404");
      }
  
    
      EEPROM.write(0, 5 );
      EEPROM.commit();
  
      delay(1500);
      ESP.reset();
      delay(5000);
    }
 
  }  //ENDIF SETUP NEEDED

  blinkLeds(greenLed, 1, 0.2);  //blink green led 1 * 0.2 sec
 
  Serial.println("Reading EEPROM thingspeak key");
  String savedKey = "";
  for (int i = 1; i < 30; i++)
    {
      savedKey += char(EEPROM.read(i));
    }
//  String savedField = "";
//    for (int i = 50; i < 60; i++)
//    {
//      savedField += char(EEPROM.read(i));
//    }

/*
  WiFi.begin("AndroidAP", "2350qqqq");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
*/

 
  Serial.println("connected...");
  productionMode = true;

   if(firstSetup) {
    var = "0";   //initial ping is 0 value
  }
  
  String batteryLevel = String(ESP.getVcc());
  delay(200);
  upldTngspk(var, savedKey, batteryLevel);

  ESP.deepSleep(sleepTime,WAKE_RF_DEFAULT); 
  delay(1000);
}

void loop() {
  if(digitalRead(2) == LOW) {
    Serial.println(digitalRead(2) == LOW);
    delay(3900);
    if(digitalRead(2) == LOW) {
       EEPROM.write(0, 0);
       EEPROM.commit();
       delay(1000);
       ESP.reset();
    }
  }

    
//  ThingSpeak.writeField(chid, 1, "Test1", "yourkey");
//  delay(20000); // ThingSpeak will only accept updates every 15 seconds.
//   upldTngspk(var);
  // put your main code here, to run repeatedly:
//  if(productionMode) {
//    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level                            // it is acive low on the ESP-01)
//    delay(1000);                      // Wait for a second
//    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
//    delay(2000); 
//  }
}

void upldTngspk(String var, String keyString, String batLevel) {
  //delay(5000);
//  ++value;
  char bat[10];
  batLevel.toCharArray(bat, 10);
  char key[30];
  keyString.toCharArray(key, 30);
//  char field[5];
//  fieldString.toCharArray(field, 5);
  
  Serial.print("connecting to ");
  Serial.println(host);
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  // We now create a URI for the request
//  String url = "/api/toggl/";  //heroku
  String url ="";            //thingspeak
 
  url += "/update?api_key=";  //thingspeak
  url += key;               //thingspeak
  url += "&field1";            //thingspeak
  //url += field;            //thingspeak
  url += "=";             //thingspeak
  url += bat;               //thingspeak
  
  
//  url += key;   //heroku
//  url += "/";   //heroku
//  url += bat;   //heroku
  
  Serial.print("Requesting URL: ");
  Serial.println(url);
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(10);
  blinkLeds(greenLed, 5, 0.1);
  // Read all the lines of the reply from server and print them to Serial
//  while(client.available()){
//    String line = client.readStringUntil('\r');
//    Serial.print(line);
//  }
  Serial.println();
  Serial.println("sent value");
}




