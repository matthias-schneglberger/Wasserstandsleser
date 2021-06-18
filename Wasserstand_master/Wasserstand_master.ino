#include <SoftwareSerial.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>


byte mac[] = { 0xA8, 0x61, 0x0A, 0xAE, 0x17, 0xC9 }; //adresse mac de la carte
IPAddress ip(10,0,0,3);
IPAddress gateway(10,0,0,138);
IPAddress subnet(255, 255, 255, 0);
//String serverIP = "192.168.1.169";
byte serverIP[] = { 10,0,0,2 };
int serverPort = 5000;

unsigned char ok_flag;
unsigned char fail_flag;

unsigned short lenth_val = 0;
unsigned char i2c_rx_buf[16];
unsigned char dirsend_flag = 0;

EthernetServer server(4711);
EthernetClient client;



#define TCAADDR 0x70
#define timeout 5000
//#define timeoutPin 2
#define switchPin_1 32 //Ueber einfahrt
#define switchPin_2 31 //Rechts neben Haustür
#define switchPin_3 43 //Boeschung
#define waterBetweenSensors 500
#define switchPin_4 42 //Unter Solar
#define switchPin_5 41 //Unter Einfahrt 
#define switchPin_6 35
#define switchPin_7 36
#define switchPin_8 37
#define transformatorRelayPin 48 //klackern kommt von hier
#define I2CPin_SENSOR_BIG 0 //Extern
#define I2CPin_SENSOR_SMALL 1
#define toggleWaterSourcePin 50 // HIGH = Big Tank
#define time_to_change_watersource_in_ms 120000 // 2 Minuten in eine Richtung!
#define pinDirektVerbTank 47
#define pinPumpenVent 33


byte maxTimeouts = 3;
int currentTimeouts = 0;
int delayBetween = 5000;
unsigned long lastMeasure = 0;
unsigned long tranformerFollowUpTimeUntil = 0;

String currentWaterLevel = "";


boolean currentlyFill = false; //kleinerTank wird befüllt?
boolean currentlyFillMax = false;
boolean pumpAutoMode = false;

boolean autoDetectingWaterSource = true;
boolean isCurrentWaterSourceSmallTank = true;

String jobs[128];
int letzteFreieStelle = 0;

unsigned long jobBegin = millis();
int jobLengthInMinutes = 0;
unsigned long jobUntil = 0;

void setup() {/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////SETUP


  /////////ETHERNET SERVER
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  
  Serial.begin(9600);
  Serial.println(Ethernet.localIP());
  Wire.begin();

  scanTcaDevices();
  

  //Pump auto stuff from Server:
  String pumpStateServer = getVarFromServer("pumpAutoMode");
  if(pumpStateServer.indexOf("1") >= 0){
    pumpAutoMode = true;
    Serial.println("Enabled pumpAutoMode by Server");
  }
  else if(pumpStateServer.indexOf("0") >= 0){
    pumpAutoMode = false;
    Serial.println("Disabled pumpAutoMode by Server");
  }
  else{
    //Try again
    pumpStateServer = getVarFromServer("pumpAutoMode");
    if(pumpStateServer.indexOf("1") >= 0){
      pumpAutoMode = true;
      Serial.println("Enabled pumpAutoMode by Server");
    }
    else if(pumpStateServer.indexOf("0") >= 0){
      pumpAutoMode = false;
      Serial.println("Disabled pumpAutoMode by Server");
    }
  }


  //AutoWaterSource stuff from Server:
  String waterSourceModeServer = getVarFromServer("autoDetectingWaterSource");
  if(waterSourceModeServer.indexOf("1") >= 0){
    autoDetectingWaterSource = true;
    Serial.println("Switched on autoWaterSourceMode by Server");
  }
  else if(waterSourceModeServer.indexOf("0") >= 0){
    autoDetectingWaterSource = false;
    Serial.println("Switched off autoWaterSourceMode by Server");
  }
  else{
    //Try again
    waterSourceModeServer = getVarFromServer("pumpAutoMode");
    if(waterSourceModeServer.indexOf("1") >= 0){
      autoDetectingWaterSource = true;
      Serial.println("Switched on autoWaterSourceMode by Server");
    }
    else if(waterSourceModeServer.indexOf("0") >= 0){
      autoDetectingWaterSource = false;
      Serial.println("Switched off autoWaterSourceMode by Server");
    }
  }

  

  //pinMode(timeoutPin, OUTPUT);

  pinMode(switchPin_1, OUTPUT);
  pinMode(switchPin_2, OUTPUT);
  pinMode(switchPin_3, OUTPUT);
  pinMode(switchPin_4, OUTPUT);
  pinMode(switchPin_5, OUTPUT);
  pinMode(switchPin_6, OUTPUT);
  pinMode(switchPin_7, OUTPUT);
  pinMode(switchPin_8, OUTPUT);
  pinMode(transformatorRelayPin, OUTPUT);

  pinMode(pinDirektVerbTank, OUTPUT);
  pinMode(pinPumpenVent, OUTPUT);


  digitalWrite(pinDirektVerbTank, 0);
  digitalWrite(pinPumpenVent, 0);

  digitalWrite(switchPin_1, 0);
  digitalWrite(switchPin_2, 0);
  digitalWrite(switchPin_3, 0);
  digitalWrite(switchPin_4, 0);
  digitalWrite(switchPin_5, 0);
  digitalWrite(switchPin_6, 0);
  digitalWrite(switchPin_7, 0);
  digitalWrite(switchPin_8, 0);
  digitalWrite(transformatorRelayPin, 0);

  selectSmallTankAsWaterSource();
}

void loop() { /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////LOOOOOOOOOOP
  
  int diff = millis() - lastMeasure;
  if (diff >= delayBetween) {
    Serial.println("measure!");
    lastMeasure = millis();
    
    String measured = measure();

    currentWaterLevel = measured;

    int grosserTank = getValue(measured, ';', 0).toInt();
    int kleinerTank = getValue(measured, ';', 1).toInt();
    int insgesamt = getValue(measured, ';', 2).toInt();

    //currentlyFill == kleinerTank soll befüllt werden

    if(pumpAutoMode){
                    if((kleinerTank > 1500 && grosserTank < 8000 && !currentlyFillMax) ||(kleinerTank > 1500 && grosserTank < 7500 && currentlyFillMax)){
                      currentlyFill = false;
                      currentlyFillMax = false;
                          //digitalWrite(pinDirektVerbTank, 0);
                          digitalWrite(pinPumpenVent, 1);
                          digitalWrite(transformatorRelayPin, 1);
                    }
                    
                    
                    else if((kleinerTank < 1000 && grosserTank > 300 && !currentlyFill) || (kleinerTank < 1000 && grosserTank > 500)){ ////// noch Platz im kleinen Tank? bei nicht füllen   ||    überhaupt noch platz?
                      currentlyFill = true;
                            //digitalWrite(pinDirektVerbTank, 1);
                          digitalWrite(pinPumpenVent, 0);
                           //digitalWrite(transformatorRelayPin, 1);
                    }
                    
                    else if(grosserTank >= 8000 || (kleinerTank < 1000 && grosserTank > 300 && currentlyFill)){
                      currentlyFillMax = true;
                      digitalWrite(pinDirektVerbTank, 0);
                          digitalWrite(pinPumpenVent, 0);
                    }
    
    }
    else{
      if(digitalRead(pinPumpenVent) == 1 && kleinerTank <= 0){
        digitalWrite(pinPumpenVent, 0);
        
      }
    }

    if(autoDetectingWaterSource){
      if(kleinerTank > 1000 && !isCurrentWaterSourceSmallTank){
        selectSmallTankAsWaterSource();
      }
      if(grosserTank > 1500 && kleinerTank < 200 && isCurrentWaterSourceSmallTank){
        selectBigTankAsWaterSource();
      }
      if(grosserTank < 200 && !isCurrentWaterSourceSmallTank){
        selectSmallTankAsWaterSource();
      }
    }
  }
  
  if(server.available()){  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////SERVER
    EthernetClient client = server.accept();
    Serial.println("New Client");
    String input = "";

    while(input.indexOf("do:") != 0){
      input = client.readString();
      }

    Serial.println(input);

    

    if(input.indexOf("howMuchW") >= 0){
      client.println(currentWaterLevel);
    }

    else if(input.indexOf("clearJobList") >= 0){
      //memset(jobs, "-", sizeof(jobs));

      jobLengthInMinutes = 0;
      jobUntil = 0;
      
      for(int i = 1; i < letzteFreieStelle; i++){
        if(!jobs[i].equals("")){
          jobs[i] = "";
          }
        }
      
      client.println("ok");
      }

    else if(input.indexOf("getState") >= 0){
      client.println(String(!digitalRead(pinDirektVerbTank)) + ";" + String(!digitalRead(pinPumpenVent)));
      }

    else if(input.indexOf("fillBigTank") >= 0){
      client.println("ok");
      digitalWrite(pinDirektVerbTank, 0);
      digitalWrite(pinPumpenVent, 1);
      digitalWrite(transformatorRelayPin, 1);
      }

    else if(input.indexOf("fillLowTank") >= 0){
      client.println("ok");
      digitalWrite(pinDirektVerbTank, 1);
      digitalWrite(pinPumpenVent, 0);
      digitalWrite(transformatorRelayPin, 1);
      }

    else if(input.indexOf("stopPump") >= 0){
      client.println("ok");
      digitalWrite(pinDirektVerbTank, 0);
      digitalWrite(pinPumpenVent, 0);
      }
      
    else if(input.indexOf("pumpState") >= 0){
      client.println(String(pumpAutoMode));
      }

    else if(input.indexOf("newPump") >= 0){
      client.println("ok");
      if(input.indexOf("true") >= 0){
        pumpAutoMode = true;
        setVarOnServer("pumpAutoMode","1");
        }
      else{
        pumpAutoMode = false;
        setVarOnServer("pumpAutoMode","0");
        }
      }      

    else if(input.indexOf("getJobs") >= 0){
      String rueckgabe = "";
      for(int i  = 0; i <= letzteFreieStelle; i++){
        rueckgabe += jobs[i] + "###";
      }
      client.println(rueckgabe);
      }

    else if(input.indexOf("addToList:") >= 0){
      String cut = input.substring(10);

      jobs[letzteFreieStelle] = cut;
      letzteFreieStelle++;

      if(letzteFreieStelle == 1){
        jobBegin = millis();
        jobLengthInMinutes = getValue(jobs[letzteFreieStelle-1], '#', 1).toInt();
        jobUntil = jobBegin + (60000 * jobLengthInMinutes);
        driveJob(jobs[0]);
        }

      

      Serial.println(jobs[letzteFreieStelle-1]);
      Serial.println(jobLengthInMinutes);

      client.println("ok");
      
      }
    else if(input.indexOf("waterSourceState") >= 0){
      client.println(String(autoDetectingWaterSource));
      }
    else if(input.indexOf("newAutoWaterSourceState") >= 0){
          client.println("ok");
          if(input.indexOf("true") >= 0){
            autoDetectingWaterSource = true;
            setVarOnServer("autoDetectingWaterSource","1");
            }
          else{
            autoDetectingWaterSource = false;
            setVarOnServer("autoDetectingWaterSource","0");
            }
          }   
     else if(input.indexOf("waterSourceSmall") >= 0){
       selectSmallTankAsWaterSource();
      client.println("ok");
      }   
     else if(input.indexOf("waterSourceBig") >= 0){
       selectBigTankAsWaterSource();
      client.println("ok");
      }
    else{
      client.println("nothing found " + input);
      }

      client.stop();


      /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    
    //client.println("Received:" + test);
    }

  

  

  if(letzteFreieStelle != 0){
    unsigned long tmp = millis();
    if(tmp > jobUntil){
      stopJob(jobs[0]);
      jobs[0] = "";
      

      for(int i = 1; i <= letzteFreieStelle; i++){
        jobs[i-1] = jobs[i];
        }
      jobs[letzteFreieStelle] = "";
      letzteFreieStelle--;


      Serial.println("jasf" + String(jobBegin));
      jobBegin = millis();
      jobLengthInMinutes = getValue(jobs[0], '#', 1).toInt();
      jobUntil = jobBegin + (60000 * jobLengthInMinutes);


      Serial.println("millis" + String(millis()));
      Serial.println("joBegin" + String(jobBegin));
      Serial.println("Job Length: " + String(jobLengthInMinutes));
      Serial.println("Job until: " + String((jobBegin + (60000 * jobLengthInMinutes))));
      //Serial.println(getValue(jobs[0], ";##", 1));
       
      driveJob(jobs[0]);
      
      
    }

    
    }

    if(digitalRead(switchPin_1)==0 &&
       digitalRead(switchPin_2)==0 &&
       digitalRead(switchPin_3)==0 &&
       digitalRead(switchPin_4)==0 &&
       digitalRead(switchPin_5)==0 &&
       digitalRead(switchPin_6)==0 &&
       digitalRead(switchPin_7)==0 &&
       digitalRead(switchPin_8)==0 &&
       digitalRead(pinPumpenVent)==0 &&
       digitalRead(pinDirektVerbTank)==0 &&
       millis() >= tranformerFollowUpTimeUntil
       ){
      digitalWrite(transformatorRelayPin, 0);
    }

}

void driveJob(String jobStr){
    Serial.println("drive Job: " + jobStr);

    if(jobStr.indexOf("Unit1;") > 0){
      digitalWrite(switchPin_1, 1);
      Serial.println("switchOnPin_1");

      digitalWrite(transformatorRelayPin, 1);
      }
    if(jobStr.indexOf("Unit2;") > 0){
      digitalWrite(switchPin_2, 1);
      Serial.println("switchOnPin_2");

      digitalWrite(transformatorRelayPin, 1);
      }
    if(jobStr.indexOf("Unit3;") > 0){
      digitalWrite(switchPin_3, 1);
      Serial.println("switchOnPin_3");

      digitalWrite(transformatorRelayPin, 1);
    }
    if(jobStr.indexOf("Unit4;") > 0){
      digitalWrite(switchPin_4, 1);
      Serial.println("switchOnPin_4");

      digitalWrite(transformatorRelayPin, 1);
      }

    if(jobStr.indexOf("Unit5;") > 0){
      digitalWrite(switchPin_5, 1);
      Serial.println("switchOnPin_5");

      digitalWrite(transformatorRelayPin, 1);
      }
    if(jobStr.indexOf("Unit6;") > 0){
      digitalWrite(switchPin_6, 1);
      Serial.println("switchOnPin_6");

      digitalWrite(transformatorRelayPin, 1);
      }
    if(jobStr.indexOf("Unit7;") > 0){
      digitalWrite(switchPin_7, 1);
      Serial.println("switchOnPin_7");

      digitalWrite(transformatorRelayPin, 1);
    }
    if(jobStr.indexOf("Unit8;") > 0){
      digitalWrite(switchPin_8, 1);
      Serial.println("switchOnPin_7");

      digitalWrite(transformatorRelayPin, 1);
      }
    
    
  }
void stopJob(String jobStr){
  Serial.println("stop Job" + jobStr);


  if(jobStr.indexOf("Unit1;") > 0){
      digitalWrite(switchPin_1, 0);
      Serial.println("switchOffPin_1");
      }
    if(jobStr.indexOf("Unit2;") > 0){
      digitalWrite(switchPin_2, 0);
      Serial.println("switchOffPin_2");
      }
    if(jobStr.indexOf("Unit3;") > 0){
      digitalWrite(switchPin_3, 0);
      Serial.println("switchOffPin_3");
    }
    if(jobStr.indexOf("Unit4;") > 0){
      digitalWrite(switchPin_4, 0);
      Serial.println("switchOffPin_4");
      }

    if(jobStr.indexOf("Unit5;") > 0){
      digitalWrite(switchPin_5, 0);
      Serial.println("switchOffPin_5");
      }
    if(jobStr.indexOf("Unit6;") > 0){
      digitalWrite(switchPin_6, 0);
      Serial.println("switchOffPin_6");
      }
    if(jobStr.indexOf("Unit7;") > 0){
      digitalWrite(switchPin_7, 0);
      Serial.println("switchOffPin_7");
    }
    if(jobStr.indexOf("Unit8;") > 0){
      digitalWrite(switchPin_8, 0);
      Serial.println("switchOffPin_7");
      }
  }





String measure() {  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////MEASURE
  lastMeasure = millis();

  int waterLevel_bigTank = getExternWaterLevel();
  int waterLevel_smallTank = getWaterLevel();
  int waterLevel = waterLevel_bigTank + waterLevel_smallTank;

  String waterLevel_str = String(waterLevel_bigTank) + ";" + String(waterLevel_smallTank) + ";" + String(waterLevel);

  Serial.println(waterLevel_str);
  return waterLevel_str;
}

int getExternWaterLevel(){
  int currentWaterLevel = 0;
  byte anzTanks = 8;
  int abstand = 9;
  int entfernung2 = ReadDistance(I2CPin_SENSOR_BIG);
  Serial.println("Big tank Distance in cm: ");
  Serial.println(entfernung2);
  if (entfernung2 > 100+abstand || entfernung2 <= 0) 
  {
  //return
  }
  else 
  {
    int level = 100 - (entfernung2 - abstand);
    currentWaterLevel = level * 10 * anzTanks;
  }

 return currentWaterLevel;
}
int getWaterLevel(){
  int currentWaterLevel = 0;
  int anzTanks = 4;
  int abstand = 60;
  int entfernung = ReadDistance(I2CPin_SENSOR_SMALL); 
  Serial.println("Distance in cm: ");
  Serial.println(entfernung);
  if (entfernung > 100+abstand || entfernung <= 0) 
  {
  //return
  }
  else 
  {
    int level = 100 - (entfernung - abstand);
    currentWaterLevel = level * 10 * anzTanks;
  }

 return currentWaterLevel;
}



void tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}
void SensorRead(unsigned char addr, unsigned char* datbuf, unsigned char cnt){
  unsigned short result = 0;
  // step 1: instruct sensor to read echoes
  Wire.beginTransmission(82); // transmit to device #82 (0x52)
  // the address specified in the datasheet is 164 (0xa4)
  // but i2c adressing uses the high 7 bits so it's 82
  Wire.write(byte(addr));      // sets distance data address (addr)
  Wire.endTransmission();      // stop transmitting
  // step 2: wait for readings to happen
  delay(1);                   // datasheet suggests at least 30uS
  // step 3: request reading from sensor
  Wire.requestFrom(82, cnt);    // request cnt bytes from slave device #82 (0x52)
  // step 5: receive reading from sensor
  if (cnt <= Wire.available()) { // if two bytes were received
    *datbuf++ = Wire.read();  // receive high byte (overwrites previous reading)
    *datbuf++ = Wire.read(); // receive low byte as lower 8 bits
  }
}
int ReadDistance(int sensor) {
  tcaselect(sensor);
  delay(150);
  SensorRead(0x00, i2c_rx_buf, 2);
  lenth_val = i2c_rx_buf[0];
  lenth_val = lenth_val << 8;
  lenth_val |= i2c_rx_buf[1];
  return lenth_val/10;
}
void scanTcaDevices(){
   for (uint8_t t = 0; t < 8; t++) {
    tcaselect(t);
    Serial.print ("Port #");
    Serial.println(t);

    for (uint8_t addr = 0; addr <= 127; addr++) {
      
      // Don't report on the TCA9548A itself!
      if (addr == TCAADDR) continue;

      // See whether a device is on this address
      Wire.beginTransmission(addr);

      // See if something acknowledged the transmission
      int response = Wire.endTransmission();
      if (response == 0) {
        Serial.print("Found I2C 0x");  Serial.println(addr, HEX);
      }
    }

    // Slow the loop scanner down a bit
    delay(1000);
  }
  Serial.println("\nScan completed.");
}

void selectBigTankAsWaterSource(){
  Serial.println("Changing WaterSource to BigTanks");
  digitalWrite(transformatorRelayPin, 1);
  digitalWrite(toggleWaterSourcePin, 1);
  tranformerFollowUpTimeUntil = time_to_change_watersource_in_ms*2 + millis();
  isCurrentWaterSourceSmallTank = false;
}
void selectSmallTankAsWaterSource(){
  Serial.println("Changing WaterSource to SmallTanks");
  digitalWrite(transformatorRelayPin, 1);
  digitalWrite(toggleWaterSourcePin, 0);
  tranformerFollowUpTimeUntil = time_to_change_watersource_in_ms*2 + millis();
  isCurrentWaterSourceSmallTank = true;
}



String getVarFromServer(String key){


  if(client.connect(serverIP, serverPort)) {
    // if connected:
    // make a HTTP request:
    // send HTTP header
    client.println("GET /var/" + key + " HTTP/1.1");
    client.println(); // end HTTP header

    String fullText = "";
    while(client.connected()) {
      if(client.available()){
        // read an incoming byte from the server and print it to serial monitor:
        fullText += String(char(client.read()));
        
      }
    }

    // the server's disconnected, stop the client:
    client.stop();


    int indexOf2nz = fullText.indexOf("\r\n\r\n");

    Serial.println("From Server: " +fullText.substring(indexOf2nz+4));

    return (fullText.substring(indexOf2nz+4));
    
  } else {// if not connected:
    Serial.println("No connection to Server");
  }

}
void setVarOnServer(String key, String value){ 
  
  if(client.connect(serverIP, serverPort)) {
    // if connected:
    // make a HTTP request:
    // send HTTP header
    client.println("POST /var/" + key + "/" + value + " HTTP/1.1");
    client.println(); // end HTTP header

    client.stop();
    
  } else {// if not connected:
  }
}




String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
