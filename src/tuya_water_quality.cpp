#include "tuya_water_quality.h"

TuyaWaterQuality::TuyaWaterQuality() : Tuya()
{
  _onSensorDataCallback = nullptr;
  _sensorData = {
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
  };
}

bool TuyaWaterQuality::queryStatus()
{
  TuyaFrame frame = createFrame(TuyaDeviceType::Module, TuyaCommand::QueryDpStatus);
  return sendFrame(frame);
}

double TuyaWaterQuality::getTemperature() const
{
  return _sensorData.temperature.value;
}

double TuyaWaterQuality::getPh() const
{
  return _sensorData.ph.value;
}

int32_t TuyaWaterQuality::getTds() const
{
  return static_cast<int32_t>(_sensorData.tds.value);
}

double TuyaWaterQuality::getMaxTemperature() const
{
  return _sensorData.temperature.maxThreshold;
}

double TuyaWaterQuality::getMinTemperature() const
{
  return _sensorData.temperature.minThreshold;
}

double TuyaWaterQuality::getMaxPh() const
{
  return _sensorData.ph.maxThreshold;
}

double TuyaWaterQuality::getMinPh() const
{
  return _sensorData.ph.minThreshold;
}

int32_t TuyaWaterQuality::getMaxTds() const
{
  return static_cast<int32_t>(_sensorData.tds.maxThreshold);
}

int32_t TuyaWaterQuality::getMinTds() const
{
  return static_cast<int32_t>(_sensorData.tds.minThreshold);
}

bool TuyaWaterQuality::setMaxTemperature(double value)
{
  return setThreshold(TuyaWaterQualityDp::HighTemperatureThreshold, value);
}

bool TuyaWaterQuality::setMinTemperature(double value)
{
  return setThreshold(TuyaWaterQualityDp::LowTemperatureThreshold, value);
}

bool TuyaWaterQuality::setMaxPh(double value)
{
  return setThreshold(TuyaWaterQualityDp::HighPHThreshold, value);
}

bool TuyaWaterQuality::setMinPh(double value)
{
  return setThreshold(TuyaWaterQualityDp::LowPHThreshold, value);
}

bool TuyaWaterQuality::setMaxTds(int32_t value)
{
  return setThreshold(TuyaWaterQualityDp::HighTDSThreshold, value);
}

bool TuyaWaterQuality::setMinTds(int32_t value)
{
  return setThreshold(TuyaWaterQualityDp::LowTDSThreshold, value);
}

void TuyaWaterQuality::onSensorData(void (*callback)(TuyaWaterQualitySensorData &sensorData))
{
  _onSensorDataCallback = callback;
}

bool TuyaWaterQuality::decodeReportStatusAsync(TuyaFrame &frame)
{
  TuyaWaterQualityDp dpId = static_cast<TuyaWaterQualityDp>(frame.data[0]);
  TuyaDataType dataType = static_cast<TuyaDataType>(frame.data[1]);
  if (dataType != TuyaDataType::Value)
    return false;

  uint32_t rawValue = decodeSensorRawValue(frame.data);

  switch (dpId)
  {
  case TuyaWaterQualityDp::Temperature:
    _sensorData.temperature.value = rawValue / 10.0;
    break;
  case TuyaWaterQualityDp::HighTemperatureThreshold:
    _sensorData.temperature.maxThreshold = rawValue / 10.0;
    break;
  case TuyaWaterQualityDp::LowTemperatureThreshold:
    _sensorData.temperature.minThreshold = rawValue / 10.0;
    break;
  case TuyaWaterQualityDp::PH:
    _sensorData.ph.value = rawValue / 100.0;
    break;
  case TuyaWaterQualityDp::HighPHThreshold:
    _sensorData.ph.maxThreshold = rawValue / 100.0;
    break;
  case TuyaWaterQualityDp::LowPHThreshold:
    _sensorData.ph.minThreshold = rawValue / 100.0;
    break;
  case TuyaWaterQualityDp::TDS:
    _sensorData.tds.value = rawValue;
    break;
  case TuyaWaterQualityDp::HighTDSThreshold:
    _sensorData.tds.maxThreshold = rawValue;
    break;
  case TuyaWaterQualityDp::LowTDSThreshold:
    _sensorData.tds.minThreshold = rawValue;
    break;
  default:
    return false;
  }

  if (_onSensorDataCallback != nullptr)
  {
    _onSensorDataCallback(_sensorData);
  }

  return true;
}

uint32_t TuyaWaterQuality::decodeSensorRawValue(const uint8_t *data) const
{
  return (static_cast<uint32_t>(data[4]) << 24) |
         (static_cast<uint32_t>(data[5]) << 16) |
         (static_cast<uint32_t>(data[6]) << 8) |
         (static_cast<uint32_t>(data[7]));
}

bool TuyaWaterQuality::setThreshold(TuyaWaterQualityDp dp, double value)
{
  // For temperature and pH, value is scaled (x10 or x100)
  int32_t intValue;
  if (dp == TuyaWaterQualityDp::HighTemperatureThreshold || dp == TuyaWaterQualityDp::LowTemperatureThreshold)
    intValue = static_cast<int32_t>(value * 10);
  else if (dp == TuyaWaterQualityDp::HighPHThreshold || dp == TuyaWaterQualityDp::LowPHThreshold)
    intValue = static_cast<int32_t>(value * 100);
  else
    return false;
  return setThreshold(dp, intValue);
}

bool TuyaWaterQuality::setThreshold(TuyaWaterQualityDp dp, int32_t value)
{
  uint8_t data[8];
  if (!buildSensorDataPayload(data, dp, value))
    return false;

  TuyaFrame frame = createFrame(TuyaDeviceType::Module, TuyaCommand::SendCommand, data, sizeof(data));
  return sendFrame(frame);
}

bool TuyaWaterQuality::buildSensorDataPayload(uint8_t (&buffer)[8], TuyaWaterQualityDp dp, int32_t value) const
{
  constexpr uint8_t VALUE_LENGTH = 4;
  buffer[0] = static_cast<uint8_t>(dp);
  buffer[1] = static_cast<uint8_t>(TuyaDataType::Value);
  buffer[2] = (VALUE_LENGTH >> 8) & 0xFF;
  buffer[3] = VALUE_LENGTH & 0xFF;
  buffer[4] = (value >> 24) & 0xFF;
  buffer[5] = (value >> 16) & 0xFF;
  buffer[6] = (value >> 8) & 0xFF;
  buffer[7] = value & 0xFF;
  return true;
}