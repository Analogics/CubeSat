#include <Arduino.h>
#include <Wire.h>

#define I2C_ADDR_SLAVE_A 0x55  // Dirección del esclavo (ESP32-2)
#define SDA_MASTER 25
#define SCL_MASTER 26

TwoWire I2C_bus_A = TwoWire(0);  // Bus maestro

void setup() {
    Serial.begin(115200);
    I2C_bus_A.begin(SDA_MASTER, SCL_MASTER);  // Inicia como maestro
    Serial.println("Maestro listo");
}

void loop() {
    delay(3000);  // Solicita datos cada 3 segundos
    
    // Solicita 2 bytes al esclavo
    I2C_bus_A.requestFrom(I2C_ADDR_SLAVE_A, 2);
    
    if (I2C_bus_A.available() == 2) {
        uint8_t highByte = I2C_bus_A.read();
        uint8_t lowByte = I2C_bus_A.read();
        int adcValue = (highByte << 8) | lowByte;  // Reconstruye el valor
        
        Serial.print("[Master] ADC recibido: ");
        Serial.println(adcValue);
    } else {
        Serial.println("[Master] Error en la lectura");
    }
}
