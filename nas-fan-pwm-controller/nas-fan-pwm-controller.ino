#include "SevSeg.h"
SevSeg sevseg; //Instantiate a seven segment controller object

const byte pwmPin = 3;        // OC2B, driven by Timer2 at 25kHz
const byte thermistorPin = A0;
const byte tachPin = 12;

// Steinhart-Hart coefficients for the NTC, r1 is the fixed resistor of the divider
const float r1 = 10000, c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

const int minTemp = 25, maxTemp = 45;
const byte pwmTop = 79;                    // Timer2 TOP: 16MHz / 8 / (79 + 1) = 25kHz
const byte minSpeed = 0, maxSpeed = pwmTop; // duty cycle range (0-79)

const unsigned long sampleInterval = 20;   // ms between ADC samples
const unsigned long updateInterval = 1000; // ms between temperature/fan/display updates
const float displayHysteresis = 0.7;       // °C the reading must move before the display changes

long adcFiltered = -1;   // exponential moving average of the ADC, scaled by 16
unsigned long lastSample, lastUpdate;
int shownTemperature = -100;

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
  TCCR2B = 0x0A;   // WGM22, Prescaler = /8
  // Set TOP and initialize duty cycle
  OCR2A = pwmTop;    // TOP DO NOT CHANGE, SETS PWM PULSE RATE
  OCR2B = maxSpeed;  // start at full speed until the first reading is available

  pinMode(tachPin, INPUT_PULLUP);

  //Serial.begin(9600);
}

void loop() {
  // Keep the multiplexing as regular as possible: only cheap work runs here
  sevseg.refreshDisplay();

  unsigned long now = millis();

  if (now - lastSample >= sampleInterval) {
    lastSample = now;
    sampleTemp();
  }

  if (now - lastUpdate >= updateInterval) {
    lastUpdate = now;
    update();
  }
}

// Single ADC read (~110us) fed into an integer EMA, cheap enough to not disturb the display
void sampleTemp() {
  long raw = (long)analogRead(thermistorPin) << 4;
  if (adcFiltered < 0) {
    adcFiltered = raw;
  } else {
    adcFiltered += (raw - adcFiltered) / 16;
  }
}

void update() {
  float temperature = getTemp();

  // Sensor disconnected or shorted: show "--" and run the fan at full speed
  if (isnan(temperature) || temperature < -20 || temperature > 99) {
    sevseg.setChars("--");
    shownTemperature = -100;
    setFanSpeed(maxSpeed);
    return;
  }

  // Change the shown value only when the reading clearly moved, so it doesn't bounce between two digits
  if (abs(temperature - shownTemperature) >= displayHysteresis) {
    shownTemperature = round(temperature);
    sevseg.setNumber(shownTemperature);
  }

  int speed;
  if (shownTemperature <= minTemp) {
    speed = minSpeed;
  } else if (shownTemperature >= maxTemp) {
    speed = maxSpeed;
  } else {
    speed = map(shownTemperature, minTemp, maxTemp, minSpeed, maxSpeed);
  }
  //Serial.println(speed);
  setFanSpeed(speed);
}

float getTemp() {
  float vo = adcFiltered / 16.0;
  if (vo < 1 || vo > 1022) {
    return NAN;
  }
  float r2 = r1 * (1023.0 / vo - 1.0);
  float logR2 = log(r2);
  float t = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  t = t - 273.15;
  //Serial.print("Temperature: ");
  //Serial.print(t);
  //Serial.println(" C");
  return t;
}

void setFanSpeed(byte speed) {
  OCR2B = speed;    // set duty cycle (0 to pwmTop)
}
