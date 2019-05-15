#define LORA 1

#if LORA == 1
  //includes for Lora
  #include <SPI.h>
  #include <LoRa.h>
  #include <sha1.h>
  #include <timer.h>
  #include <xxtea-iot-crypt.h>
  // PRO RF SAMD21 Pins 
  #define ss 12
  #define rst 7
  #define dio0 6
  #define sensorPin 2 
  #define transistorPin 5
  #define ledPin 3
#else
  //NB-IoT includes
  #include <TelenorNBIoT.h>
  #include <HardwareSerial.h>
  //NB-IOT Telenor pins.
  #define tx 16
  #define rx 17
  #define sensorPin 14 
  #define transistorPin 32
  #define ledPin 33
#endif
//Post Status
int NEW_POST = 1;
int REMOVED_POST = 2;
int NO_CHANGE = 3;

// konfigurasjon for timeout
long rcvTimeout = 5000;
int rcvTries = 3;

//kyptering 
String identifier = "2CFE";
// initial variabler
int postStatus = NO_CHANGE;
volatile bool receiveOk = false;
volatile int lastStatus;
int counter = 0;
auto timer = timer_create_default();

#if LORA == 0
  //starter NB-IoT og seriell kommunikasjon
  HardwareSerial MySerial(2);
  TelenorNBIoT nbiot("mda.ee", 242, 01);
  IPAddress remoteIP(172, 16, 15, 14);
  int REMOTE_PORT = 1234;
#endif

void setup() {
  pinMode(transistorPin, OUTPUT);
  digitalWrite(transistorPin, HIGH);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  //setup for LoRa radio
  #if LORA == 1
    xxtea.setKey(identifier);  
    LoRa.setPins(ss, rst, dio0);
  #else
  startNBIoT();
  #endif
  //starter intern pullup resistor for sensor
  pinMode(sensorPin, INPUT_PULLUP);
  delay(100);
  lastStatus = digitalRead(sensorPin);
  timer.every(5, readStatus);
  timer.in(20000, powerOff);    
}

void loop() {
  timer.tick();
}

bool powerOff(void *argument) {
  sendMessage();
  //Skriver trasistor pin lav for å skru av mikrokontrolleren
  digitalWrite(transistorPin, LOW);  
  while(1) {       
  }
}

bool readStatus(void *argument) {
  // 0/LOW for brudd på IR sensor, 1/HIGH for sikt imellom IR sensor
  bool currStatus = digitalRead(sensorPin);
  if(postStatus != NEW_POST && postStatus != REMOVED_POST && currStatus == lastStatus) {
    postStatus = 3;
  } else if (currStatus == LOW && lastStatus == HIGH) {
    postStatus = NEW_POST;
  } else if (currStatus == HIGH && lastStatus == LOW) {
    postStatus = REMOVED_POST;
  }
  lastStatus = currStatus;
  return true;
 }
#if LORA == 1
void notifyHub(String packetContent, String hashStr) {
  String encrypted = xxtea.encrypt(packetContent);
  //Send LoRa packet til mottaker
  LoRa.beginPacket();
  LoRa.print(encrypted);
  LoRa.endPacket();
  counter++;    
  receiveOk=true;
  long currentMillis = millis();
  //ser etter svar ifra hjemmehub
  while(receiveOk) {
    int packetSize = LoRa.parsePacket();
    if(packetSize) {
      while(LoRa.available()) {
        String replyString = LoRa.readString();
        String decrypted = xxtea.decrypt(replyString);
        delay(200);
        //sjekker om decryptert melding er lik den hash'ete strengen som ble sendt.
        if (decrypted == hashStr) {
          receiveOk = false;
          //skrur av mikrokontroll om 1 sekund
          timer.in(1000, powerOff);
        } else {
          delay(5000);
          //prøver på nytt å sende og motta melding
          notifyHub(packetContent, hashStr);
          }
      } 
      } else {
        //om det er over 5 sekunder siden den sist sendte en packet
        if(millis() - currentMillis >= rcvTimeout) {
            if(counter <= rcvTries) {
              //prøver igjen om det er sendt 3 eller færre packets
              notifyHub(packetContent, hashStr);
            } else {
              //skru av mikrokontroller om 1 sekund. ingen svar tilbake
              receiveOk=false;
              timer.in(1000, powerOff);
            }
        }
    }
  }
}
#else
void sendMsgIOT(String sendStatus){
    if (nbiot.isConnected()) {
      // om NB-IoT ble tilkoblet nettverket
      // sender melding til server
      nbiot.sendString(remoteIP, REMOTE_PORT, sendStatus);
      }
      else {
        // ikke tilkoblet enda, prøver igjen om 5 sekunder
        delay(5000);
    }
  }
#endif
#if LORA == 1
  void startLora() {
    while (!LoRa.begin(915E6)) {
      delay(500);
    }
    //lora konfigurasjon
    LoRa.setSpreadingFactor(12);
    LoRa.setTxPower(14);    
    LoRa.setSyncWord(0xF3);
  }
  //sha1 hash funksjon
  String hashFunc(String stringTohash) {
      char tmp[41];  
      memset(tmp, 0, 41);
      Sha1.init();
      Sha1.print(stringTohash);  
      uint8_t *hash = Sha1.result();
      for (int i=0; i<20; i++) {
        tmp[i*2] = "0123456789abcdef"[hash[i]>>4];
        tmp[i*2+1] = "0123456789abcdef"[hash[i]&0xf];
      }
    return tmp;
}
#else
  void startNBIoT(){
    MySerial.begin(9600, SERIAL_8N1, tx, rx);
    //starter seriell kommunikasjon med NB-IoT radio og lager en socket.
    while (!nbiot.begin(MySerial)) {
      delay(1000);
    }
    while (!nbiot.createSocket()) {
      delay(100);
    }  
  }
#endif


  
void sendMessage() {
  // Gjennomsnitt spenning over 50 lesinger
  int battLvl = readBatt(50);
  String pkgStr;
  #if LORA == 1
    String hashStr;
    long rng = random(200000);
    long milli = millis();
    //Streng sendt til hjemmehub
    pkgStr = String(rng) + String(milli)+":"+ "volt:" + String(battLvl) + ",status:" + String(postStatus)+",";
    hashStr = hashFunc(pkgStr);
    startLora();
    notifyHub(pkgStr, hashStr);
  #else
    int battLvl = readBatt(50);
    //Streng sendt til NB-IoT server
    pkgStr = String(postStatus) + ":" + String(battLvl);
    sendMsgIOT(pkgStr);
  #endif
}

int readBatt(int smoothLvl) {
  int sensorValue = 0;
  for (int i=0; i < smoothLvl; i++) {
     sensorValue += analogRead(A3);
  }
  return (sensorValue/smoothLvl);
}
