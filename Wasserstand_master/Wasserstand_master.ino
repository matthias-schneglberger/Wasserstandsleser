#define timeout 5000
#define timeoutPin 2
#define waterBetweenSensors 500
#define switchPin_1 30
#define switchPin_2 31
#define switchPin_3 32
#define switchPin_4 33

#define btRX 4
#define btTX 3


int maxTimeouts = 3;
int currentTimeouts = 0;
int delayBetween = 5000;
int lastMeasure = 0;

void setup() {/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////SETUP


  Serial.begin(9600);

  pinMode(timeoutPin, OUTPUT);

  pinMode(switchPin_1, OUTPUT);
  pinMode(switchPin_2, OUTPUT);
  pinMode(switchPin_3, OUTPUT);
  pinMode(switchPin_4, OUTPUT);

  //Serial.println("Wasserstandsanzeige");

}

void loop() { /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////LOOOOOOOOOOP
  int diff = millis() - lastMeasure;
  if (diff >= delayBetween) {
    Serial.println("measure!");
    lastMeasure = millis();
    Serial.println("ANFANG" + measure() + "ENDE");
  }

  //if(BTSerial.readString() == "measure"){
  //BTSerial.println(measure());
  //}

}







String measure() {  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////MEASURE
  lastMeasure = millis();
  String waterStr = "";
  Serial.println("howMuchWater?");

  //Serial.println("whaiting for line...");
  String line = Serial.readString();

  long start = millis();

  while (line == "") {
    //Serial.println("in while");
    int tmp = millis() - start;
    if (tmp >= timeout) {
      //Serial.println("timeout");
      line = "timeout";

      //      if(currentTimeouts >= maxTimeouts){
      //        currentTimeouts = 0;
      //        digitalWrite(timeoutPin, HIGH);
      //        delay(250);
      //        digitalWrite(timeoutPin, LOW);
      //        Serial.println("too many timeouts");
      //        }
      //
      //      currentTimeouts++;


      break;
    }

//    if (line.indexOf("waterLevel:") > 0) {
//      Serial.println("found smth");
//      waterStr = line;
//      break;
//    }


    //Serial.println("readNewString...");
    line = Serial.readStringUntil('\n');

    if(line.length() > 10){line = "";}
  }

  //Serial.println("out of while");

  //Serial.println("Antwort: " + waterStr + "antwort ENDE");
  //if(isDigit(line){

  //}


  if(line.indexOf("waterLevel:") > 0){
    line = getValue(line, ':', 1);
    }


  //int waterLevel_bigTank = ;
  //int waterLevel_smallTank = getWaterLevel();
  //int waterLevel = waterLevel_bigTank + waterLevel_smallTank;




  //String waterLevel_str = String(waterLevel_bigTank) + ";" + String(waterLevel_smallTank) + String(waterLevel);
  //String waterLevel_str = String(waterLevel_bigTank);
  //Serial.println(waterLevel_str);

  
  
  return line;
}



//int getWaterLevel(){////////////////////////////////////////////////////////////////////////////////////////////7/////////////////////////////////////////////////////////////////////////////////GET WATER LEVEL
//  int currentWaterLevel = 0;
//
//  for(int i = 0; i <= 15; i++){
//    if(getValueForSensor(i) == 0){
//        currentWaterLevel += waterBetweenSensors;
//      }
//    else{
//      break;
//      }
//    }
//
//
//  if(getValueForSensor(14) == 0 && getValueForSensor(15) == 0){
//      currentWaterLevel = 8000;
//  }
//
//  return currentWaterLevel;
//
//
//  }




//int getValueForSensor(int sensorNum){/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////GET VALUE FOR SENSOR
//
//  switch(sensorNum){
//    case 0:
//      return analogRead(A0);
//      break;
//    case 1:
//      return analogRead(A1);
//      break;
//    case 2:
//      return analogRead(A2);
//      break;
//    case 3:
//      return analogRead(A3);
//      break;
//    case 4:
//      return analogRead(A4);
//      break;
//    case 5:
//      return analogRead(A5);
//      break;
//    case 6:
//      return analogRead(A6);
//      break;
//    case 7:
//      return analogRead(A7);
//      break;
////    case 8:
////      return analogRead(A8);
////      break;
////    case 9:
////      return analogRead(A9);
////      break;
////    case 10:
////      return analogRead(A10);
////      break;
////    case 11:
////      return analogRead(A11);
////      break;
////    case 12:
////      return analogRead(A12);
////      break;
////    case 13:
////      return analogRead(A13);
////      break;
////    case 14:
////      return analogRead(A14);
////      break;
////    case 15:
////      return analogRead(A15);
////      break;
//
//    }
//    return 100;
//
//  }





































































String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}
