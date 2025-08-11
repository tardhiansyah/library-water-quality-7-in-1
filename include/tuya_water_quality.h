#pragma once

#include <Arduino.h>
#include <Stream.h>
#include <tuya.h>

// =======================
// Enums
// =======================

enum class TuyaWaterQualityDp : uint8_t
{
  Temperature = 0x08,
  HighTemperatureThreshold = 0x66,
  LowTemperatureThreshold = 0x67,
  PH = 0x6A,
  HighPHThreshold = 0x6B,
  LowPHThreshold = 0x6C,
  TDS = 0x6F,
  HighTDSThreshold = 0x70,
  LowTDSThreshold = 0x71,
};

// =======================
// Structs
// =======================

struct TuyaSensorValue
{
  double value;
  double maxThreshold;
  double minThreshold;
};

struct TuyaWaterQualitySensorData
{
  TuyaSensorValue temperature;
  TuyaSensorValue ph;
  TuyaSensorValue tds;
};

struct TuyaWaterQualityInfo
{
  String productId;
  String version;
  uint16_t operationMode;
};

// =======================
// TuyaWaterQuality Class
// =======================

class TuyaWaterQuality : public Tuya
{
public:
  TuyaWaterQuality();

  // Query
  bool queryStatus();

  // Getters
  double getTemperature() const;
  double getPh() const;
  int32_t getTds() const;

  double getMaxTemperature() const;
  double getMinTemperature() const;
  double getMaxPh() const;
  double getMinPh() const;
  int32_t getMaxTds() const;
  int32_t getMinTds() const;

  // Setters
  bool setMaxTemperature(double value);
  bool setMinTemperature(double value);
  bool setMaxPh(double value);
  bool setMinPh(double value);
  bool setMaxTds(int32_t value);
  bool setMinTds(int32_t value);

  // Event
  void onSensorData(void (*callback)(TuyaWaterQualitySensorData &sensorData));

protected:
  bool decodeReportStatusAsync(TuyaFrame &frame) override;

private:
  TuyaWaterQualitySensorData _sensorData;
  void (*_onSensorDataCallback)(TuyaWaterQualitySensorData &sensorData) = nullptr;

  uint32_t decodeSensorRawValue(const uint8_t *data) const;
  bool setThreshold(TuyaWaterQualityDp dp, double value);
  bool setThreshold(TuyaWaterQualityDp dp, int32_t value);
  bool buildSensorDataPayload(uint8_t (&buffer)[8], TuyaWaterQualityDp dp, int32_t value) const;
};