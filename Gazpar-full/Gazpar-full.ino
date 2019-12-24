//***********************************
//************* Module Gazpar vers Domoticz 
//*************  Cyril Poissonnier 2019 
//***********************************

// wifi & webserver
#include <ESP8266WiFi.h>
#include <ESPAsyncWiFiManager.h>    
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h> 
// file system
#include <fs.h>
// ntp   --> https://projetsdiy.fr/esp8266-web-serveur-partie3-heure-internet-ntp-ntpclientlib/
//#include <TimeLib.h>
//#include <NtpClientLib.h>
// ota mise à jour sans fil
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const String VERSION = "Version 1.0" ;


//***********************************
//************* Gestion serveur Domoticz / Jeedom
//***********************************

#define usejeedom 0
#define usedomoticz 1

char* domotic_server = "192.168.1.20" ;
int   port     = 8080;
const char* apiKey   = "My API KEY";   /// for Jeedom
String IDX  = "55" ;

WiFiClient domotic_client;
HTTPClient http;
AsyncWebServer server(80);
DNSServer dns;




//constantes de fonctionnement
#define USE_SERIAL  Serial
//int  = 5 ; //D1
#define Gazpar  D1
const int interruptPin = 0;
String config, valeurs;
float conso_instant, conso_heure; 
float actual_conso_heure = 0; 

//Variables

int NombreBoucleMillis = 0;
int CompteurGaz = 0; // 1 impulsion = 10dm3 = 0.01 m3

bool Impulsion = false;
unsigned long TimeImpulsion = 0;
unsigned long epoch = 0;
unsigned long last_epoch = 0;
unsigned long hour_epoch = 0;


//***********************************
//************* Fonction Gazpar
//***********************************

void ICACHE_RAM_ATTR  IntChange() {

  if (Impulsion == false)
    {
    Serial.println("Variation detectee");
    CompteurGaz+=1;
    Impulsion = true; 
    TimeImpulsion = millis();
    }
 
}


//***********************************
//************* Fonction web
//***********************************


String getConfig() {
  config = String(domotic_server) + ";" +  port + ";"  + IDX + ";"  +  VERSION +";" ;
  return String(config);
}


String getConso() {
  float conso; 
  if ( conso_heure == 0 ) { conso = actual_conso_heure ; }
  else { conso = conso_heure; }
  valeurs = String(conso_instant) + ";" + String(conso) ; 
  return String(valeurs);
}

String getDebug() {
config = String(last_epoch) + ";" +  String(epoch) + ";"  + String(conso_heure) + ";"  +  String(conso_heure) +";"  +  String(actual_conso_heure) ;
  return String(config);
}

String processor(const String& var){
   Serial.println(var);
   if (var == "SIGMA"){
    return getConso();
  }
  else if (var == "SENDMODE"){
  
    return getConso();
  }
  else if (var == "STATE"){
    
    return getConso();
  }  
}

void setup() {

// init port
  
  Serial.begin(115200);
  Serial.println();

  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Gazpar), IntChange, FALLING);
  
  Serial.println("Demarrage file System");
  SPIFFS.begin();
  
  // configuration  Wifi
  AsyncWiFiManager wifiManager(&server,&dns);
  wifiManager.autoConnect("Gazpar-Domotique", "Gazpar");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //Si connexion affichage info dans console
  WiFi.hostname("Gazpar-ESP8266");
  Serial.println("");
  Serial.print("Connection ok sur le reseau :  ");
  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); 
  Serial.println(ESP.getResetReason());

//***********************************
//************* OTA 
//***********************************
  ArduinoOTA.setHostname("Gazpar");
  //ArduinoOTA.setPassword(otapassword);
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
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
  
  /// init ntp 
	// NTP.begin() ;
 // 	last-epoch = NTP.getUptime();
	
//***********************************
//************* Web pages
//***********************************

  server.on("/",HTTP_ANY, [](AsyncWebServerRequest *request){
    Serial.print("index");
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/all.min.css", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/all.min.css", "text/css");
  });

  server.on("/favicon.ico", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/favicon.ico", "image/png");
  });

  server.on("/fa-solid-900.woff2", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/fa-solid-900.woff2", "text/css");
  });

  server.on("/sb-admin-2.js", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/sb-admin-2.js", "text/javascript");
  });

  server.on("/sb-admin-2.min.css", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/sb-admin-2.min.css", "text/css");
  });

  
  server.on("/state", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getConso().c_str());
  });

  server.on("/config", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getConfig().c_str());
  });
  
  server.on("/debug", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getDebug().c_str());
  });

	server.begin(); 
	last_epoch = millis();
	hour_epoch = last_epoch ; 
}


//***********************************
//************* loop
//***********************************

void loop() {

//epoch = NTP.getUptime();
epoch = millis();

// Reset memoire impulsion au bout de 1 seconde
  if ((Impulsion == true) and (millis() > (TimeImpulsion + 250)))
  {
    Impulsion = false; 
  }

  // Variation compteur : mise a jour de domoticz
  if ( CompteurGaz >= 1 )
  {
     SendToDomotic(String(CompteurGaz));  
     CompteurGaz = 0 ;
	 last_epoch =  epoch - last_epoch; 
	 conso_instant = 3600 * 1000 / last_epoch * 0.01 ;  /// consommation en m3 / h 
	 actual_conso_heure += 0.01; 
	 last_epoch = epoch; 
  }


if ( ( epoch - last_epoch ) >= 60000 ) { conso_instant = 0 ;} // si au dessus 1 min pas d'impulsion alors conso instant = 0 

if ( ( epoch - hour_epoch ) >= 3600000 ) { 
	hour_epoch = epoch; 
	conso_heure = actual_conso_heure ; 
	actual_conso_heure = 0; 
	}


ArduinoOTA.handle();
delay(250);

}


//***********************************
//************* Fonction domotique 
//***********************************

boolean SendToDomotic(String Svalue){
  String baseurl; 
  Serial.print("connecting to ");
  Serial.println(domotic_server);
  Serial.print("Requesting URL: ");

  if ( usedomoticz == 1 ) { baseurl = "/json.htm?type=command&param=udevice&idx="+IDX+"&nvalue=0&svalue="+Svalue;  }
  if ( usejeedom == 1 ) {  baseurl = "/core/api/jeeApi.php?apikey=" + String(apiKey) + "&type=virtual&id="+ IDX + "&value=" + Svalue;  }
  
  Serial.println(baseurl);

  http.begin(domotic_server,port,baseurl);
  int httpCode = http.GET();
  
  Serial.println("closing connection");
  http.end();
  
}
