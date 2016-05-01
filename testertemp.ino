#include <OneWire.h>

OneWire  ds(10);  // on pin 10 (a 4.7K pullup resistor is necessary)

// Yellow Wire
#define CAM_1_PWR 42  // Power wire from camera 1
#define CAM_2_PWR 48  // Power wire from camera 2
#define CAM_3_PWR 49  // Power wire from camera 3
#define CAM_4_PWR 43  // Power wire from camera 4

// White Wire
#define CAM_1_REC 44  // Record wire from camera 1
#define CAM_2_REC 50  // Record wire from camera 2
#define CAM_3_REC 51  // Record wire from camera 3
#define CAM_4_REC 45  // Record wire from camera 4

// Blue Wire
#define CAM_1_LED 46  // LED for camera 1
#define CAM_2_LED 52  // LED for camera 2 
#define CAM_3_LED 53  // LED for camera 3
#define CAM_4_LED 47  // LED for camera 4

// OneWire constants
#define TYPE_DS18S20  0
#define TYPE_DS18B20  1
#define TYPE_DS18S22  2
#define TYPE_MAX31850 3

#define SAMPLE_SIZE 10
#define NUM_OF_CAMS 4

int led1, led2, led3, led4;
int i, cnt = 0;

int rec[NUM_OF_CAMS][SAMPLE_SIZE];
int ledArray[NUM_OF_CAMS];
int sum[NUM_OF_CAMS];

float kelvin;
unsigned int temperature;

unsigned char msg;
bool start, finish, should = false;

unsigned char cameraByte, tcByte = 0;

boolean on[NUM_OF_CAMS] = {false, false, false, false};        //Flag to indicate if cameras are on
boolean recording[NUM_OF_CAMS] = {false, false, false, false}; //Flag to indicate whether cameras are recording

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

  digitalWrite(CAM_1_REC, HIGH);  //Set the record ports to high as default
  digitalWrite(CAM_2_REC, HIGH); 
  digitalWrite(CAM_3_REC, HIGH); 
  digitalWrite(CAM_4_REC, HIGH); 
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
      respond(true);
      beginRecording();
      should = true;
      break;

    case 0x52:
      Serial.println("\nStarting up...\n");
      respond(false);
      beginRecording();
      should = true;
      break;

    case 0x53:
//      Serial.println("\nShutting down...\n");
      respond(true);
      endRecording();
      should = false;
      break;

    case 0x54:
//      Serial.println("\nShutting down...\n");
      respond(false);
      endRecording();
      should = false;
      break;

//    default:
//      return;
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

void configureBytes() {
  unsigned char tmp = 0;
  tmp = (unsigned char) temperature & 0x0f;
  
  cameraByte = (unsigned char) cameraByte | tmp;
//  cameraByte = (unsigned char) 0xf0 | tmp;  
  tcByte = temperature >> 4;
}


/*
 * Get temperature readings from the Thermocouples.
 */
void readTemp() {
//  Serial.println("Reading Temp...");
  byte i;
  byte present = 0;
  byte temptype;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  
  if (!ds.search(addr)) {
    ds.reset_search();

    // 250 ms active-delay
    int ct2 = 0;
    while(ct2 < 250)
    {
      delay(1);
      if(Serial.available()) {
        readSerial();
      }
      ct2++;
    }
    return;
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

  // 750ms active-delay
  int ct1 = 0;
  while(ct1 < 750)
  {
    delay(1);
    if(Serial.available()) {
      readSerial();
    }
    ct1++;
  }
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

   
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
  //    Serial.println("**FAULT!**");
      return;
    }
  } else {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
  }
  celsius = (float)raw / 16.0;
  kelvin = celsius + 273.15;
  temperature = kelvin * 10;
}

void togglePwrON() {
  int sumv = 0;
  for(int v = 0; v < NUM_OF_CAMS; v++) {
    Serial.println("\t\tin for");
    if (on[v]) {
      sumv++;
      Serial.println("\t\t\tin if");
    }
  }
  Serial.print("\n\nSumv = ");
  Serial.print(sumv);
  Serial.println();

  if (sumv == 4) {
    Serial.println("returning");
    return;
  }
  Serial.println("\t\tTurning on power");  
  if (!on[0]) {
    digitalWrite(CAM_1_PWR, HIGH);  //Hold down the power buttons
  }
  if (!on[1]) {
    digitalWrite(CAM_2_PWR, HIGH);      
  }
  if (!on[2]) {
    digitalWrite(CAM_3_PWR, HIGH);      
  }
  if (!on[3]) {
    digitalWrite(CAM_4_PWR, HIGH);      
  }
  
  // 3.5s active-delay
  int ct1 = 0;
  while(ct1 < 3500)
  {
    delay(1);
    if(Serial.available()) {
      readSerial();
      ct1++;
    }
    ct1++;
  }
  
  if (!on[0]) {
    digitalWrite(CAM_1_PWR, LOW);  //Release the power buttons
    on[0] = true;
  }
  if (!on[1]) {
    digitalWrite(CAM_2_PWR, LOW); 
    on[1] = true;     
  }
  if (!on[2]) {
    digitalWrite(CAM_3_PWR, LOW); 
    on[2] = true;     
  }
  if (!on[3]) {
    digitalWrite(CAM_4_PWR, LOW);   
    on[3] = true;   
  }

  // 3s active-delay
  int ct2 = 0;
  while(ct2 < 3000)
  {
    delay(1);
    if(Serial.available()) {
      readSerial();
      ct2++;
    }
    ct2++;
  }
}

void togglePwrOFF() {
  int sumv = 0;
  for(int v = 0; v < NUM_OF_CAMS; v++) {
    sumv += on[v];
  }

  if (!sumv) {
    return;
  }
  Serial.println("\t\tTurning off power");
  if (on[0]) {
    digitalWrite(CAM_1_PWR, HIGH);  //Hold down the power buttons
  }
  if (on[1]) {
    digitalWrite(CAM_2_PWR, HIGH);      
  }
  if (on[2]) {
    digitalWrite(CAM_3_PWR, HIGH);      
  }
  if (on[3]) {
    digitalWrite(CAM_4_PWR, HIGH);      
  }
  
  // 3.5s active-delay
  int ct1 = 0;
  while(ct1 < 3500)
  {
    delay(1);
    if(Serial.available()) {
      readSerial();
      ct1++;
    }
    ct1++;
  }
  
  if (on[0]) {
    digitalWrite(CAM_1_PWR, LOW);  //Release the power buttons
    on[0] = false;
  }
  if (on[1]) {
    digitalWrite(CAM_2_PWR, LOW); 
    on[1] = false;     
  }
  if (on[2]) {
    digitalWrite(CAM_3_PWR, LOW);
    on[2] = false;      
  }
  if (on[3]) {
    digitalWrite(CAM_4_PWR, LOW); 
    on[3] = false;     
  }

  // 3s active-delay
  int ct2 = 0;
  while(ct2 < 3000)
  {
    delay(1);
    if(Serial.available()) {
      readSerial();
      ct2++;
    }
    ct2++;
  }
}

void startRec() {
  int sumv = 0;
  for(int v = 0; v < NUM_OF_CAMS; v++) {
    sumv += recording[v];
  }

  if (sumv == 4) {
    return;
  }
  Serial.println("\t\tStarting record");
  if (!recording[0]) {
    digitalWrite(CAM_1_REC, LOW);  //Start Recording
    
    int ct1 = 0;
    while(ct1 < 100)
    {
      delay(1);
      if(Serial.available()) {
        readSerial();
        ct1++;
      }
      ct1++;
    }
    
    digitalWrite(CAM_1_REC, HIGH);  
    recording[0] = true;
  }
  if (!recording[1]) {
    digitalWrite(CAM_2_REC, LOW);
    
    int ct1 = 0;
    while(ct1 < 100)
    {
      delay(1);
      if(Serial.available()) {
        readSerial();
        ct1++;
      }
      ct1++;
    }
    
    digitalWrite(CAM_2_REC, HIGH);  
    recording[1] = true;
  }
  if (!recording[2]) {
    digitalWrite(CAM_3_REC, LOW);
    
    int ct1 = 0;
    while(ct1 < 100)
    {
      delay(1);
      if(Serial.available()) {
        readSerial();
        ct1++;
      }
      ct1++;
    }
    
    digitalWrite(CAM_3_REC, HIGH); 
    recording[2] = true;
  }
  if (!recording[3]) {
    digitalWrite(CAM_4_REC, LOW); 
    
    int ct1 = 0;
    while(ct1 < 100)
    {
      delay(1);
      if(Serial.available()) {
        readSerial();
        ct1++;
      }
      ct1++;
    }
    
    digitalWrite(CAM_4_REC, HIGH); 
    recording[3] = true;
  }
  
  int ct1 = 0;
  while(ct1 < 1000)
  {
    delay(1);
    if(Serial.available()) {
      readSerial();
      ct1++;
    }
    ct1++;
  }
}

void stopRec() {
  int sumv = 0;
  for(int v = 0; v < NUM_OF_CAMS; v++) {
    sumv += recording[v];
  }

  if (!sumv) {
    return;
  }
  Serial.println("\t\tStopping record");
  if (recording[0]) {
    digitalWrite(CAM_1_REC, LOW);  //Start Recording
    
    int ct1 = 0;
    while(ct1 < 100)
    {
      delay(1);
      if(Serial.available()) {
        readSerial();
        ct1++;
      }
      ct1++;
    }
    
    digitalWrite(CAM_1_REC, HIGH);  
    recording[0] = false;
  }
  if (recording[1]) {
    digitalWrite(CAM_2_REC, LOW);
    
    int ct1 = 0;
    while(ct1 < 100)
    {
      delay(1);
      if(Serial.available()) {
        readSerial();
        ct1++;
      }
      ct1++;
    }
    
    digitalWrite(CAM_2_REC, HIGH);  
    recording[1] = false;
  }
  if (recording[2]) {
    digitalWrite(CAM_3_REC, LOW);
    
    int ct1 = 0;
    while(ct1 < 100)
    {
      delay(1);
      if(Serial.available()) {
        readSerial();
        ct1++;
      }
      ct1++;
    }
    
    digitalWrite(CAM_3_REC, HIGH); 
    recording[2] = false;
  }
  if (recording[3]) {
    digitalWrite(CAM_4_REC, LOW); 
    
    int ct1 = 0;
    while(ct1 < 100)
    {
      delay(1);
      if(Serial.available()) {
        readSerial();
        ct1++;
      }
      ct1++;
    }
    
    digitalWrite(CAM_4_REC, HIGH); 
    recording[3] = false;
  }
  
  int ct1 = 0;
  while(ct1 < 1000)
  {
    delay(1);
    if(Serial.available()) {
      readSerial();
      ct1++;
    }
    ct1++;
  }
}

// Monitor whether or not the cameras are recording
void isRecording(int cam[]) { 
  // If there has been 10 loops
  if (cnt%SAMPLE_SIZE) {
    Serial.println("ifffffffffffffffffffff");
    for (int y = 0; y < NUM_OF_CAMS; y++) {
      rec[y][cnt%SAMPLE_SIZE] = cam[y];
    }
    cnt++;
    return;
  }
  
  // Check each camera individually
  for(int t = 0; t < 4; t++) {      
    for(int j = 0; j < SAMPLE_SIZE; j++) {   
      sum[t] += rec[t][j]; 
      Serial.println("rec: ");
    Serial.println(rec[t][j]);
    }
//    Serial.println("sum: ");
//    Serial.println(sum[t]);

    if(sum[t] == 0) {
      recording[t] = false;
      on[t] = false;
        Serial.print("\n#");
        Serial.print(t);
        Serial.print(": OFF, NOT RECORDING\n");
      switch (t) {
        case 0:
          cameraByte = cameraByte & (0x70); // Toggle off corresponding indicator bit
        case 1:
          cameraByte = cameraByte & (0xB0); // Toggle off corresponding indicator bit
        case 2:
          cameraByte = cameraByte & (0xD0); // Toggle off corresponding indicator bit
        case 3:
          cameraByte = cameraByte & (0xE0); // Toggle off corresponding indicator bit
      }
    } 
    else if (sum[t] == SAMPLE_SIZE) {
      recording[t] = false;
      on[t] = true;
          Serial.print("\n#");
          Serial.print(t);
          Serial.println(": ON, NOT RECORDING\n");
      switch (t) {
        case 0:
          cameraByte = cameraByte & (0x70); // Toggle off corresponding indicator bit
        case 1:
          cameraByte = cameraByte & (0xB0); // Toggle off corresponding indicator bit
        case 2:
          cameraByte = cameraByte & (0xD0); // Toggle off corresponding indicator bit
        case 3:
          cameraByte = cameraByte & (0xE0); // Toggle off corresponding indicator bit
      }      
    }
    else if (sum[t] > 0 && sum[t] < SAMPLE_SIZE) {
      if(i > 0) {
        recording[t] = true;
        on[t] = true;
          Serial.print("\n#");
          Serial.print(t);
          Serial.println(": RECORDING\n");          
        cameraByte = cameraByte | (1 << (7 - t)); // Toggle or leave corresponding indicator bit
      }
    } 
    
    for(int j = 0; j < SAMPLE_SIZE; j++) { rec[t][j] = 0; } 
    sum[t] = 0;

    rec[t][cnt%SAMPLE_SIZE] = cam[t];
  }
  
//  verify();
  if (cnt <= 100) {
    cnt++;
    Serial.println(cnt);
  } else {
    cnt = 0;
  }
}

void verify() {
//  if (!(cnt%SAMPLE_SIZE)) {
//    return;
//  }
  
//  int onSum, recSum = 0;
//  for(int t = 0; t < NUM_OF_CAMS; t++) {
//    if (on[t]) { onSum++; }
//    
//    if (rec[t]) { recSum++; }
//  }

  if (should) {
    Serial.println("\tShould be on");
    togglePwrON();
    startRec();
  } else {
    Serial.println("\tshouldn't be on");
    stopRec();
    togglePwrOFF();
  }
//  if(onSum){
//    if(!(onSum == 4)) {
//      if (should) {
//        togglePwrON();
//      } else {
//        togglePwrOFF();
//      }
//    }
//  } else {
//    if (should) {
//      togglePwrON();
//    }
//  }
//  if(recSum) {
//    if(!(recSum == 4)) {
//      if (should) {
//        startRec();
//      } else {
//        stopRec();
//      }
//    }
//  } else {
//    if(should) {
//      startRec();
//    }
//  }
}

void beginRecording() {
  togglePwrON();
  startRec();

  for(int b = 0; b < NUM_OF_CAMS; b++) {
    on[b] = true;
    recording[b] = true;
  }
}

void endRecording() {
  stopRec();
  togglePwrOFF();

  for(int b = 0; b < NUM_OF_CAMS; b++) {
    on[b] = false;
    recording[b] = false;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
//  Serial.println("About to read temp");
//  Serial.println("Reading Temp");
  readTemp();
  
//  if(start) {
//    togglePwrON();
//    startRec();
//
//    start = false;
//  }
//
//  if(finish) {
//    stopRec();
//    togglePwrOFF();
//    
//    finish = false;
//  }
//  Serial.println("Verifying");
//  verify();   // Think about running this every 5 loops or something.

//  Serial.println("Reading Serial");
  if(Serial.available()) {
    readSerial();
  }

  led1 = digitalRead(CAM_1_LED);  //Track LEDs
  led2 = digitalRead(CAM_2_LED);
  led3 = digitalRead(CAM_3_LED);
  led4 = digitalRead(CAM_4_LED);

  ledArray[0] = led1;
  ledArray[1] = led2;
  ledArray[2] = led3;
  ledArray[3] = led4;
//  Serial.println("Is it recording?");
  isRecording(ledArray);

  if(i == 100) { i = 0; } // Reset i to prevent it from spilling over max int value
  
  i++; 
//  Serial.println(i);
}

