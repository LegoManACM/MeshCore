#include "target.h"

#include <Arduino.h>
#include <helpers/ArduinoHelpers.h>

WaveshareBoard board;

RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_0, P_LORA_RST, P_LORA_DIO_1, SPI);
WRAPPER_CLASS radio_driver(radio, board);

VolatileRTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
SensorManager sensors;

bool radio_init() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("Starting up...");

  rtc_clock.begin(Wire);
  Serial.println("RTC done");

  pinMode(P_LORA_RST, OUTPUT);
  digitalWrite(P_LORA_RST, LOW);
  delay(10);
  digitalWrite(P_LORA_RST, HIGH);
  delay(10);
  Serial.println("Reset done");

  SPI.begin();
  Serial.println("SPI done");

  bool result = radio.std_init(NULL);
  Serial.println(result ? "Radio OK" : "Radio FAILED");
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