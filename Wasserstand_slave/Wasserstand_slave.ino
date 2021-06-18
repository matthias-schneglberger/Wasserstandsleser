#include <Wire.h>

#define ArduinoNum 0   // 0-> intern; 1 -> extern

unsigned char ok_flag;
unsigned char fail_flag;

unsigned short lenth_val = 0;
unsigned char i2c_rx_buf[16];
unsigned char dirsend_flag = 0;

void setup() { /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////SETUP
  Serial.begin(9600);
  Wire.begin();

  Serial.println("Testing - System:");
  ScanDevices();
  Serial.println("Done with Scanning for I2C devices -> Idle Mode");
}

void loop() {/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////LOOOOOOOOOOP

  String readed = Serial.readString();
  while(readed.equals("")){readed = Serial.readString();}
  
  Serial.println("New Message received:" + readed);
  
  if(readed.startsWith(String(ArduinoNum) + ":measure")){
    Serial.println("distance:" + String(ReadDistance()));
  }
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
int ReadDistance() {
  SensorRead(0x00, i2c_rx_buf, 2);
  lenth_val = i2c_rx_buf[0];
  lenth_val = lenth_val << 8;
  lenth_val |= i2c_rx_buf[1];
  return lenth_val/10;
}
void ScanDevices(){
  byte error, address; //variable for error and I2C address
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");
      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}
