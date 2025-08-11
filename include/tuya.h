#pragma once

#include <Arduino.h>
#include <Stream.h>

// =======================
// Enums
// =======================

enum class TuyaError
{
  None = 0,
  NoData,
  Checksum,
  Overflow,
};

enum class TuyaDataType : uint8_t
{
  Raw = 0x00,
  Boolean = 0x01,
  Value = 0x02,
  String = 0x03,
  Enum = 0x04,
  Bitmap = 0x05,
};

enum class TuyaDeviceType : uint8_t
{
  Module = 0x00,
  MCU = 0x03,
};

enum class TuyaCommand : uint8_t
{
  Heartbeats = 0x00,
  QueryProductInfo = 0x01,
  QueryWorkingMode = 0x02,
  ReportNetworkStatus = 0x03,
  ResetWiFi = 0x04,
  ResetWiFiPairMode = 0x05,
  SendCommand = 0x06,
  ReportStatusAsync = 0x07,
  QueryDpStatus = 0x08,
  StartOta = 0x0A,
  TransmitOtaData = 0x0B,
  GetGmtTime = 0x0C,
  TestWiFiScanning = 0x0E,
  GetModuleMemory = 0x0F,
  GetLocalTime = 0x1C,
  EnableWeatherServices = 0x20,
  SendWeatherData = 0x21,
  ReportStatusSync = 0x22,
  ResponseStatusSync = 0x23,
  GetWiFiSignalStrength = 0x24,
  DisableHeartbeats = 0x25,
  PairViaSerialPort = 0x2A,
  GetCurrentNetworkStatus = 0x2B,
  TestWiFiConnection = 0x2C,
  GetModuleMacAddress = 0x2D,
  ExtendedServices = 0x34,
  BluetoothPairing = 0x35,
  ReportSendExtendedDp = 0x36,
  NewFeatureSetting = 0x37,
};

enum class TuyaNetworkStatus : uint8_t
{
  PairingEzMode = 0x00,
  PairingApMode = 0x01,
  WiFiNotConnected = 0x02,
  WiFiConnected = 0x03,
  CloudConnected = 0x04,
  LowPowerMode = 0x05,
  EzApConfigMode = 0x06,
};

// =======================
// Structs
// =======================

struct TuyaFrame
{
  uint8_t header[2];
  uint8_t version;
  uint8_t command;
  uint8_t length[2];
  uint8_t data[1024];
  uint8_t checksum;
};

struct TuyaProductInfo
{
  String productId;
  String version;
  uint16_t operationMode;
};

struct TuyaModuleInfo
{
  TuyaProductInfo productInfo;
  TuyaNetworkStatus networkStatus;
  bool heartbeatsReceived;
  bool productInfoReceived;
  bool workingModeReceived;
  bool initialized;
};

// =======================
// Tuya Class
// =======================

class Tuya
{
public:
  Tuya();

  // Core API
  void begin(Stream *serial);
  void loop();

  // Configuration
  void enableDebug(Stream &debugStream, bool enable);
  void setDelay(uint32_t delayMs);
  void setNetworkStatus(TuyaNetworkStatus status);

  // State
  bool isInitialized() const;
  TuyaNetworkStatus getNetworkStatus() const;
  TuyaProductInfo getProductInfo() const;

  // Event
  void onResetWiFiPairMode(void (*callback)());

protected:
  // Decoding
  virtual bool decodeHeartbeats(TuyaFrame &frame);
  virtual bool decodeProductInfo(TuyaFrame &frame);
  virtual bool decodeQueryWorkingMode(TuyaFrame &frame);
  virtual bool decodeReportStatusAsync(TuyaFrame &frame);

  // Frame helpers
  TuyaFrame createFrame(TuyaDeviceType deviceType, TuyaCommand command, uint8_t *data, uint16_t dataLength) const;
  TuyaFrame createFrame(TuyaDeviceType deviceType, TuyaCommand command) const;
  bool sendFrame(const TuyaFrame &frame) const;

private:
  // Serial
  Stream *_serial = nullptr;
  Stream *_debugStream = nullptr;

  // State
  TuyaModuleInfo _moduleInfo;
  uint32_t _delayMs = 250;
  uint32_t _heartbeatIntervalMs = 1000;
  uint32_t _lastHeartbeatMs = 0;
  bool _debugEnabled = false;
  void (*_resetWiFiPairModeCallback)() = nullptr;

  // Internal helpers
  TuyaError receiveMessage(TuyaFrame &frame);
  bool validateChecksum(const TuyaFrame &frame) const;
  uint8_t calculateChecksum(const TuyaFrame &frame) const;

  void decodeFrame(TuyaFrame &frame);
  void printFrame(const TuyaFrame &frame) const;

  // Command handlers
  void handleHeartbeats(TuyaFrame &frame);
  void handleQueryProductInfo(TuyaFrame &frame);
  void handleQueryWorkingMode(TuyaFrame &frame);
  void handleReportNetworkStatus(TuyaFrame &frame);
  void handleReportStatusAsync(TuyaFrame &frame);
  void handleGetCurrentNetworkStatus(TuyaFrame &frame);
  void handleResetWiFiPairMode(TuyaFrame &frame);
  void handleUnknownCommand(TuyaFrame &frame);

  // Communication
  void sendNetworkStatus() const;
  void reportNetworkStatus() const;
  void sendHeartbeats() const;
  void queryProductInfo() const;
  void queryWorkingMode() const;

  // Utility
  String dataToString(uint8_t *data, uint16_t length) const;
};