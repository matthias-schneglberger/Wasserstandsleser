#include <SoftwareSerial.h>
#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xAC, 0x68 }; //adresse mac de la carte
IPAddress ip( 192, 168, 1, 103 );
IPAddress subnet( 255, 255, 255, 0 );

EthernetServer server(4711);

int trigger=22; 
int echo=23; 
long dauer=0; 
long entfernung=0; 


int trigger2=24;  //2 => extern!
int echo2=25; 
long dauer2=0; 
long entfernung2=0; 


#define timeout 5000
#define timeoutPin 2
#define waterBetweenSensors 500
#define switchPin_1 30
#define switchPin_2 31
#define switchPin_3 32
#define switchPin_4 33
#define switchPin_5 34
#define switchPin_6 35
#define switchPin_7 36
#define switchPin_8 37

#define btRX 4
#define btTX 3

#define pinDirektVerbTank 47
#define pinPumpenVent 49


int maxTimeouts = 3;
int currentTimeouts = 0;
int delayBetween = 5000;
int lastMeasure = 0;

String currentWaterLevel = "";


boolean currentlyFill = false; //kleinerTank wird befüllt?
boolean pumpAutoMode = true;

SoftwareSerial BTSerial(5, 6); // RX, TX


String jobs[128];
int letzteFreieStelle = 0;

unsigned long jobBegin = millis();
int jobLengthInMinutes = 0;
unsigned long jobUntil = 0;

void setup() {/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////SETUP

  Ethernet.begin(mac, ip);
  server.begin();
  


  Serial.begin(9600);
  BTSerial.begin(9600);

  Serial.println(Ethernet.localIP());

  pinMode(timeoutPin, OUTPUT);

  pinMode(switchPin_1, OUTPUT);
  pinMode(switchPin_2, OUTPUT);
  pinMode(switchPin_3, OUTPUT);
  pinMode(switchPin_4, OUTPUT);

  pinMode(pinDirektVerbTank, OUTPUT);
  pinMode(pinPumpenVent, OUTPUT);


  digitalWrite(pinDirektVerbTank, 1);
  digitalWrite(pinPumpenVent, 1);

  digitalWrite(switchPin_1, 1);
  digitalWrite(switchPin_2, 1);
  digitalWrite(switchPin_3, 1);
  digitalWrite(switchPin_4, 1);

  pinMode(trigger2, OUTPUT); // Sets the trigPin as an Output
  pinMode(echo2, INPUT); // Sets the echoPin as an Input
  
  pinMode(trigger, OUTPUT); // Sets the trigPin as an Output
  pinMode(echo, INPUT); // Sets the echoPin as an Input


  //Serial.println("Wasserstandsanzeige");

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

    //currentlyFill == kleinerTank gets filled

    if(pumpAutoMode){
      if(kleinerTank < 500 && grosserTank >= 500){
        currentlyFill = true;
        digitalWrite(pinDirektVerbTank, 0);
        digitalWrite(pinPumpenVent, 1);
        }
      if((kleinerTank >= 500 && !currentlyFill) || (kleinerTank < 200 && grosserTank < 200)){
        digitalWrite(pinDirektVerbTank, 1);
        digitalWrite(pinPumpenVent, 1);
        }
      if(kleinerTank >= 1000 && currentlyFill){
        currentlyFill = false;
        digitalWrite(pinDirektVerbTank, 1);
        digitalWrite(pinPumpenVent, 1);
        }
    
      if(kleinerTank >=1500 && grosserTank < 8000){
        digitalWrite(pinDirektVerbTank, 1);
        digitalWrite(pinPumpenVent, 0);
        }
      }
    
    

    
    
    
    Serial.println(insgesamt);
    
    
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
      client.println(String(digitalRead(pinDirektVerbTank)) + ";" + String(digitalRead(pinPumpenVent)));
      }

    else if(input.indexOf("fillBigTank") >= 0){
      client.println("ok");
      digitalWrite(pinDirektVerbTank, 1);
      digitalWrite(pinPumpenVent, 0);
      }

    else if(input.indexOf("fillLowTank") >= 0){
      client.println("ok");
      digitalWrite(pinDirektVerbTank, 0);
      digitalWrite(pinPumpenVent, 1);
      }
      
    else if(input.indexOf("pumpState") >= 0){
      client.println(String(pumpAutoMode));
      }

    else if(input.indexOf("newPump") >= 0){
      client.println("ok");
      if(input.indexOf("true") >= 0){
        pumpAutoMode = true;
        }
      else{
        pumpAutoMode = false;
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
    else{
      client.println("nothing found " + input);
      }


      /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    
    //client.println("Received:" + test);
    }

  

  if(BTSerial.available()){
    String input = BTSerial.readString();
    
    if(input.indexOf("howMuchW") >= 0){
      BTSerial.println(currentWaterLevel);
      
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
      
      BTSerial.println("ok");
      }

    else if(input.indexOf("getState") >= 0){
      BTSerial.println(String(digitalRead(pinDirektVerbTank)) + ";" + String(digitalRead(pinPumpenVent)));
      }

    else if(input.indexOf("fillBigTank") >= 0){
      BTSerial.println("ok");
      digitalWrite(pinDirektVerbTank, 1);
      digitalWrite(pinPumpenVent, 0);
      }

    else if(input.indexOf("fillLowTank") >= 0){
      BTSerial.println("ok");
      digitalWrite(pinDirektVerbTank, 0);
      digitalWrite(pinPumpenVent, 1);
      }
      
    else if(input.indexOf("pumpState") >= 0){
      BTSerial.println(String(pumpAutoMode));
      }

    else if(input.indexOf("newPump") >= 0){
      BTSerial.println("ok");
      if(input.indexOf("true") >= 0){
        pumpAutoMode = true;
        }
      else{
        pumpAutoMode = false;
        }
      }      

    else if(input.indexOf("getJobs") >= 0){
      String rueckgabe = "";
      for(int i  = 0; i <= letzteFreieStelle; i++){
        rueckgabe += jobs[i] + "###";
      }
      BTSerial.println(rueckgabe);
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

      BTSerial.println("ok");
      
      }
    else{
      BTSerial.println("nothing found " + input);
      }
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


  





  

}

void driveJob(String jobStr){
    Serial.println("drive Job: " + jobStr);

    if(jobStr.indexOf("Unit1;") > 0){
      digitalWrite(switchPin_1, 0);
      Serial.println("switchOnPin_1");
      }
    if(jobStr.indexOf("Unit2;") > 0){
      digitalWrite(switchPin_2, 0);
      Serial.println("switchOnPin_2");
      }
    if(jobStr.indexOf("Unit3;") > 0){
      digitalWrite(switchPin_3, 0);
      Serial.println("switchOnPin_3");
    }
    if(jobStr.indexOf("Unit4;") > 0){
      digitalWrite(switchPin_4, 0);
      Serial.println("switchOnPin_4");
      }

    if(jobStr.indexOf("Unit5;") > 0){
      digitalWrite(switchPin_5, 0);
      Serial.println("switchOnPin_5");
      }
    if(jobStr.indexOf("Unit6;") > 0){
      digitalWrite(switchPin_6, 0);
      Serial.println("switchOnPin_6");
      }
    if(jobStr.indexOf("Unit7;") > 0){
      digitalWrite(switchPin_7, 0);
      Serial.println("switchOnPin_7");
    }
    if(jobStr.indexOf("Unit8;") > 0){
      digitalWrite(switchPin_8, 0);
      Serial.println("switchOnPin_7");
      }
    
    
  }
void stopJob(String jobStr){
  Serial.println("stop Job" + jobStr);


  if(jobStr.indexOf("Unit1;") > 0){
      digitalWrite(switchPin_1, 1);
      Serial.println("switchOffPin_1");
      }
    if(jobStr.indexOf("Unit2;") > 0){
      digitalWrite(switchPin_2, 1);
      Serial.println("switchOffPin_2");
      }
    if(jobStr.indexOf("Unit3;") > 0){
      digitalWrite(switchPin_3, 1);
      Serial.println("switchOffPin_3");
    }
    if(jobStr.indexOf("Unit4;") > 0){
      digitalWrite(switchPin_4, 1);
      Serial.println("switchOffPin_4");
      }

    if(jobStr.indexOf("Unit5;") > 0){
      digitalWrite(switchPin_5, 1);
      Serial.println("switchOffPin_5");
      }
    if(jobStr.indexOf("Unit6;") > 0){
      digitalWrite(switchPin_6, 1);
      Serial.println("switchOffPin_6");
      }
    if(jobStr.indexOf("Unit7;") > 0){
      digitalWrite(switchPin_7, 1);
      Serial.println("switchOffPin_7");
    }
    if(jobStr.indexOf("Unit8;") > 0){
      digitalWrite(switchPin_8, 1);
      Serial.println("switchOffPin_7");
      }
  }





String measure() {  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////MEASURE
  lastMeasure = millis();
//  String waterStr = "";
//  Serial.println("howMuchWater?");
//
//  String line = Serial.readString();
//
//  long start = millis();
//
//  while (line == "") {
//    int tmp = millis() - start;
//    if (tmp >= timeout) {
//      line = "timeout";
//
//      //      if(currentTimeouts >= maxTimeouts){
//      //        currentTimeouts = 0;
//      //        digitalWrite(timeoutPin, HIGH);
//      //        delay(250);
//      //        digitalWrite(timeoutPin, LOW);
//      //        Serial.println("too many timeouts");
//      //        }
//      //
//      //      currentTimeouts++;
//
//
//      break;
//    }
//
////    }
//
//
//    line = Serial.readString();
//  }
//
//
//
//  //}
//  Serial.println(line + "returned");
//  line = line.substring(11,line.length()-3);


  
  int waterLevel_bigTank = getExternWaterLevel();
  int waterLevel_smallTank = getWaterLevel();
  int waterLevel = waterLevel_bigTank + waterLevel_smallTank;




  String waterLevel_str = String(waterLevel_bigTank) + ";" + String(waterLevel_smallTank) + ";" + String(waterLevel);

  Serial.println(waterLevel_str);
  //String waterLevel_str = String(waterLevel_bigTank);
  //Serial.println(waterLevel_str);
  
  
  
  return waterLevel_str;
}

int getExternWaterLevel(){
  int currentWaterLevel = 0;
  int anzTanks = 8;
  int abstand = 10;
  digitalWrite(trigger2, LOW); 
  delay(5); 
  digitalWrite(trigger2, HIGH); 
  delay(10);
  digitalWrite(trigger2, LOW);
  dauer2 = pulseIn(echo2, HIGH);
  entfernung2 = (dauer2/2) * 0.03432; 
  if (entfernung2 > 100 || entfernung2 <= 0) 
  {
  //return
  }
  else 
  {
    int level = 100 - (entfernung2 - abstand);
    currentWaterLevel = level * 10 * anzTanks;
  }

  if(currentWaterLevel > 1000 * anzTanks){
    currentWaterLevel = 1000 * anzTanks;
  }
 return currentWaterLevel;
}


int getWaterLevel(){
  int currentWaterLevel = 0;
  int anzTanks = 4;
  int abstand = 10;
  digitalWrite(trigger, LOW); 
  delay(5); 
  digitalWrite(trigger, HIGH); 
  delay(10);
  digitalWrite(trigger, LOW);
  dauer = pulseIn(echo, HIGH);
  entfernung = (dauer/2) * 0.03432; 
  if (entfernung > 100 || entfernung <= 0) 
  {
  //return
  }
  else 
  {
    int level = 100 - (entfernung - abstand);
    currentWaterLevel = level * 10 * anzTanks;
  }

  if(currentWaterLevel > 1000 * anzTanks){
    currentWaterLevel = 1000 * anzTanks;
  }
 return currentWaterLevel;
}


int getOLDWaterLevel(){////////////////////////////////////////////////////////////////////////////////////////////7/////////////////////////////////////////////////////////////////////////////////GET WATER LEVEL
  int currentWaterLevel = 0;

  for(int i = 0; i <= 7; i++){
    if(getValueForSensor(i) == 0){
        currentWaterLevel += waterBetweenSensors;
      }
    else{
      break;
      }
    }


  if(getValueForSensor(6) == 0 && getValueForSensor(7) == 0){
      currentWaterLevel = 4000;
  }

  return currentWaterLevel;


  }




int getValueForSensor(int sensorNum){/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////GET VALUE FOR SENSOR

  switch(sensorNum){
    case 0:
      return analogRead(A0);
      break;
    case 1:
      return analogRead(A1);
      break;
    case 2:
      return analogRead(A2);
      break;
    case 3:
      return analogRead(A3);
      break;
    case 4:
      return analogRead(A4);
      break;
    case 5:
      return analogRead(A5);
      break;
    case 6:
      return analogRead(A6);
      break;
    case 7:
      return analogRead(A7);
      break;
//    case 8:
//      return analogRead(A8);
//      break;
//    case 9:
//      return analogRead(A9);
//      break;
//    case 10:
//      return analogRead(A10);
//      break;
//    case 11:
//      return analogRead(A11);
//      break;
//    case 12:
//      return analogRead(A12);
//      break;
//    case 13:
//      return analogRead(A13);
//      break;
//    case 14:
//      return analogRead(A14);
//      break;
//    case 15:
//      return analogRead(A15);
//      break;

    }
    return 100;

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
