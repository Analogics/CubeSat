/*Codigo de la primera version del EPS
  Diseño elaborado por: @A.N.A -------*/

#include <Arduino.h>
#include <Wire.h>

#define OBC 15
#define ADCS 4
#define COM 16
#define PLAYLOAD 17
#define TIME1 5
#define TIME2 18

// Pines de potenciómetros (entradas analógicas)
#define POT1_PIN 34 // Potenciometro de Corriente
#define POT2_PIN 35 // Potenciometro de Voltaje

//---------- cositas para el I2C ------------
#define I2C_ADDR_SLAVE_A 0x55 // Dirección del esclavo (ESP32-2)

#define SDA_SLAVE 21
#define SCL_SLAVE 22

#define BLINK 2

TwoWire I2C_bus_A = TwoWire(1); // Bus esclavo

int pot1Value;
int pot2Value;

void cambiosEstado(int orden)
{

  switch (orden)
  {
  case 1: // pedir datos al EPS
    digitalWrite(BLINK, HIGH);
    delay(1000);
    digitalWrite(BLINK, LOW);
    break;

  case 2: // Reset de ADCS
    digitalWrite(ADCS, LOW);
    delay(1000);
    digitalWrite(ADCS, HIGH);
    break;

  case 3: // Reset de OBC
    digitalWrite(OBC, LOW);
    delay(1000);
    digitalWrite(OBC, HIGH);
    break;

  case 4: // Reset de COM
    digitalWrite(COM, LOW);
    delay(1000);
    digitalWrite(COM, HIGH);
    break;

  case 5: // Activacion del modo seguro
    digitalWrite(ADCS, LOW);
    digitalWrite(PLAYLOAD, LOW);
    break;

  case 6: // Reset de ADCS
    digitalWrite(ADCS, LOW);
    digitalWrite(ADCS, HIGH);
    break;

  case 7: // Encender el PLayLoad
    digitalWrite(PLAYLOAD, HIGH);
    break;

  case 8: // Apagamos el PlayLoad
    digitalWrite(PLAYLOAD, LOW);
    break;
  }
}

void onReceiveB(int len)
{
  int datoEnviado = I2C_bus_A.read();
  delay(50);
  // Serial.print("[Slave ESP32-2] Recibido: ");
  // Serial.println(received);

  if (datoEnviado == 0xA1)
  {
    cambiosEstado(1);
  }

  if (datoEnviado == 0xB1)
  {
    cambiosEstado(2);
  }

  if (datoEnviado == 0xB2)
  {
    cambiosEstado(3);
  }

  if (datoEnviado == 0xB3)
  {
    cambiosEstado(4);
  }

  if (datoEnviado == 0xC1)
  {
    cambiosEstado(5);
  }

  if (datoEnviado == 0xC2)
  {
    cambiosEstado(6);
  }

  if (datoEnviado == 0xD1)
  {
    cambiosEstado(7);
  }

  if (datoEnviado == 0xD2)
  {
    cambiosEstado(8);
  }
}

void onRequestB()
{
  //int response = 30;

  byte buffer[4]; // creamos un buffer donde mandaremos los dos valores

  buffer[0] = highByte(pot1Value); // Parte alta del primer potenciómetro
  buffer[1] = lowByte(pot1Value);  // Parte baja

  buffer[2] = highByte(pot2Value); // Parte alta del segundo
  buffer[3] = lowByte(pot2Value);  // Parte baja

  I2C_bus_A.write(buffer, 4); // Envía 4 bytes (2 enteros de 16 bits)


  //I2C_bus_A.write(response);
}

//--------------- fin de  cositas para el I2C

// Configuración ADC
const int adcResolution = 12; // Resolución de 12 bits (0-4095)
unsigned long lastRead = 0;   // Última lectura de ADC
const int readInterval = 500; // Intervalo de lectura en ms

void leerPotenciometros()
{
  pot1Value = analogRead(POT1_PIN);
  pot2Value = analogRead(POT2_PIN);

  // Calcular voltaje (3.3V es el voltaje de referencia)
  float voltage1 = (pot1Value * 3.3) / 4095.0;
  float voltage2 = (pot2Value * 3.3) / 4095.0;

  // Imprimir valores por serial
  Serial.printf("POT1: %.2fV | POT2: %.2fV\n", voltage1, voltage2);
}

void setup()
{

  // Configurar comunicación serial
  Serial.begin(115200);

  // Estamos configurando los puntos de los Subsistemas como salidas.
  pinMode(OBC, OUTPUT);
  pinMode(ADCS, OUTPUT);
  pinMode(COM, OUTPUT);
  pinMode(PLAYLOAD, OUTPUT);
  pinMode(TIME1, OUTPUT);
  pinMode(TIME2, OUTPUT);
  pinMode(BLINK, OUTPUT);

  digitalWrite(PLAYLOAD, LOW);

  digitalWrite(OBC, HIGH);
  digitalWrite(ADCS, HIGH);
  digitalWrite(COM, HIGH);

  // Configurar resolución ADC
  analogReadResolution(adcResolution);

  //----------------- cosas para el i2c - parte 2
  // Iniciar bus esclavo
  I2C_bus_A.setPins(SDA_SLAVE, SCL_SLAVE);
  bool success = I2C_bus_A.begin(I2C_ADDR_SLAVE_A);

  if (success)
  {
    Serial.println("Esclavo iniciado en SDA 25, SCL 26");
    I2C_bus_A.onReceive(onReceiveB);
    I2C_bus_A.onRequest(onRequestB);
  }
  else
  {
    Serial.println("Error iniciando esclavo");
  }

  // -------------- final de cosas para el i2c parte 2
}

void loop()
{
  /*
  digitalWrite(OBC, HIGH);     // Encender el Sistema de OBC
  digitalWrite(ADCS, HIGH);    // Encender el Sistema de ADCS
  digitalWrite(COM, HIGH);     // Encender el Sistema de COM
  digitalWrite(PLAYLOAD, LOW); // Encender el Sistema de PLAYLOAD
  digitalWrite(TIME1, HIGH);   // Encender el Timer 1
  digitalWrite(TIME2, HIGH);   // Encender el Timer 2
*/

  // digitalWrite(BLINK, HIGH);  // Encender el Timer 2

  // Lectura periódica de potenciómetros
  if (millis() - lastRead >= readInterval)
  {
    leerPotenciometros();
    lastRead = millis();
  }
}
