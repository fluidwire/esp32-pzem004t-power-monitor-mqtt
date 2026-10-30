/*
 * ESP32 PZEM-004T v3 AC Power Monitor (MQTT)
 * ------------------------------------------
 * Reads voltage, current, power, energy (kWh), frequency and power factor
 * from a PZEM-004T v3 module over UART (Modbus-RTU under the hood) and
 * publishes the values as JSON to an MQTT broker.
 *
 * Useful for household electricity-bill studies: the module accumulates
 * kWh in its own non-volatile memory, so energy survives ESP32 reboots.
 *
 * !! MAINS WARNING !!  The PZEM-004T measures AC mains voltage. Wire the
 * AC side only with the power OFF, inside an insulated enclosure, with a
 * fuse/breaker upstream, and only if you are qualified to do so.
 *
 * Required libraries (install via Library Manager):
 *   - "PZEM004Tv30" by Jakub Mandula
 *   - "PubSubClient" by Nick O'Leary
 *
 * Wiring (the module's TTL side is 5V, so power it from 5V / VIN):
 *   PZEM 5V -> ESP32 VIN (5V)
 *   PZEM GND -> ESP32 GND
 *   PZEM TX -> ESP32 GPIO16 (RX2)
 *   PZEM RX -> ESP32 GPIO17 (TX2)
 *   (The PZEM TX line is 3.3V-safe for the ESP32 input on most boards.)
 *
 * Setup:
 *   1. Copy config.h.example to config.h
 *   2. Fill in WiFi + MQTT broker details.
 *   3. Flash. Open Serial Monitor at 115200.
 *
 * Sample MQTT payload:
 *   {"voltage_v":229.8,"current_a":0.412,"power_w":86.3,"energy_kwh":12.345,
 *    "freq_hz":60.0,"pf":0.91}
 *
 * (c) 2026 Fluidwire. Released under the MIT License.
 * https://github.com/fluidwire
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <PZEM004Tv30.h>
#include "config.h"

const int      PZEM_RX_PIN         = 16;   // ESP32 RX2  <- PZEM TX
const int      PZEM_TX_PIN         = 17;   // ESP32 TX2  -> PZEM RX
const uint32_t PUBLISH_INTERVAL_MS = 5000UL;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
PZEM004Tv30 pzem(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);

void connectWiFi() {
  Serial.print(F("[wifi] connecting to "));
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  Serial.print(F("[wifi] IP: "));
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  while (!mqtt.connected()) {
    Serial.print(F("[mqtt] connecting... "));
    bool ok = strlen(MQTT_USER) > 0
      ? mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)
      : mqtt.connect(MQTT_CLIENT_ID);
    if (ok) {
      Serial.println(F("ok"));
    } else {
      Serial.print(F("failed state="));
      Serial.println(mqtt.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("[pzem] PZEM-004T v3 on Serial2"));
  connectWiFi();
}

void loop() {
  static uint32_t lastPublish = 0;

  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  uint32_t now = millis();
  if (now - lastPublish < PUBLISH_INTERVAL_MS && lastPublish != 0) {
    delay(20);
    return;
  }
  lastPublish = now;

  // The library returns NAN when the module does not answer (wiring fault,
  // no AC on the sensing side, wrong baud rate).
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power   = pzem.power();
  float energy  = pzem.energy();     // kWh, kept in the module's memory
  float freq    = pzem.frequency();
  float pf      = pzem.pf();

  char buf[200];
  if (isnan(voltage) || isnan(current) || isnan(power) || isnan(energy)) {
    snprintf(buf, sizeof(buf), "{\"error\":\"pzem_no_response\",\"uptime_s\":%lu}",
             (unsigned long)(now / 1000UL));
    Serial.println(F("[pzem] no response -- check wiring and AC supply"));
    mqtt.publish(MQTT_TOPIC, buf);
    return;
  }

  snprintf(buf, sizeof(buf),
           "{\"voltage_v\":%.1f,\"current_a\":%.3f,\"power_w\":%.1f,"
           "\"energy_kwh\":%.3f,\"freq_hz\":%.1f,\"pf\":%.2f}",
           voltage, current, power, energy,
           isnan(freq) ? 0.0f : freq, isnan(pf) ? 0.0f : pf);
  Serial.print(F("[mqtt] "));
  Serial.println(buf);
  mqtt.publish(MQTT_TOPIC, buf);
}
