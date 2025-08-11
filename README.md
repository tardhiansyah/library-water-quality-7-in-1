# WaterQuality7in1 Arduino Library

This library provides an interface for Tuya-based 7-in-1 water quality sensors (temperature, pH, TDS, and thresholds) using an ESP8266 (ESP-12S) as a drop-in replacement for the original Tuya CB3S module.

## Hardware Requirements

- **ESP8266 (ESP-12S)** module (used as a replacement for Tuya CB3S)
- Only the following pins need to be connected and soldered:
  - **VCC**
  - **GND**
  - **RX**
  - **TX**

## Features

- Read temperature, pH, and TDS values
- Set and get threshold values for each parameter
- Query sensor status
- Callback for real-time sensor data updates

## Dependencies

- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) (version ^7.0.0)

## Example

See [`examples/Simple/Simple.cpp`](examples/Simple/Simple.cpp):

```cpp
#include <Arduino.h>
#include <tuya_water_quality.h>

TuyaWaterQuality waterQuality;

void onSensorData(TuyaWaterQualitySensorData &data) {
  Serial.print("Temperature: ");
  Serial.print(data.temperature.value);
  Serial.print(" C, pH: ");
  Serial.print(data.ph.value);
  Serial.print(", TDS: ");
  Serial.println(data.tds.value);
}

void setup() {
  Serial.begin(9600);
  waterQuality.begin(&Serial);
  waterQuality.onSensorData(onSensorData);
}

void loop() {
  waterQuality.loop();
  delay(1000);
}
```

## Usage Notes

- This library is **only for ESP8266 (ESP-12S)** and is intended to be used as a firmware replacement for the Tuya CB3S chip.
- Ensure you connect the ESP-12S module's RX/TX pins to the corresponding TX/RX pins on the sensor board.
- Power the ESP-12S with 3.3V (VCC and GND).

## License

MIT License
