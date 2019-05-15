#include <TelenorNBIoT.h>
#include <HardwareSerial.h>

HardwareSerial MySerial(2);
TelenorNBIoT nbiot("mda.ee", 242, 01);

IPAddress remoteIP(172, 16, 15, 14);
int REMOTE_PORT = 1234;
volatile bool sendMsg = false;
void setup() {
  Serial.begin(9600);
  MySerial.begin(9600, SERIAL_8N1, 16, 17);
  //knapp
  pinMode(27, INPUT);
  //lys
  pinMode(15, OUTPUT);
  //interrupt på knapp
  attachInterrupt(digitalPinToInterrupt(27),knappFunksjon,FALLING);
  while (!nbiot.begin(MySerial)) {
    delay(1000);
  }
  while (!nbiot.createSocket()) {
    delay(100);
  }
}

void loop() {
  if(nbiot.isConnected()){
    //tilkoblet nettverk, lys på
    digitalWrite(15,HIGH);
  }
  if(sendMsg) {
    sendMessage();
  }
delay(500);
}

void sendMessage(){
  if (nbiot.isConnected()) {
    if (true == nbiot.sendString(remoteIP, REMOTE_PORT, "volt:520,status:2,dato:31")) {
    }
  } else {
    //ikke tilkoblet vent 5 sek.
    delay(5000);
  }
  sendMsg=false;
}
void knappFunksjon(){
  detachInterrupt(digitalPinToInterrupt(27));
  sendMsg=true;
  attachInterrupt(digitalPinToInterrupt(27),knappFunksjon,FALLING);  
}
