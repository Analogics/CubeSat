#include <Arduino.h>
#include <Wire.h>

// Configuración ADC
const int adcPin = 35;  // Pin del potenciómetro (ADC1_CH7)

// Direcciones y pines I2C
#define I2C_ADDR_SLAVE_A 0x55  // Dirección de este esclavo (ESP32-2)
#define SDA_MASTER 21
#define SCL_MASTER 22
#define SDA_SLAVE 25
#define SCL_SLAVE 26

TwoWire I2C_bus_A = TwoWire(0);  // Bus maestro
TwoWire I2C_bus_B = TwoWire(1);  // Bus esclavo

// Función para manejar solicitudes del maestro
void onRequestB() {
    int adcValue = analogRead(adcPin);  // Lectura del ADC (0-4095)
    
    // Envía el valor como 2 bytes (para cubrir el rango de 12 bits)
    I2C_bus_B.write((adcValue >> 8) & 0xFF);  // Byte alto
    I2C_bus_B.write(adcValue & 0xFF);         // Byte bajo
    
    Serial.print("[Slave] ADC enviado: ");
    Serial.println(adcValue);
}

void setup() {
    Serial.begin(115200);
    pinMode(adcPin, INPUT);  // Configura el pin ADC como entrada
    
    // Inicia el bus maestro
    I2C_bus_A.begin(SDA_MASTER, SCL_MASTER);
    
    // Inicia el bus esclavo
    I2C_bus_B.setPins(SDA_SLAVE, SCL_SLAVE);
    if (I2C_bus_B.begin(I2C_ADDR_SLAVE_A)) {
        I2C_bus_B.onRequest(onRequestB);  // Registra el callback
        Serial.println("Esclavo listo en 0x55");
    }
}

void loop() {
    // El ADC se lee solo cuando el maestro lo solicita
    delay(100);  // Reduce carga de la CPU
}
