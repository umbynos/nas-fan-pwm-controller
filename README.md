# nas-fan-pwm-controller
Implementation of a fan pwm controller regulated by NTC thermistor and equipped with 7 segment display used in my home NAS

## Hardware
- Arduino Nano (ATmega328P, FT232R USB-serial). The sketch drives the fan with Timer2 registers, so it does **not** build for ATmega32U4 boards such as the Arduino Micro or Leonardo.
- 4-pin PWM fan
- 10kΩ NTC thermistor in a voltage divider with a 10kΩ resistor
- 2-digit common anode 7 segment display, resistors on the digit pins

| Function | Pin |
| --- | --- |
| Fan PWM (25kHz, OC2B) | D3 |
| Fan tachometer (not used yet) | D12 |
| Thermistor | A0 |
| Display digits | D11, D10 |
| Display segments A–G | D5, D6, D2, D9, D8, D7, D4 |

## Behaviour
- The thermistor is sampled every 20ms and smoothed with a moving average. Temperature, display and fan speed are updated once per second.
- The display only changes when the reading moves by at least 0.7°C, so it doesn't bounce between two values.
- The fan is off below 25°C, at full speed above 45°C and scaled linearly in between.
- At power on, and whenever the thermistor reads as disconnected or shorted, the fan runs at full speed and the display shows `--`.

The thresholds and intervals are constants at the top of the sketch.

## Build and upload
Requires the [SevSeg](https://github.com/DeanIsMe/SevSeg) library.

```sh
arduino-cli core install arduino:avr
arduino-cli lib install SevSeg
arduino-cli compile -b arduino:avr:nano:cpu=atmega328 -u -p /dev/ttyUSB0 nas-fan-pwm-controller
```

Use `cpu=atmega328old` if your Nano still has the old bootloader.
