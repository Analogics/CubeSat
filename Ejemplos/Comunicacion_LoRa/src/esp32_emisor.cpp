#include <Arduino.h>

#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define RST 14
#define DIO0 2

void setup() {
  Serial.begin(115200);
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Error al iniciar LoRa");
    while (1);
  }
  Serial.println("LoRa Emisor (ESP32) listo");
}

void loop() {
  Serial.println("Enviando mensaje...");
  LoRa.beginPacket();
  LoRa.print("¡Hola desde ESP32!");
  LoRa.endPacket();
  delay(2000);
}
