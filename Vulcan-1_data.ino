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

#define SAMPLE_SIZE 10

int led1, led2, led3, led4;
int i = 0;

int rec[10];
int sum = 0;
char msg;
bool turnOff = false;
bool stopRecording = false;

boolean on = false;        //Flag to indicate if cameras are on
boolean recording = false; //Flag to indicate whether cameras are recording

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

// The setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  
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

  /* commands sent to the cameras from the serial monitor
   *  : 'f' = Turn off all cameras
   *  : 's' = Stop recording on all cameras
   */
  msg = Serial.read();
  if (msg = 'f') {
    Serial.println("\nTurning Cameras off!\n");
    
    
  }

  // read the input on analog pin 0:
  int sensorPT1 = analogRead(A0); //PT1
  int sensorPT2 = analogRead(A1); //PT2
  int sensorPT3 = analogRead(A2); //PT3
//  int sensorPT4 = analogRead(A3); //PT4
//  int sensorPT5 = analogRead(A4); //PT1
//  int sensorPT6 = analogRead(A5); //PT2
//  int sensorPT7 = analogRead(A6); //PT3
//  int sensorPT8 = analogRead(A7); //PT4
  
  // print out the value you read:

  Serial.print("PT 1: ");
  Serial.print(sensorPT1);
  Serial.print("\t");
  Serial.print("PT 2: ");
  Serial.print(sensorPT2);
  Serial.print("\t");
  Serial.print("PT 3: ");
  Serial.print(sensorPT3);
  Serial.println("\t");
//  Serial.print("PT 4: ");
//  Serial.print(sensorPT4);
//  Serial.print ("\t");
//  Serial.print("PT 5: ");
//  Serial.print(sensorPT5);
//  Serial.print ("\t");
//  Serial.print("PT 6: ");
//  Serial.print(sensorPT6);
//  Serial.print ("\t");
//  Serial.print("PT 7: ");
//  Serial.print(sensorPT7);
//  Serial.print ("\t");
//  Serial.print("PT 8: ");
//  Serial.println(sensorPT8);
  delay(500); // delay in between reads for stability

  //If the cameras are not on
  if(!on) { 
    // Turn on all of the cameras
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
    
    on = true;
  }
  // If the cameras are not yet recording
  else if(!recording) {
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
    
    recording = true;
  }

//  // print the string when a newline arrives:
//  if (stringComplete) {
//    Serial.println(inputString);
//    // clear the string:
//    inputString = "";
//    stringComplete = false;
//  }

  led1 = digitalRead(CAM_1_LED);  //Track LEDs
  led2 = digitalRead(CAM_2_LED);
  led3 = digitalRead(CAM_3_LED);
  led4 = digitalRead(CAM_4_LED);

  // LED input testing
//  if(led1) {
//    Serial.println("ON\t\t");
//    Serial.println(led1, DEC);
//  } else {
//    Serial.println("OFF\t\t");
//    Serial.println(led1);
//  }

  if (i%SAMPLE_SIZE == 0) { 
    for(int j = 0; j < SAMPLE_SIZE; j++) { 
      sum += rec[j]; 
    }
    
    if(sum == 0) {
      recording = false;
      Serial.println("\nOFF, NOT RECORDING\n");
    } 
    else if (sum == SAMPLE_SIZE) {
      recording = false;
      Serial.println("\nON, NOT RECORDING\n");
    }
    else if (sum > 0 && sum < 10) {
      if(i > 0) {
        recording = true;
        Serial.println("\nRECORDING\n");
      }
    }
    
    for(int j = 0; j < SAMPLE_SIZE; j++) { 
      rec[j] = 0; 
      sum = 0;
    } 
  }
  
  rec[i%SAMPLE_SIZE] = led1;
  
  i++;
}

//// GOing to be used as event trigger
//void serialEvent() {
//  while(Serial.available()) {
//    char inChar = (char) Serial.read();
//    inputString += inChar;
//
//    if(inChar == '\n') {
//      stringComplete = true;
//    }
//  }
//}

