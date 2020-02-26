int waterBetweenSensors = 500;
#define switchPin_5 30
#define switchPin_6 31
#define switchPin_7 32
#define switchPin_8 33
int timeLastMeasure = millis();

void setup() { /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////SETUP
  Serial.begin(9600);

  pinMode(switchPin_5, OUTPUT);
  pinMode(switchPin_6, OUTPUT);
  pinMode(switchPin_7, OUTPUT);
  pinMode(switchPin_8, OUTPUT);

}

void loop() {/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////LOOOOOOOOOOOOOOOOOOOOOOOOOP

  String readed = Serial.readString();


  while(readed.equals("")){readed = Serial.readString();}

  
  //Serial.println(readed);
  if(readed.indexOf("howMuchWater?") > 0){
    //Serial.println("bindrinn");
    timeLastMeasure = millis();
    //Serial.println("vor");
    int waterLevel = getWaterLevel();
    //Serial.println("nach");
    Serial.println("waterLevel:" + String(waterLevel) + ":");
    }
  else if(readed.indexOf("switchOnPin_5") > 0){
    digitalWrite(switchPin_5, HIGH);
    }
  else if(readed.indexOf("switchOffPin_5") > 0){
    digitalWrite(switchPin_5, LOW);
    }
  else if(readed.indexOf("switchOnPin_6") > 0){
    digitalWrite(switchPin_6, HIGH);
    }
  else if(readed.indexOf("switchOffPin_6") > 0){
    digitalWrite(switchPin_6, LOW);
    }
  else if(readed.indexOf("switchOnPin_7") > 0){
    digitalWrite(switchPin_7, HIGH);
    }
  else if(readed.indexOf("switchOffPin_7") > 0){
    digitalWrite(switchPin_7, LOW);
    }
  else if(readed.indexOf("switchOnPin_8") > 0){
    digitalWrite(switchPin_8, HIGH);
    }
  else if(readed.indexOf("switchOffPin_8") > 0){
    digitalWrite(switchPin_8, LOW);
    }



  if(millis() - timeLastMeasure > 300000){
    //////////////////////////////////////////////////////IMPLEMENT reset master arduino
    timeLastMeasure = millis();
    }
  


  
  
  

}

int getWaterLevel(){////////////////////////////////////////////////////////////////////////////////////////////7/////////////////////////////////////////////////////////////////////////////////GET WATER LEVEL
  Serial.println("getWater");
  int currentWaterLevel = 0;
  
  for(int i = 0; i <= 15; i++){
    //Serial.println(i);
    //Serial.println("Reading Sensor #" + String(i) + ":" + getValueForSensor(i));
    if(getValueForSensor(i) == 0){
      
        currentWaterLevel += waterBetweenSensors;

        //Serial.println(currentWaterLevel);
      }
     
    else{
      break;
      }
    }


  if(getValueForSensor(14) == 0 && getValueForSensor(15) == 0){
      currentWaterLevel = 8000;
  }
  //Serial.println("return");
  return currentWaterLevel;

  
  }

int getValueForSensor(int sensorNum){ /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////GET VALUE FOR SENSOR
  
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
    case 8:
      return analogRead(A8);
      break;
    case 9:
      return analogRead(A9);
      break;
    case 10:
      return analogRead(A10);
      break;
    case 11:
      return analogRead(A11);
      break;
    case 12:
      return analogRead(A12);
      break;
    case 13:
      return analogRead(A13);
      break;
    case 14:
      return analogRead(A14);
      break;
    case 15:
      return analogRead(A15);
      break;
    }


    Serial.println("ERRRRRRROR");
  
  }
