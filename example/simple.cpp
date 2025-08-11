#include <Arduino.h>
#include <tuya_water_quality.h>

TuyaWaterQuality waterQuality;

void onSensorData(TuyaWaterQualitySensorData &data)
{
    Serial.print("[Sensor Data] Temp: ");
    Serial.print(data.temperature.value);
    Serial.print(" C (min: ");
    Serial.print(data.temperature.minThreshold);
    Serial.print(", max: ");
    Serial.print(data.temperature.maxThreshold);
    Serial.print("), pH: ");
    Serial.print(data.ph.value);
    Serial.print(" (min: ");
    Serial.print(data.ph.minThreshold);
    Serial.print(", max: ");
    Serial.print(data.ph.maxThreshold);
    Serial.print("), TDS: ");
    Serial.print(data.tds.value);
    Serial.print(" (min: ");
    Serial.print(data.tds.minThreshold);
    Serial.print(", max: ");
    Serial.print(data.tds.maxThreshold);
    Serial.println(")");
}

void setup()
{
    Serial.begin(115200);
    Serial1.begin(9600);
    waterQuality.begin(&Serial1);
    waterQuality.onSensorData(onSensorData);

    // Enable debug output to Serial
    waterQuality.enableDebug(Serial1, true);

    // Set custom thresholds
    waterQuality.setMaxTemperature(30.0);
    waterQuality.setMinTemperature(10.0);
    waterQuality.setMaxPh(8.0);
    waterQuality.setMinPh(6.0);
    waterQuality.setMaxTds(1200);
    waterQuality.setMinTds(100);
}

void loop()
{
    waterQuality.loop();

    // Print current values and thresholds every 5 seconds
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 5000)
    {
        lastPrint = millis();

        Serial.println("--- Current Water Quality ---");
        Serial.print("Temperature: ");
        Serial.print(waterQuality.getTemperature());
        Serial.print(" C (min: ");
        Serial.print(waterQuality.getMinTemperature());
        Serial.print(", max: ");
        Serial.print(waterQuality.getMaxTemperature());
        Serial.println(")");

        Serial.print("pH: ");
        Serial.print(waterQuality.getPh());
        Serial.print(" (min: ");
        Serial.print(waterQuality.getMinPh());
        Serial.print(", max: ");
        Serial.print(waterQuality.getMaxPh());
        Serial.println(")");

        Serial.print("TDS: ");
        Serial.print(waterQuality.getTds());
        Serial.print(" (min: ");
        Serial.print(waterQuality.getMinTds());
        Serial.print(", max: ");
        Serial.print(waterQuality.getMaxTds());
        Serial.println(")");
        Serial.println("----------------------------");
    }
}