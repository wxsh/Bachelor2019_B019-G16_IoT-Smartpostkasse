#define LORA 1

//includes for skjerm,wifi,webserver og aksesspunkt funksjonalitet
#include "SH1106Wire.h"
#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#if LORA == 1
	//includes for LoRa
	#include <SPI.h>
	#include <LoRa.h>
	#include <Hash.h>
	#include <HTTPClient.h>
	#include <xxtea-iot-crypt.h>
	//Pins for Lora
	#define ss 14  // CS
	#define rst 27 //reset
	#define dio0 33 //G1
#else
	//Firebase include + host og auth variabler
	#include <IOXhop_FirebaseESP32.h>
	#define FIREBASE_HOST "iot-smartpostkasse.firebaseio.com"
	#define FIREBASE_AUTH %%FIREBASE_AUTH_KEY_HERE%%
#endif

//post statuser
#define NEW_POST 1
#define REMOVED_POST 2
#define NO_CHANGE 3

#define ledPin 12
//start display med port 23,22(SDA,SCL)
SH1106Wire  display(0x3c, 23, 22);
int counter = 0;
String identifier = "2CFE";
int lastPostStatus;
//bruker som firebase meldingene blir sendt til
const char* USERNAME = "sensorbruker";

WebServer Server;
AutoConnect Portal(Server);
AutoConnectConfig Config; 

//Sender brukerene til /_ac/config som er autoconnect siden for å koble til nytt wifi
void startPage() {
  Server.sendHeader("Location", String("http://") + Server.client().localIP().toString() + String("/_ac/config"));
  Server.send(302, "text/plain", "");
  Server.client().flush();
  Server.client().stop();
}
//stopper aksesspunkt og webserver
void stopServer(){
  Serial.println("Stopped server");
  Server.client().stop();
  Server.stop();
  Server.close();
  WiFi.softAPdisconnect(true);  
}
#if LORA == 1
//sender data til firebase
void postRecived(String payload){
  HTTPClient http;
  http.begin("https://us-central1-iot-smartpostkasse.cloudfunctions.net/postReceived2?username=" + String(USERNAME));
  http.addHeader("Content-Type", "text/plain");
  int httpResponseCode = http.POST(payload);
  if(httpResponseCode>0){ 
    String response = http.getString();
  }
  http.end();
}
#endif
void setup() {
  Serial.begin(115200);
  //starter skjerm
  display.init();
  display.resetDisplay();
  pinMode (ledPin, OUTPUT);  
  #if LORA == 1
	  LoRa.setPins(ss, rst, dio0);
	  while (!LoRa.begin(915E6)) {
		delay(500);
	  }
	  //lora konfigurasjon og set key for kyrptering
	  xxtea.setKey(identifier);
	  LoRa.setSpreadingFactor(12);
	  LoRa.setTxPower(14);
	  LoRa.setSyncWord(0xF3);  
  #endif
  Config.autoReconnect = true; //kobler til tidligere koblet til nettverk
  Config.apid = "Smartpostkasse"; //ssid for aksesspunktet. default passord er 12345678
  Config.portalTimeout = 60000; //ms for å koble seg til.
  Portal.home("/start");
  Portal.config(Config);
  //starter webserver med startPage funksjonen
  Server.on("/", startPage);
  Server.on("/start", startPage);
  drawStart();
  connectToAP();
  // Sjekker om mikrokontrolleren er tilkoblet wifi eller ikke
  if(WiFi.status() == WL_CONNECTED){
    drawWaitingForPost();
  } else {
    drawNoWifi();
  }
}

void loop() {
  reciverMode();
  #if LORA == 0
	delay(1000 * 60 * 15); //hvert 15min, 1000 ms * 60 s * 15 min
  #endif
}

void reciverMode(){
  String LoRaData;
  #if LORA == 1
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      while (LoRa.available()) {
        String encrypted = LoRa.readString();
        LoRaData = xxtea.decrypt(encrypted);
      }
      //henter ut og regner ut batteriprosent fra mottatt data
      float battteriProsent = batteryCalc(LoRaData);
      //henter ut poststatus fra mottatt data
      String postStatus = postStatusCheck(LoRaData);
      //lager ny string for å sende til firebase, fjerner unødvendig data fra lora stringen.
      String newLoRaData = "volt:"+ getVoltage(LoRaData) +",status:"+postStatusNumber(LoRaData)+",dato:0"; //dato ikke implementer, hentes ifra applikasjonen
      //oppdater skjerm med ny status og batteriprosent
      drawScreen(postStatus,battteriProsent);
      //sender svar til postkassenhet med samme stringen med sha1 hash
      SendReply(xxtea.encrypt(sha1(LoRaData)));
      //om enheten er koblet til wifi sendes den nye stringen til firebase og videre til appen.
      if(WiFi.status() == WL_CONNECTED){
          Portal.handleClient();
          postRecived(newLoRaData);
      }
    }
  #else
    // om NB-IoT, skjekker firebase om noen av meldingen er satt til true og innhenter disse.
    LoRaData = checkFirebase();
    if(LoRaData != "error"){
      String postStatus = postStatusCheck(LoRaData);
      float battteriProsent = batteryCalc(LoRaData);
      drawScreen(postStatus,battteriProsent);
    }  
  #endif
}
#if LORA == 1
//kode for sending av lora melding
void SendReply(String LoRaData){
      LoRa.beginPacket();
      LoRa.print(LoRaData);
      LoRa.endPacket();
}
#else
String checkFirebase(){
    if (Firebase.failed()) {
      return "error";
  }
  //Henter meldinger fra firebase og setter dem til false for å ikke hente samme melding flere ganger.
  if (Firebase.getBool("users/" + String(USERNAME) + "/hasNewMessage/bool")){
    Firebase.setBool("users/" + String(USERNAME) + "/hasNewMessage/bool", false);
    String messageFromLte = (Firebase.getString("users/" + String(USERNAME) + "/hasNewMessage/lastMessage"));
    return messageFromLte; 
  } 
  return "error";
}
#endif
void connectToAP(){
      if (Portal.begin()) {
        //stopper aksesspunkt
        stopServer();
        #if LORA == 0
			//starter firebase om det er NB-IoT
			Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
        #endif
    }
}
void drawScreen(String postStatus,float batLvl){
  display.setFont(ArialMT_Plain_16);
  display.resetDisplay();
  display.drawString(0, 0, postStatus);
  display.drawString(0, 20, String(batLvl) + "% Batteri");
  display.drawProgressBar(0,40,80,10,uint8_t(batLvl));
  display.display();
}
void drawStart(){
  display.setFont(ArialMT_Plain_10);
  display.resetDisplay();
  display.drawString(0, 0, "For å starte WIFI koble til");
  display.drawString(0, 15, "Nettverk: Smartpostkasse" );
  display.drawString(0, 30, "Passord:12345678" );  
  display.drawString(0, 45, "Naviger til: 192.168.244.1" );
  display.display();
}
void drawWaitingForPost(){
  display.setFont(ArialMT_Plain_16);
  display.resetDisplay();
  display.drawString(0, 0, "Koblet til WIFI.");
  display.drawString(0, 20, "Venter på post" );
  display.display();
}
void drawNoWifi(){
  display.setFont(ArialMT_Plain_10);
  display.resetDisplay();
  display.drawString(0, 0, "Fikk ikke koblet til WIFI.");
  display.drawString(0, 15, "Venter på post." );
  display.drawString(0, 30, "For å prøve på nytt restart" );
  display.drawString(0, 45, "enheten." );
  display.display();
}
//henter ut spenningen ifra mottatt melding, henter ut alt imellom 'volt:___,' ofte 3 tall, men kan hente ut mer eller mindre
float batteryCalc(String LoRaData) {
  int batLvlPlacement = LoRaData.indexOf("volt:");
  int komma = LoRaData.indexOf(',');
  String batLvlString = LoRaData.substring(batLvlPlacement + 5,komma);
  int batLvl = batLvlString.toInt();
  //regner ut for interval 4.2 til 3.0 v.
  float batPercent = (((((batLvl/1023.0)*6.6) - 3.0)*100.0)/1.2);
  if(batPercent > 100){
    return 100.0;
  } else if(batPercent < 0){
    return 0.0;
  }else {
    return batPercent;
  }
}
//henter ut spenningen ifra mottatt melding, henter ut ___ ifra 'volt:___,' ofte 3 tall, men kan hente ut mer eller mindre
String getVoltage(String LoRaData) {
  int batLvlPlacement = LoRaData.indexOf("volt:");
  int komma = LoRaData.indexOf(',');
  String batLvlString = LoRaData.substring(batLvlPlacement + 5,komma);
  return batLvlString;
}
//henter ut poststatus ifra mottatt melding, henter ut _ ifra 'status:_,' ofte 1 tall, men kan hente ut mer eller mindre
String postStatusNumber(String LoRaData){
  int postStatusPlacement = LoRaData.lastIndexOf("status:");
  int komma = LoRaData.indexOf(",",postStatusPlacement);
  String postStatusString = LoRaData.substring(komma -1,komma);
  return postStatusString;
}
//henter ut poststatus ifra mottatt melding, henter ut _ ifra 'status:_,' ofte 1 tall, men kan hente ut mer eller mindre
//returnerer tekst som går inn på skjerm via drawScreen funksjonen
String postStatusCheck(String LoRaData){
  int postStatusPlacement = LoRaData.lastIndexOf("status:");
  int komma = LoRaData.indexOf(",",postStatusPlacement);
  String postStatusString = LoRaData.substring(komma -1,komma);
  int postStatus = postStatusString.toInt();
  if(postStatus == NEW_POST){ //post motatt
    lastPostStatus = NEW_POST;
    digitalWrite(ledPin, HIGH); // led på
    return "Post ankommet!";   
  } else if(postStatus == REMOVED_POST) { // post fjernet
    lastPostStatus = REMOVED_POST;
    digitalWrite(ledPin, LOW); // led av
    return "Post fjernet!";
  } else if(postStatus == NO_CHANGE){ //postkassen er tom
    if(lastPostStatus == NEW_POST){
      return "Post ankommet!"; 
    } else {    
      return "Ingen post!";
    }
  }else { //annet
    return "Ukjent postkode";
 }
}
