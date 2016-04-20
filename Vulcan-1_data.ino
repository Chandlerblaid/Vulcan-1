#include <SD.h>
#include <OneWire.h>

/*
 * This is the code for connecting data readings from the rocket
 * to the ground using the Arduino board.
 */

// Yellow Wire
#define CAM_1_PWR 30  // Power wire from camera 1
#define CAM_2_PWR 31  // Power wire from camera 2
#define CAM_3_PWR 32  // Power wire from camera 3
#define CAM_4_PWR 33  // Power wire from camera 4

// White Wire
#define CAM_1_REC 34  // Record wire from camera 1
#define CAM_2_REC 35  // Record wire from camera 2
#define CAM_3_REC 36  // Record wire from camera 3
#define CAM_4_REC 37  // Record wire from camera 4

// Blue Wire
#define CAM_1_LED 38  // LED for camera 1
#define CAM_2_LED 39  // LED for camera 2 
#define CAM_3_LED 40  // LED for camera 3
#define CAM_4_LED 41  // LED for camera 4

// OneWire constants
#define TYPE_DS18S20  0
#define TYPE_DS18B20  1
#define TYPE_DS18S22  2
#define TYPE_MAX31850 3

OneWire  ds(10);  // on pin 10 (a 4.7K pullup resistor is necessary)

#define SAMPLE_SIZE 10
#define NUM_OF_CAMS 4

File dataFile;

int led1, led2, led3, led4;
int i = 0;

int rec[NUM_OF_CAMS][SAMPLE_SIZE];
int ledArray[NUM_OF_CAMS];
int sum[NUM_OF_CAMS];

float kelvin;
short temperature;

unsigned char msg;
bool turnOff, stopRecording, start, finish = false;

byte cameraByte, tcByte = 0;

boolean on = false;        //Flag to indicate if cameras are on
boolean recording = false; //Flag to indicate whether cameras are recording

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

// The setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(38400);
  
  pinMode(A0, INPUT); //A0 input
  pinMode(A1, INPUT); //A1 input
  pinMode(A2, INPUT); //A2 input
  pinMode(A3, INPUT); //defines A3 as input
  
  pinMode(CAM_1_PWR, OUTPUT); //D30 output to camera 1 Power
  pinMode(CAM_2_PWR, OUTPUT); //D31 output to camera 2 Power
  pinMode(CAM_3_PWR, OUTPUT); //D32 output to camera 3 Power
  pinMode(CAM_4_PWR, OUTPUT); //D33 output to camera 4 Power

  pinMode(CAM_1_REC, OUTPUT); //D34 output to camera 1 Record button
  pinMode(CAM_2_REC, OUTPUT); //D35 output to camera 2 Record button
  pinMode(CAM_3_REC, OUTPUT); //D36 output to camera 3 Record button
  pinMode(CAM_4_REC, OUTPUT); //D37 output to camera 4 Record button

  pinMode(CAM_1_LED, INPUT); //D38  output to camera 1 LED
  pinMode(CAM_2_LED, INPUT); //D39  output to camera 2 LED
  pinMode(CAM_3_LED, INPUT); //D40 output to camera 3 LED
  pinMode(CAM_4_LED, INPUT); //D41 output to camera 4 LED

  inputString.reserve(200); //Allocate space for an input string in the serial port

  digitalWrite(CAM_1_REC, HIGH);  //Set the record ports to high as default
  digitalWrite(CAM_2_REC, HIGH); 
  digitalWrite(CAM_3_REC, HIGH); 
  digitalWrite(CAM_4_REC, HIGH); 

}

// The loop routine runs over and over again forever:
void loop() {

  if(Serial.available()) {
    readSerial(); 
  }
  
  readTemp();
//  saveToSD();
  
//  delay(50); // delay in between reads for stability

  if(start) {
    if(!on) {
      Serial.println("\tToggling Power Button");
      togglePwrButton();
    }

    if(!recording) {
      Serial.println("\tToggling Record Button\n");
      toggleRecButton();
    } else {
      Serial.println("\tToggling Record Button\n");
      toggleRecButton();
      toggleRecButton();
    }

    start = false;
  }

  if(finish) {
    if(recording) {
      Serial.println("\tToggling Record Button");
      toggleRecButton();
    }

    if(on) {
      Serial.println("\tToggling Power Button\n");
      togglePwrButton();
    }

    finish = false;
  }

  led1 = digitalRead(CAM_1_LED);  //Track LEDs
  led2 = digitalRead(CAM_2_LED);
  led3 = digitalRead(CAM_3_LED);
  led4 = digitalRead(CAM_4_LED);

  ledArray[0] = led1;
  ledArray[1] = led2;
  ledArray[2] = led3;
  ledArray[3] = led4;

  isRecording(i, ledArray);
  
  i++;
}

void togglePwrButton() {
    digitalWrite(CAM_1_PWR, HIGH);  //Hold down the power buttons
    digitalWrite(CAM_2_PWR, HIGH);
    digitalWrite(CAM_3_PWR, HIGH);
    digitalWrite(CAM_4_PWR, HIGH);
    delay(3500);
    digitalWrite(CAM_1_PWR, LOW);   //Release the power buttons
    digitalWrite(CAM_2_PWR, LOW);
    digitalWrite(CAM_3_PWR, LOW);
    digitalWrite(CAM_4_PWR, LOW);
    delay(3000);
}

void toggleRecButton() {
    digitalWrite(CAM_1_REC, LOW);  //Start Recording
    digitalWrite(CAM_2_REC, LOW);
    digitalWrite(CAM_3_REC, LOW);
    digitalWrite(CAM_4_REC, LOW); 
    delay(100);
    digitalWrite(CAM_1_REC, HIGH); 
    digitalWrite(CAM_2_REC, HIGH); 
    digitalWrite(CAM_3_REC, HIGH); 
    digitalWrite(CAM_4_REC, HIGH); 
    delay(1000);
}

// Monitor whether or not the cameras are recording
void isRecording(int cnt, int cam[]) { 
 
  // Check each camera individually
  for(int t = 0; t <= 4; t++) { 
    // If there has been 10 loops
    if (cnt%SAMPLE_SIZE == 0) { 
      
      for(int j = 0; j < SAMPLE_SIZE; j++) {   
        sum[t] += rec[t][j]; 
      }
    
      if(!sum[t]) {
        recording = false;
        on = false;
        
        Serial.print("\n#");
        Serial.print(t);
        Serial.print(": OFF, NOT RECORDING\n");
  
        cameraByte = cameraByte & (1 << (7 - t)); // Toggle off corresponding indicator bit
      } 
      else if (sum[t] == SAMPLE_SIZE) {
        recording = false;
        on = true;
        
        Serial.print("\n#");
        Serial.print(t);
        Serial.println(": ON, NOT RECORDING\n");
  
        cameraByte = cameraByte & (1 << (7 - t)); // Toggle off corresponding indicator bit
      }
      else if (sum[t] > 0 && sum[t] < SAMPLE_SIZE) {
        if(i > 0) {
          recording = true;
          on = true;
          Serial.print("\n#");
          Serial.print(t);
          Serial.println(": RECORDING\n");
          
          cameraByte = cameraByte | (1 << (7 - t)); // Toggle or leave corresponding indicator bit
        }
      } 
      
      for(int j = 0; j < SAMPLE_SIZE; j++) { rec[t][j] = 0; } 
      sum[t] = 0;
    }
    
    rec[t][cnt%SAMPLE_SIZE] = cam[t];
  }
}

// Read in a serial event
void readSerial() {
  /* 
   *  Commands that can be sent to the cameras from the serial monitor
   *  : 0x51 = Turn on all cameras, and respond with CameraByte
   *  : 0x52 = Turn on all cameras, and respond with TC Byte
   *  : 0x53 = Turn off all cameras, and respond with CameraByte
   *  : 0x54 = Turn off all cameras, and respond with TC Byte   
   *  
   */
  msg = Serial.read();
  switch (msg) {
    case 0x51:
      Serial.println("\nStarting up...\n");
      start = true;
      respond(true);
      break;

    case 0x52:
      Serial.println("\nStarting up...\n");
      start = true;
      respond(false);
      break;

    case 0x53:
      Serial.println("\nShutting down...\n");
      finish = true;
      respond(true);
      break;

    case 0x54:
      Serial.println("\nShutting down...\n");
      finish = true;
      respond(false);
      break;
  }
}

void respond(bool camByte) {
  configureBytes();
  
  if(camByte) {
    Serial.write(cameraByte);
  } else {
    Serial.write(tcByte);
  }
}


/*
 * Get temperature readings from the Thermocouples.
 */
void readTemp() {
  byte i;
  byte present = 0;
  byte temptype;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  
  if (!ds.search(addr)) {
    ds.reset_search();
    delay(250);
    return;
  }
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      return;
  }
  switch (addr[0]) {
    case 0x10:
      temptype = TYPE_DS18S20;
      break;
    case 0x28:
      temptype = TYPE_DS18B20;
      break;
    case 0x22:
      temptype = TYPE_DS18S22;
      break;
    case 0x3B:
      temptype = TYPE_MAX31850;
      break;
    default:
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
//  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  int ct = 0;
  while(ct < 750)
  {
    delay(1);
    if(Serial.available()) {
      readSerial();
    }
    ct++;
  }
//  ct=0;
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }
//  Serial.print(" CRC=");
//  Serial.print(OneWire::crc8(data, 8), HEX);
//  Serial.println();

 // Serial.print("  Address = 0x"); Serial.println(data[4] & 0xF, HEX);

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (temptype == TYPE_DS18S20) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else if (temptype == TYPE_MAX31850) {
    //Serial.println(raw, HEX);
    if (raw & 0x01) {
      Serial.println("**FAULT!**");
      return;
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  kelvin = celsius + 273.15;
//  fahrenheit = celsius * 1.8 + 32.0;
//  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.print(kelvin);
  Serial.print(" Kelvin");
//  Serial.print(fahrenheit);
//  Serial.println(" Fahrenheit");

  temperature = kelvin * 10;
}

void configureBytes() {
  short tmp = 0;
  tmp = tmp | (cameraByte << 8); //if temp & (1 << 11) -> cuttoff

  tmp = tmp | temperature;

  cameraByte = (byte) (temperature >> 8);
  tcByte = (byte) temperature;
}

//void saveToSD() {
//  //Wrting to SD card
//  Serial.print("Initializing SD card.."); 
//  //pin 10 on most Arduino boards must be left as an output or 
//  //the SD library functions will not work
//  pinMode(10,OUTPUT);
//  if(!SD.begin(4)) {
//    Serial.println("Initializing failed!");
//    return;
//  }
//  Serial.println("Initializing done.");
//  dataFile = SD.open("data.txt", FILE_WRITE);
//  if (dataFile) {
//    dataFile.print("Writing to data.txt...");
//    dataFile.print("PT 1: ");
//    dataFile.print(sensorPT1);
//    dataFile.print("\t");
//    dataFile.print("PT 2: ");
//    dataFile.print(sensorPT2);
//    dataFile.print("\t");
//    dataFile.print("PT 3: ");
//    dataFile.print(sensorPT3);
//    dataFile.print("\t");
//    dataFile.print("PT 4: ");
//    dataFile.print(sensorPT4);
//    dataFile.print ("\t");
//    dataFile.print("PT 5: ");
//    dataFile.print(sensorPT5);
//    dataFile.print ("\t");
//    dataFile.print("PT 6: ");
//    dataFile.print(sensorPT6);
//    dataFile.print ("\t");
//    dataFile.print("PT 7: ");
//    dataFile.print(sensorPT7);
//    dataFile.print ("\t");
//    dataFile.print("PT 8: ");
//    dataFile.println(sensorPT8);
//    dataFile.close();
//    Serial.println("Reading done.");
//  } else {
//    Serial.println("error opening data.txt");
//  }
//  delay(1000); // delay in between reads for stability
//}

