#pragma once

#include "../BaseSerialInterface.h"
#include <BLE.h>
#include <BLEServer.h>
#include <BLEServiceUART.h>

#ifndef BLE_TX_POWER
#define BLE_TX_POWER 4
#endif

#if BLE_DEBUG_LOGGING && ARDUINO
  #include <Arduino.h>
  #define BLE_DEBUG_PRINTLN(F, ...) Serial.printf("BLE: " F "\n", ##__VA_ARGS__)
#else
  #define BLE_DEBUG_PRINTLN(...) {}
#endif

class SerialBLEInterface : public BaseSerialInterface, public BLEServerCallbacks {
  BLEServer* _server = nullptr;
  BLEServiceUART _uart;
  bool _isEnabled = false;
  bool _isConnected = false;
  unsigned long _last_health_check = 0;

  struct Frame {
    uint8_t len;
    uint8_t buf[MAX_FRAME_SIZE];
  };

  #define FRAME_QUEUE_SIZE 12

  uint8_t send_queue_len = 0;
  Frame send_queue[FRAME_QUEUE_SIZE];

  uint8_t recv_queue_len = 0;
  Frame recv_queue[FRAME_QUEUE_SIZE];

  void shiftSendQueueLeft();
  void shiftRecvQueueLeft();

public:
  SerialBLEInterface() {}

  void begin(const char* prefix, char* name, uint32_t pin_code);

  // BLEServerCallbacks
  void onConnect(BLEServer* p) override;
  void onDisconnect(BLEServer* p) override;

  void enable() override;
  void disable() override;
  bool isEnabled() const override { return _isEnabled; }
  bool isConnected() const override { return _isConnected; }
  bool isWriteBusy() const override { return send_queue_len >= (FRAME_QUEUE_SIZE * 2 / 3); }
  size_t writeFrame(const uint8_t src[], size_t len) override;
  size_t checkRecvFrame(uint8_t dest[]) override;
};