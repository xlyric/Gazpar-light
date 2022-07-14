//***********************************
//************* Module Gazpar vers Domoticz 
//*************  Cyril Poissonnier 2022 
//***********************************

#include <Arduino.h>
#include <ESP8266WiFi.h>
// Web services
#include <ESP8266WiFi.h>
#include <ESPAsyncWiFiManager.h>    
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h> 
//mqtt
#include <PubSubClient.h>
// configuration
#include "config.h"

const String VERSION = "Version 2.0" ;

//***********************************
//************* Gestion serveur Domoticz / Jeedom
//***********************************

// mqtt
void mqtt(String idx, String value);

WiFiClient domotic_client;
PubSubClient client(domotic_client);


AsyncWebServer server(80);
DNSServer dns;
HTTPClient http;

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

void IRAM_ATTR  IntChange() {

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
  
  Serial.begin(115200);
  Serial.println();

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

}


//***********************************
//************* loop
//***********************************

void loop() {


// Reset memoire impulsion au bout de 1 seconde
  if ((Impulsion == true) and (millis() > (TimeImpulsion + 400)))
  {
    Impulsion = false; 
  }

  // Variation compteur : mise a jour de domoticz
  if ( CompteurGaz >= 1 )
  {
     mqtt(IDX,String(CompteurGaz));  
     CompteurGaz = 0 ;
  }


delay(250);

}


//***********************************
//************* Fonction domotique 
//***********************************


//////////// reconnexion MQTT

void reconnect() {
  // Loop until we're reconnected
  int timeout = 0; 
  while (!client.connected()) {
    
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "Dimmer";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      timeout++; // after 10s break for apply command 
      if (timeout > 2) {
          Serial.println(" try again next time ") ; 
          break;
          }

    }
  }
}

//// envoie de commande MQTT 
void mqtt(String idx, String value)
{
  reconnect();
  String nvalue = "0" ; 
  if ( value != "0" ) { nvalue = "2" ; }
  String message = "  { \"/idx\" : " + idx +" ,   \"svalue\" : \"" + value + "\",  \"nvalue\" : " + nvalue + "  } ";


  client.loop();
  client.publish("domoticz/in", message.c_str(), true);
  
  String jdompub = String(idx) + "/"+idx ;
  client.publish(jdompub.c_str() , value.c_str(), true);

  Serial.println("MQTT SENT");

}


