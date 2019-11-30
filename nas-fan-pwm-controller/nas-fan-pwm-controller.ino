#include "SevSeg.h"
SevSeg sevseg; //Instantiate a seven segment controller object
int pwmPin  = 3; // digital PWM pin 9
unsigned long time;
unsigned int rpm;
String stringRPM;
int thermistorPin = 0;
int tachPin = 12;
int vo;
float logR2, r2, t, r1 = 10000, c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
int diskTemperature;
unsigned int minTemp = 25, maxTemp = 45, minSpeed = 0, maxSpeed = 80;
int speed;


void setup() {
  //LCD init
  byte numDigits = 2;
  byte digitPins[] = {11, 10};
  byte segmentPins[] = {5, 6, 2, 9, 8, 7, 4};
  bool resistorsOnSegments = false; // 'false' means resistors are on digit pins
  byte hardwareConfig = COMMON_ANODE; // See README.md for options
  bool updateWithDelays = false; // Default 'false' is Recommended
  bool leadingZeros = false; // Use 'true' if you'd like to keep the leading zeros
  bool disableDecPoint = true; // Use 'true' if your decimal point doesn't exist or isn't connected
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments, updateWithDelays, leadingZeros, disableDecPoint);
  sevseg.setBrightness(90);

  //Fan PWM init
  // generate 25kHz PWM pulse rate on Pin 3
  pinMode(pwmPin, OUTPUT);   // OCR2B sets duty cycle
  // Set up Fast PWM on Pin 3
  TCCR2A = 0x23;     // COM2B1, WGM21, WGM20
  // Set prescaler 
  TCCR2B = 0x0A;   // WGM21, Prescaler = /8
  // Set TOP and initialize duty cycle to zero(0)
  OCR2A = 79;    // TOP DO NOT CHANGE, SETS PWM PULSE RATE
  OCR2B = 0;    // duty cycle for Pin 6 (0-79) generates 1 500nS pulse even when 0 :
  digitalWrite(tachPin, HIGH);   // Starts reading fan speed

  //Serial.begin(9600);
  }

void loop() {
  diskTemperature = getTemp();
  sevseg.setNumber(diskTemperature);
  if (diskTemperature < minTemp) {
    speed = minSpeed;
  } else if (diskTemperature > maxTemp) {
    speed = maxSpeed;
  } else {
    speed = map(diskTemperature, minTemp, maxTemp, minSpeed, maxSpeed);
  }
  //Serial.println(speed);
  setFanSpeed(speed);
  sevseg.refreshDisplay(); // Must run repeatedly
}

int getRPMS() {
  time = pulseIn(tachPin, HIGH);
  rpm = (1000000 * 60) / (time * 4);
  stringRPM = String(rpm);
  if (stringRPM.length() < 5) {
    Serial.print("Fan Speed: ");
    Serial.print(rpm, DEC);
    Serial.println("rpm");
  }
  return rpm;
}

float getTemp() {
  vo = analogRead(thermistorPin);
  r2 = r1 * (1023.0 / (float)vo - 1.0);
  logR2 = log(r2);
  t = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  t = t - 273.15;
  //Serial.print("Temperature: "); 
  //Serial.print(t);
  //Serial.println(" C");
  return t;
}

void setFanSpeed(unsigned int speed){
  OCR2B = speed;    // set duty cycle (0 to 80)
}
