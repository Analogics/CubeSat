#include <Arduino.h>
#include <Wire.h>

#define I2C_ADDR_SLAVE_A 0x55  // Dirección del esclavo (ESP32-2)
#define I2C_ADDR_SLAVE_B 0x66  // Dirección de este esclavo (ESP32-1)

// Configuración de pines I2C
#define SDA_MASTER 25
#define SCL_MASTER 26
#define SDA_SLAVE 21
#define SCL_SLAVE 22

TwoWire I2C_bus_A = TwoWire(0);  // Bus maestro
TwoWire I2C_bus_B = TwoWire(1);  // Bus esclavo

// Función para recibir datos como esclavo
void onReceiveB(int len) {
    int received = I2C_bus_B.read();
    Serial.print("[Slave ESP32-1] Recibido: ");
    Serial.println(received);
}

// Función para enviar datos como esclavo
void onRequestB() {
    int response = 10;
    I2C_bus_B.write(response);
    Serial.print("[Slave ESP32-1] Respondido: ");
    Serial.println(response);
    Serial.println("============================================");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== ESP32-1 (Maestro) ===");

    // Iniciar bus maestro
    I2C_bus_A.begin(SDA_MASTER, SCL_MASTER);
    Serial.println("Maestro iniciado en SDA 25, SCL 26");

    // Iniciar bus esclavo
    I2C_bus_B.setPins(SDA_SLAVE, SCL_SLAVE);
    bool success = I2C_bus_B.begin(I2C_ADDR_SLAVE_B);

    if (success) {
        Serial.println("Esclavo iniciado en SDA 21, SCL 22");
        I2C_bus_B.onReceive(onReceiveB);
        I2C_bus_B.onRequest(onRequestB);
    } else {
        Serial.println("Error iniciando esclavo");
    }
}

void loop() {
    delay(5000);
    
    // Enviar dato al otro esclavo (ESP32-2)
    int message = 20;
    I2C_bus_A.beginTransmission(I2C_ADDR_SLAVE_A);
    I2C_bus_A.write(message);
    I2C_bus_A.endTransmission();
    
    Serial.print("[Master->Slave ESP32-2] Enviado: ");
    Serial.println(message);

    // Solicitar respuesta
    I2C_bus_A.requestFrom(I2C_ADDR_SLAVE_A, 1);
    if (I2C_bus_A.available()) {
        int response = I2C_bus_A.read();
        Serial.print("[Master<-Slave ESP32-2] Respuesta: ");
        Serial.println(response);
    }
    Serial.println("--------------------------------------------");
}
