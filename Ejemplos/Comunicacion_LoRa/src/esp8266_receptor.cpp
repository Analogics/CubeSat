#include <Arduino.h>

#include <SPI.h>
#include <LoRa.h>

#define SS 15   // D8
#define RST 16  // D0
#define DIO0 5  // D1

void setup() {
  Serial.begin(115200);
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Error al iniciar LoRa");
    while (1);
  }
  Serial.println("LoRa Receptor (ESP8266) listo");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.print("Mensaje recibido: ");
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }
    Serial.println();
  }
}
