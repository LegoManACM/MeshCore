#include "SerialBLEInterface.h"
#include <Arduino.h>

#define BLE_HEALTH_CHECK_INTERVAL 10000

void SerialBLEInterface::onConnect(BLEServer* p) {
  (void)p;
  Serial.println("BLE: connected");
  _isConnected = true;
  send_queue_len = 0;
  recv_queue_len = 0;
}

void SerialBLEInterface::onDisconnect(BLEServer* p) {
  (void)p;
  Serial.println("BLE: disconnected");
  _isConnected = false;
  send_queue_len = 0;
  recv_queue_len = 0;
  if (_isEnabled) {
    BLE.startAdvertising();
  }
}

void SerialBLEInterface::shiftSendQueueLeft() {
  if (send_queue_len > 0) {
    send_queue_len--;
    for (uint8_t i = 0; i < send_queue_len; i++) {
      send_queue[i] = send_queue[i + 1];
    }
  }
}

void SerialBLEInterface::shiftRecvQueueLeft() {
  if (recv_queue_len > 0) {
    recv_queue_len--;
    for (uint8_t i = 0; i < recv_queue_len; i++) {
      recv_queue[i] = recv_queue[i + 1];
    }
  }
}

void SerialBLEInterface::begin(const char* prefix, char* name, uint32_t pin_code) {
  (void)pin_code;  // TODO: add security/PIN support later

  char dev_name[48];
  snprintf(dev_name, sizeof(dev_name), "%s%s", prefix, name);

  BLE.begin(dev_name);

  _server = BLE.server();
  _server->setCallbacks(this);
  _server->addService(&_uart);
  _uart.begin();
  _uart.setAutoflush(0);  // disable autoflush so we control when data is sent

  BLE.setSecurity(BLESecurityNone);

  enable();
}

void SerialBLEInterface::enable() {
  if (_isEnabled) return;
  _isEnabled = true;
  _last_health_check = millis();
  BLE.startAdvertising();
}

void SerialBLEInterface::disable() {
  _isEnabled = false;
  _isConnected = false;
  BLE.stopAdvertising();
}

size_t SerialBLEInterface::writeFrame(const uint8_t src[], size_t len) {
  if (!_isConnected || len == 0 || len > MAX_FRAME_SIZE) return 0;
  if (send_queue_len >= FRAME_QUEUE_SIZE) {
    BLE_DEBUG_PRINTLN("send queue full");
    return 0;
  }
  send_queue[send_queue_len].len = len;
  memcpy(send_queue[send_queue_len].buf, src, len);
  send_queue_len++;
  return len;
}

size_t SerialBLEInterface::checkRecvFrame(uint8_t dest[]) {
  // drain send queue
  if (send_queue_len > 0 && _isConnected) {
    Frame& f = send_queue[0];
    size_t written = _uart.write(f.buf, f.len);
    _uart.flush();
    Serial.printf("BLE: wrote %d of %d bytes\n", written, f.len);
    if (written == f.len) {
      shiftSendQueueLeft();
    }
  }

  // read incoming
  int avail = _uart.available();
  if (avail > 0) {
    Serial.printf("BLE: %d bytes available\n", avail);
    if (recv_queue_len < FRAME_QUEUE_SIZE) {
      if (avail > MAX_FRAME_SIZE) avail = MAX_FRAME_SIZE;
      recv_queue[recv_queue_len].len = avail;
      _uart.readBytes(recv_queue[recv_queue_len].buf, avail);
      recv_queue_len++;
    }
  }

  if (recv_queue_len > 0) {
    size_t len = recv_queue[0].len;
    Serial.printf("BLE: returning frame len=%d hdr=0x%02x\n", len, recv_queue[0].buf[0]);
    memcpy(dest, recv_queue[0].buf, len);
    shiftRecvQueueLeft();
    return len;
  }

  // advertising watchdog
  if (_isEnabled && !_isConnected) {
    unsigned long now = millis();
    if (now - _last_health_check >= BLE_HEALTH_CHECK_INTERVAL) {
      _last_health_check = now;
      BLE.startAdvertising();
    }
  }

  return 0;
}