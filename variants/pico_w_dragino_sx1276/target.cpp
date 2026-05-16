#include "target.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/sensors/MicroNMEALocationProvider.h>
#include <helpers/sensors/EnvironmentSensorManager.h>

WaveshareBoard board;

RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_0, P_LORA_RST, P_LORA_DIO_1, SPI);
WRAPPER_CLASS radio_driver(radio, board);

VolatileRTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);

MicroNMEALocationProvider nmea(Serial1, &rtc_clock);
EnvironmentSensorManager sensors(nmea);
bool g_relay_mode = false;

bool radio_init() {
  // Check for relay mode button hold
  pinMode(2, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Sample button for 3 seconds
  bool relay_mode = true;
  for (int i = 0; i < 30; i++) {
    if (digitalRead(2) == HIGH) {
      relay_mode = false;
      break;
    }
    digitalWrite(LED_BUILTIN, (i % 2 == 0) ? HIGH : LOW);  // blink while checking
    delay(100);
  }
  
  if (relay_mode) {
    digitalWrite(LED_BUILTIN, HIGH);  // solid LED = relay mode
  } else {
    digitalWrite(LED_BUILTIN, LOW);   // off = companion mode
  }

  // store result globally
  g_relay_mode = relay_mode;

  Serial1.setRX(PIN_GPS_RX);
  Serial1.setTX(PIN_GPS_TX);
  Serial1.begin(9600);
  rtc_clock.begin(Wire);
  SPI.begin();
  bool result = radio.std_init(NULL);
  
  if (!LittleFS.begin()) {
      LittleFS.format();
      LittleFS.begin();
  }

  return result;
}

uint32_t radio_get_rng_seed() {
  return radio.random(0x7FFFFFFF);
}

void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr) {
  radio.setFrequency(freq);
  radio.setSpreadingFactor(sf);
  radio.setBandwidth(bw);
  radio.setCodingRate(cr);
}

void radio_set_tx_power(int8_t dbm) {
  radio.setOutputPower(dbm);
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);
}