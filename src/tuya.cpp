#include <ArduinoJson.h>
#include "tuya.h"

Tuya::Tuya()
    : _serial(nullptr),
      _debugStream(nullptr),
      _moduleInfo{
          .productInfo = {
              .productId = "",
              .version = "",
              .operationMode = 0,
          },
          .networkStatus = TuyaNetworkStatus::WiFiNotConnected,
          .heartbeatsReceived = false,
          .productInfoReceived = false,
          .workingModeReceived = false,
          .initialized = false},
      _delayMs(250), _heartbeatIntervalMs(1000), _lastHeartbeatMs(0), _debugEnabled(false), _resetWiFiPairModeCallback(nullptr)
{
}

void Tuya::begin(Stream *serial)
{
  _serial = serial;
}

void Tuya::loop()
{
  if (_serial == nullptr)
  {
    return;
  }

  uint32_t intervalHeartbeats = _moduleInfo.heartbeatsReceived ? 15000 : 1000;
  if (millis() - _lastHeartbeatMs > intervalHeartbeats)
  {
    sendHeartbeats();
    _lastHeartbeatMs = millis();
  }

  if (_moduleInfo.heartbeatsReceived && !_moduleInfo.productInfoReceived)
  {
    queryProductInfo();
  }

  if (_moduleInfo.heartbeatsReceived && !_moduleInfo.workingModeReceived)
  {
    queryWorkingMode();
  }

  TuyaFrame frame;
  if (receiveMessage(frame) == TuyaError::None)
  {
    decodeFrame(frame);
  }

  _moduleInfo.initialized = _moduleInfo.heartbeatsReceived &&
                            _moduleInfo.productInfoReceived &&
                            _moduleInfo.workingModeReceived;

  delay(_delayMs);
}

void Tuya::enableDebug(Stream &debugStream, bool enable)
{
  _debugEnabled = enable;
  _debugStream = _debugEnabled ? &debugStream : nullptr;
}

void Tuya::setDelay(uint32_t delayMs)
{
  _delayMs = delayMs;
}

bool Tuya::isInitialized() const
{
  return _moduleInfo.initialized;
}

TuyaNetworkStatus Tuya::getNetworkStatus() const
{
  return _moduleInfo.networkStatus;
}

TuyaProductInfo Tuya::getProductInfo() const
{
  return _moduleInfo.productInfo;
}

void Tuya::onResetWiFiPairMode(void (*callback)())
{
  _resetWiFiPairModeCallback = callback;
}

TuyaError Tuya::receiveMessage(TuyaFrame &frame)
{
  while (_serial->available())
  {
    frame.header[0] = _serial->read();
    if (frame.header[0] == 0x55)
    {
      frame.header[1] = _serial->read();
      if (frame.header[1] == 0xAA)
      {
        frame.version = _serial->read();
        frame.command = _serial->read();
        frame.length[0] = _serial->read();
        frame.length[1] = _serial->read();
        uint16_t dataLength = (frame.length[0] << 8) | frame.length[1];
        if (dataLength > sizeof(frame.data))
        {
          _serial->flush();
          return TuyaError::Overflow;
        }
        if (dataLength > 0)
        {
          _serial->readBytes(frame.data, dataLength);
        }
        frame.checksum = _serial->read();
        if (!validateChecksum(frame))
        {
          _serial->flush();
          return TuyaError::Checksum;
        }
        _serial->flush();
        return TuyaError::None;
      }
    }
  }
  return TuyaError::NoData;
}

bool Tuya::validateChecksum(const TuyaFrame &frame) const
{
  uint16_t sum = frame.header[0] + frame.header[1] + frame.version + frame.command + frame.length[0] + frame.length[1];
  uint16_t len = (frame.length[0] << 8) | frame.length[1];
  for (uint16_t i = 0; i < len; i++)
  {
    sum += frame.data[i];
  }
  return (sum % 256) == frame.checksum;
}

uint8_t Tuya::calculateChecksum(const TuyaFrame &frame) const
{
  uint16_t sum = frame.header[0] + frame.header[1] + frame.version + frame.command + frame.length[0] + frame.length[1];
  uint16_t len = (frame.length[0] << 8) | frame.length[1];
  for (uint16_t i = 0; i < len; i++)
  {
    sum += frame.data[i];
  }
  return (sum % 256);
}

TuyaFrame Tuya::createFrame(TuyaDeviceType deviceType, TuyaCommand command, uint8_t *data, uint16_t dataLength) const
{
  TuyaFrame frame{};
  frame.header[0] = 0x55;
  frame.header[1] = 0xAA;
  frame.version = static_cast<uint8_t>(deviceType);
  frame.command = static_cast<uint8_t>(command);
  frame.length[0] = (dataLength >> 8) & 0xFF;
  frame.length[1] = dataLength & 0xFF;
  if (dataLength > 0 && data != nullptr)
  {
    memcpy(frame.data, data, dataLength);
  }
  frame.checksum = calculateChecksum(frame);
  return frame;
}

TuyaFrame Tuya::createFrame(TuyaDeviceType deviceType, TuyaCommand command) const
{
  return createFrame(deviceType, command, nullptr, 0);
}

bool Tuya::sendFrame(const TuyaFrame &frame) const
{
  if (!_serial)
    return false;
  uint16_t len = (frame.length[0] << 8) | frame.length[1];
  _serial->write(frame.header[0]);
  _serial->write(frame.header[1]);
  _serial->write(frame.version);
  _serial->write(frame.command);
  _serial->write(frame.length[0]);
  _serial->write(frame.length[1]);
  if (len > 0)
  {
    _serial->write(frame.data, len);
  }
  _serial->write(frame.checksum);
  _serial->flush();
  return true;
}

void Tuya::decodeFrame(TuyaFrame &frame)
{
  printFrame(frame);

  switch (static_cast<TuyaCommand>(frame.command))
  {
  case TuyaCommand::Heartbeats:
    handleHeartbeats(frame);
    break;
  case TuyaCommand::QueryProductInfo:
    handleQueryProductInfo(frame);
    break;
  case TuyaCommand::QueryWorkingMode:
    handleQueryWorkingMode(frame);
    break;
  case TuyaCommand::ReportNetworkStatus:
    handleReportNetworkStatus(frame);
    break;
  case TuyaCommand::ReportStatusAsync:
    handleReportStatusAsync(frame);
    break;
  case TuyaCommand::GetCurrentNetworkStatus:
    handleGetCurrentNetworkStatus(frame);
    break;
  case TuyaCommand::ResetWiFiPairMode:
    handleResetWiFiPairMode(frame);
    break;
  default:
    handleUnknownCommand(frame);
    break;
  }
}

void Tuya::printFrame(const TuyaFrame &frame) const
{
  if (_debugEnabled && _debugStream)
  {
    _debugStream->print("Received frame: ");
    _debugStream->print("header[0]: ");
    _debugStream->print(frame.header[0], HEX);
    _debugStream->print(", header[1]: ");
    _debugStream->print(frame.header[1], HEX);
    _debugStream->print(", version: ");
    _debugStream->print(frame.version, HEX);
    _debugStream->print(", command: ");
    _debugStream->print(frame.command, HEX);
    _debugStream->print(", length[0]: ");
    _debugStream->print(frame.length[0], HEX);
    _debugStream->print(", length[1]: ");
    _debugStream->print(frame.length[1], HEX);
    _debugStream->print(", data: ");
    uint16_t len = (frame.length[0] << 8) | frame.length[1];
    for (uint16_t i = 0; i < len; i++)
    {
      _debugStream->print(frame.data[i], HEX);
      _debugStream->print(" ");
    }
    _debugStream->print(", checksum: ");
    _debugStream->println(frame.checksum, HEX);
  }
}

void Tuya::setNetworkStatus(TuyaNetworkStatus status)
{
  _moduleInfo.networkStatus = status;
  reportNetworkStatus();
}

void Tuya::reportNetworkStatus() const
{
  if (_debugEnabled && _debugStream)
  {
    _debugStream->println("Reporting network status");
  }

  uint8_t data[1] = {static_cast<uint8_t>(_moduleInfo.networkStatus)};
  TuyaFrame frame = createFrame(TuyaDeviceType::Module, TuyaCommand::ReportNetworkStatus, data, sizeof(data));
  sendFrame(frame);
}

void Tuya::sendNetworkStatus() const
{
  uint8_t data[1] = {static_cast<uint8_t>(_moduleInfo.networkStatus)};
  TuyaFrame frame = createFrame(TuyaDeviceType::Module, TuyaCommand::GetCurrentNetworkStatus, data, sizeof(data));
  sendFrame(frame);
}

void Tuya::sendHeartbeats() const
{
  TuyaFrame frame = createFrame(TuyaDeviceType::Module, TuyaCommand::Heartbeats);
  sendFrame(frame);
}

void Tuya::queryProductInfo() const
{
  TuyaFrame frame = createFrame(TuyaDeviceType::Module, TuyaCommand::QueryProductInfo);
  sendFrame(frame);
  delay(_delayMs);
}

void Tuya::queryWorkingMode() const
{
  TuyaFrame frame = createFrame(TuyaDeviceType::Module, TuyaCommand::QueryWorkingMode);
  sendFrame(frame);
  delay(_delayMs);
}

bool Tuya::decodeHeartbeats(TuyaFrame &)
{
  return true;
}

bool Tuya::decodeProductInfo(TuyaFrame &frame)
{
  uint16_t length = (frame.length[0] << 8) | frame.length[1];
  String productInfoStr = dataToString(frame.data, length);

  JsonDocument json;
  if (deserializeJson(json, productInfoStr))
  {
    return false;
  }

  _moduleInfo.productInfo.productId = json["product_id"] | "";
  _moduleInfo.productInfo.version = json["version"] | "";
  _moduleInfo.productInfo.operationMode = json["operation_mode"] | 0;

  return true;
}

bool Tuya::decodeQueryWorkingMode(TuyaFrame &)
{
  return true;
}

bool Tuya::decodeReportStatusAsync(TuyaFrame &)
{
  return true;
}

String Tuya::dataToString(uint8_t *data, uint16_t length) const
{
  String result;
  result.reserve(length);
  for (uint16_t i = 0; i < length; i++)
  {
    result += static_cast<char>(data[i]);
  }
  return result;
}

void Tuya::handleHeartbeats(TuyaFrame &frame)
{
  if (_debugEnabled && _debugStream)
  {
    _debugStream->println("Received heartbeats");
  }
  _moduleInfo.heartbeatsReceived = decodeHeartbeats(frame);
}

void Tuya::handleQueryProductInfo(TuyaFrame &frame)
{
  if (_debugEnabled && _debugStream)
  {
    _debugStream->println("Received query product info");
  }
  _moduleInfo.productInfoReceived = decodeProductInfo(frame);
}

void Tuya::handleQueryWorkingMode(TuyaFrame &frame)
{
  if (_debugEnabled && _debugStream)
  {
    _debugStream->println("Received query working mode");
  }
  _moduleInfo.workingModeReceived = decodeQueryWorkingMode(frame);
}

void Tuya::handleReportNetworkStatus(TuyaFrame &)
{
  if (_debugEnabled && _debugStream)
  {
    _debugStream->println("Received report network status");
  }
  // No action needed
}

void Tuya::handleReportStatusAsync(TuyaFrame &frame)
{
  if (_debugEnabled && _debugStream)
  {
    _debugStream->println("Received report status");
  }
  decodeReportStatusAsync(frame);
}

void Tuya::handleGetCurrentNetworkStatus(TuyaFrame &)
{
  if (_debugEnabled && _debugStream)
  {
    _debugStream->println("Received get current network status");
  }
  sendNetworkStatus();
}

void Tuya::handleResetWiFiPairMode(TuyaFrame &)
{
  if (_debugEnabled && _debugStream)
  {
    _debugStream->println("Received reset WiFi pair mode");
  }

  if (_resetWiFiPairModeCallback != nullptr)
  {
    _resetWiFiPairModeCallback();
  }
}

void Tuya::handleUnknownCommand(TuyaFrame &)
{
  if (_debugEnabled && _debugStream)
  {
    _debugStream->println("Received unknown command");
  }
}