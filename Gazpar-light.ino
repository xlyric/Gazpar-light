//***********************************
//************* Module Gazpar vers Domoticz 
//***********************************


#include <ESP8266WiFi.h>
#include <ESPAsyncWiFiManager.h>    
//#include <ESPAsyncTCP.h>
//#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h> 
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


//Variables
byte SigneDeVie = 0;
unsigned long Jour = 0;
unsigned long Heure = 0;
unsigned long Minute = 0;
unsigned long MemMillis = 0;
int NombreBoucleMillis = 0;
int CompteurGaz = 0; // 1 impulsion = 10dm3 = 0.01 m3

bool Impulsion = false;
unsigned long TimeImpulsion = 0;


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


void setup() {

// init port
  // pinMode(Gazpar, INPUT);
  
  Serial.begin(115200);
  Serial.println();

  //attachInterrupt(Gazpar, IntChange, FALLING);
  
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Gazpar), IntChange, FALLING);
  

  Serial.println("Demarrage file System");

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

}


//***********************************
//************* loop
//***********************************

void loop() {
// Calcul temps de fonctionnement
   /* if (millis() > MemMillis) 
   {
      Jour = (millis()/86400000) + (49 * NombreBoucleMillis);
      Heure = (millis()-(Jour*86400000))/3600000;
      Minute = (millis()-(Jour*86400000)-(Heure*3600000))/60000;
   }
   else
   {
      NombreBoucleMillis = NombreBoucleMillis + 1;
   } */
   
    // MemMillis = millis();
  
   /*SigneDeVie++;
   mb.Hreg(100, SigneDeVie); */

// Reset memoire impulsion au bout de 1 seconde
  if ((Impulsion == true) and (millis() > (TimeImpulsion + 400)))
  {
    Impulsion = false; 
  }

  // Variation compteur : mise a jour de domoticz
  if ( CompteurGaz >= 1 )
  {
     SendToDomotic(String(CompteurGaz));  
     CompteurGaz = 0 ;
  }




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
