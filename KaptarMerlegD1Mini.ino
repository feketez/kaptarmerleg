// ESP8266 DS18B20 ArduinoIDE Thingspeak IoT Example code
// http://vaasa.hacklab.fi
//
// https://github.com/milesburton/Arduino-Temperature-Control-Library
// https://gist.github.com/jeje/57091acf138a92c4176a
//
// https://github.com/feketez/kaptarmerleg

#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 13                                 // DS18B20              GPIO13 esetén 13 (D7)
#define merleg_adat 14                                  // Mérleg adat          GPI14 esetén 14 (D5)   
#define merleg_indit 12                                 // Mérleg indító jele   GPI12 esetén 12 (D6)
#define wakeup_pin 16                                   // GPIO16
#define programming 0                                   // GPIO0

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

const char* host = "api.thingspeak.com";                // Your domain  

String ApiKey = "********************";                 //  A Thingspeekről kell beírni a Te apikey-edet.
String path = "/update?key=" + ApiKey + "&field1=";     //  Hozz létre a thingspeeken 6 mezőt
String path2 = "&field2=";  
String path3 = "&field3=";
String path4 = "&field4=";
String path5 = "&field5=";
String path6 = "&field6=";
const char* ssid = "******";                           // A telefonodon beállított SSID
const char* pass = "********";                         // és jelszó
char temperatureString[6];
float sulyf;   
long inditIdo;         
long wakeUpTime;      
unsigned long sleepTime;                       

void wifiConnectWithTimeout(void) {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  int i=0;
  while (WiFi.status() != WL_CONNECTED && i<60) {
    delay(100);
    Serial.print(".");
    i++;
  }
  if (WiFi.status() == WL_CONNECTED) {
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("signal strength (RSSI):");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  } else {
  Serial.println("");
  Serial.println("WiFi not connected!!!!!!!!!!!!!!!!");
  }
}

float getTemperature() {
  float temp;
  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);
    delay(100);
  } while (temp == 85.0 || temp == (-127.0));
  return temp;
}


float sulymeres(void)   {
  boolean elozoRead, aktualisRead;
  unsigned long idoBelyeg,elozoIdoBelyeg, aktualisIdoBelyeg, idotartam;
  int i, b=40, cheksum; 
  int indulasraVar = 1500;                                // ms-umban kell megadni
  int indulasraVarCiklus = 40000.0/1113.0*indulasraVar;
  unsigned char sulyt[5];
  
  sulyf=-4;                                               // Hibakód: a mérés nem indul el
  i=0;
  idoBelyeg=millis();
  while( i<3500) {                                        // Vár 100ms magas szintre - a mérleg pihen
   yield();
   if (digitalRead(merleg_adat) == LOW ) i=0;
   delayMicroseconds(20);    
   i++;
  }
  Serial.print("Varakozas: ");
  Serial.println(millis() - idoBelyeg);

  i=0;
  idoBelyeg=millis();
  digitalWrite(merleg_indit, LOW);                        // Mérleg indítása
  while( i<indulasraVarCiklus && digitalRead(merleg_adat)) {           // kb. 1000ms 
   yield();
   delayMicroseconds(20);    
   i++;
  }
  digitalWrite(merleg_indit, HIGH); 
  inditIdo = millis() - idoBelyeg;
  Serial.print("Inditas: ");
  Serial.println(inditIdo);
  
  
  
  if( i < indulasraVarCiklus )   sulyf = -3;                               // Hibakód: hibás az első két bájt, vagy ingadozik a mérés

  elozoRead=HIGH;
  idoBelyeg = micros();
  elozoIdoBelyeg = idoBelyeg;
  Serial.println("**********************   Meres indul   ************************");
  Serial.println(idoBelyeg);
  while( micros() - idoBelyeg < 15000000 && sulyf < 0 ){                   // 15 másodpercig jönnek az adatok
    yield();
    aktualisRead = digitalRead(merleg_adat);
    aktualisIdoBelyeg = micros();
    if (aktualisRead != elozoRead)  {
      elozoRead = aktualisRead;
      if (aktualisIdoBelyeg - elozoIdoBelyeg > 350 && aktualisRead == LOW) {
        idotartam = aktualisIdoBelyeg - elozoIdoBelyeg;
        if (idotartam > 50000) { 
          b=0;
          sulyt[0]=0 ; sulyt[1]=0 ;sulyt[2]=0 ;sulyt[3]=0 ;sulyt[4]=0 ;
        }
        else {
          if (b<39) {
            sulyt[b/8] = sulyt[b/8] << 1;
            if (idotartam < 650) {
              sulyt[b/8]=sulyt[b/8]|1;
            }
          b++; 
          } 
          if (b==39) { 
            sulyt[b/8] = sulyt[b/8] << 1;
            b++;
            for ( i=0 ; i<=4 ; i++ ) {
              Serial.print(sulyt[i], HEX);
              Serial.print(" ");
            }
            if ((sulyt[0]==0xAB && sulyt[1]==0x8C)||(sulyt[0]==0xAB && sulyt[1]==0x08)){
              cheksum = 0;
              Serial.print("Cheksum ");
              for ( i=0 ; i<=3 ; i++ ) {                   
                cheksum += sulyt[i];                           // Cheksum kiszámítása
                cheksum %= 0xFF;
                Serial.print(cheksum, HEX);
                Serial.print(" ");
              }
              cheksum &= ~1;       
              Serial.print(cheksum, HEX);
              Serial.print(" ");
              if (cheksum == sulyt[4]) {
                sulyf=(sulyt[2]*256+sulyt[3])/10.0;
              }
              else sulyf=-1;                                    // Hibakód: Cheksum hiba
            }
            else sulyf=-2;
            Serial.println(sulyf);
          }
        }
      }
      elozoIdoBelyeg=aktualisIdoBelyeg;
    }
  }
  Serial.println("**********************   Meres vege   *************************");
  Serial.println( (micros()-idoBelyeg)/1000 );
  return sulyf;
}




void setup() {
  WiFi.forceSleepBegin();                                          //  modem sleep
  delay(1);
  wakeUpTime=millis();
  sleepTime= 50ul * 1000ul * 1000ul;                                // 50 másodperc
  Serial.begin(115200);
  Serial.println("");
  Serial.println("");
  Serial.println("");
  
  Serial.println("Mar fent vagyok!");
  pinMode(programming, INPUT);
  pinMode(wakeup_pin, INPUT);
  digitalWrite(merleg_indit, HIGH);             
  pinMode(merleg_indit, OUTPUT);  
  pinMode(merleg_adat, INPUT_PULLUP);  
  pinMode(A0, INPUT);
}

void loop() {
  long idoBelyeg;
  float akkufesz;
  float temperature; 
  float korrSuly;
  int korrSulyInt;

  sulymeres();                                                      // Súly mérés
  Serial.print("Kaptarsuly: ");  
  Serial.print(sulyf);
  Serial.println(" kg");
  
  WiFi.forceSleepWake();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  
  idoBelyeg = millis();
  DS18B20.begin();  
  Serial.print("Homero inicializalas ido: ");
  Serial.print(millis()-idoBelyeg);
  Serial.println("ms");
  
  idoBelyeg = millis();
  temperature=getTemperature();                                                  // Hőmérséklet mérés
  dtostrf(temperature, 2, 2, temperatureString);
  Serial.print("Homeres ido: ");
  Serial.print(millis()-idoBelyeg);
  Serial.println(" ms");
  Serial.print("Homereseklet: ");  
  Serial.print(temperature);
  Serial.println(" Celsius");

 idoBelyeg = millis();
  akkufesz = (analogRead(A0)*4.07)/958.0;            // 4.07 V-nál mért 958-at
  Serial.print("Akkufesz: ");  
  Serial.print(akkufesz);
  Serial.println(" V");
  Serial.print("Akkufesz meres: ");
  Serial.print(millis()-idoBelyeg);
  Serial.println(" ms");

  korrSulyInt=(10.0*sulyf+0.5+(temperature-24.0)/11.4);
  korrSuly=korrSulyInt/10.0;

  Serial.print("Korrigalt suly: ");
  Serial.print(korrSuly);
  Serial.println(" kg");
  
  idoBelyeg = millis();
  wifiConnectWithTimeout();
  Serial.print("Wifi connect ido: ");
  Serial.print(millis()-idoBelyeg);
  Serial.println(" ms");
  
  idoBelyeg=millis();
  WiFiClient client;
  const int httpPort = 80;
  int i;
  i=1;
  while (WiFi.status() == WL_CONNECTED && !client.connect(host, httpPort) && i<4) {
    Serial.print("connection failed ");
    Serial.println(i);
    i++;
  }

  if (WiFi.status() == WL_CONNECTED && i<4) {
    sleepTime = (61ul * 60ul + 30) * 1000ul * 1000ul;                             // 59 perc 58 másodperc 61ul * 60ul + 32
    }                    
  
  client.print(String("GET ") + path + sulyf + 
                path2  + temperatureString + path3 + inditIdo + path4 + akkufesz + path5 + ((millis()-wakeUpTime)/1000.0) + path6 + korrSuly + 
                " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: keep-alive\r\n\r\n");
  Serial.print(String("GET ") + path + sulyf + 
                path2  + temperatureString + path3 + inditIdo + path4 + akkufesz + path5 + ((millis()-wakeUpTime)/1000.0) + path6 + korrSuly + 
                " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: keep-alive\r\n\r\n");
  Serial.print("Wifi kuldes ido: ");
  Serial.print(millis()-idoBelyeg);
  Serial.println(" ms");
  Serial.print("Osszes ido: ");
  Serial.print((millis()-wakeUpTime)/1000.0);
  Serial.println(" s");
  
  Serial.println("Megyek aludni, jo ejszakat!");
  Serial.println();
  Serial.println();
  Serial.println();

  pinMode(merleg_indit, INPUT_PULLUP);  
  pinMode(merleg_adat,  INPUT_PULLUP);  
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);

  ESP.deepSleep(sleepTime , WAKE_RF_DEFAULT);  

  delay(1000);
  Serial.println("Ezt már nem írom ki, mert alszom.");
}


