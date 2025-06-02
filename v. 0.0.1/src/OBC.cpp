/*CODIGO DEL OBC v 0.0.2-------------------
    Datos y estructura: @A.N.A -----------*/

#include <Arduino.h>
#include <Wire.h>

#define I2C_ADDR_SLAVE_A 0x55 // Dirección del esclavo (EPS)
#define I2C_ADDR_SLAVE_B 0x66 // Direccion del ADCS

// Configuración de pines I2C
#define SDA_MASTER 21
#define SCL_MASTER 22

#define BLINK 2

TwoWire I2C_bus_A = TwoWire(0); // Bus maestro

// Recibe el dato que escribimos en el Serial y lo mandamos por i2c.
void enviarDatosPorI2c(int dato)
{

    if (dato == 0xA2)
    {

        // ordenes para el ADCS
        I2C_bus_A.beginTransmission(I2C_ADDR_SLAVE_B);
        I2C_bus_A.write(dato);
        I2C_bus_A.endTransmission();

        /*
        I2C_bus_A.requestFrom(I2C_ADDR_SLAVE_B, 1);
        if (I2C_bus_A.available())
        {
            int response = I2C_bus_A.read();
            Serial.print("Enviamos el dato .. recibimos  : ");
            Serial.println(response);
        }
            */

        // Codigo para esperar los datos del giroscpio.
        I2C_bus_A.requestFrom(I2C_ADDR_SLAVE_B, 12);

        if (I2C_bus_A.available() == 12)
        {
            byte buffer[12];
            for (int i = 0; i < 12; i++)
            {
                buffer[i] = I2C_bus_A.read();
            }

            // Reconstruir como int16_t
            int16_t ax = (buffer[0] << 8) | buffer[1];
            int16_t ay = (buffer[2] << 8) | buffer[3];
            int16_t az = (buffer[4] << 8) | buffer[5];
            int16_t gx = (buffer[6] << 8) | buffer[7];
            int16_t gy = (buffer[8] << 8) | buffer[9];
            int16_t gz = (buffer[10] << 8) | buffer[11];

            Serial.print("Datos: ");
            Serial.print(ax);
            Serial.print(", ");
            Serial.print(ay);
            Serial.print(", ");
            Serial.print(az);
            Serial.print(", ");
            Serial.print(gx);
            Serial.print(", ");
            Serial.print(gy);
            Serial.print(", ");
            Serial.println(gz);
        }
    }
    if (dato != 0xA2)
    {

        // en caso no se quiere comunicar con el ADCS, sera directo con el EPS

        I2C_bus_A.beginTransmission(I2C_ADDR_SLAVE_A);
        I2C_bus_A.write(dato);
        I2C_bus_A.endTransmission();

        // Solicitar respuesta al esclavo.

        if (dato == 0xA1)
        {
            I2C_bus_A.requestFrom(I2C_ADDR_SLAVE_A, 4); // Solicita 4 bytes al esclavo

            if (I2C_bus_A.available() == 4)
            {
                // Reconstruye los valores crudos
                int pot1 = (I2C_bus_A.read() << 8) | I2C_bus_A.read();
                int pot2 = (I2C_bus_A.read() << 8) | I2C_bus_A.read();

                // Conversión a voltaje
                float voltaje1 = (pot1 * 3.3) / 4095.0;
                float voltaje2 = (pot2 * 3.3) / 4095.0;

                Serial.printf("Corriente: %.2fV | Voltaje: %.2fV\n", voltaje1, voltaje2);
            }
        }
    }

    /*
        I2C_bus_A.requestFrom(I2C_ADDR_SLAVE_A, 1);
        if (I2C_bus_A.available())
        {
            int response = I2C_bus_A.read();
            Serial.print("Enviamos el dato .. recibimos  : ");
            Serial.println(response);
        }
            */
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    pinMode(BLINK, OUTPUT);

    Serial.println("---------------------------------------------------------------");
    Serial.println("BIENVENIDO AL SISTEMA DE PRUEBAS v 0.0.2");
    Serial.println("------------------------------------------------------------------");
    Serial.println("Por favor introduzca orden al CubeSat");

    // Iniciar bus maestro
    I2C_bus_A.begin(SDA_MASTER, SCL_MASTER);
    Serial.println("Maestro iniciado en SDA 21, SCL 22");
}

void loop()
{

    // Verificando que tipo de mensajes enviaremos al sistema

    if (Serial.available() > 0)
    { // verificar que el serial sea viable.

        int message = Serial.read(); // Leer el byte

        if (message == 0xA1)
        { // Si es el carácter 0xA1

            digitalWrite(BLINK, HIGH); // Enciende el LED

            // Enviar dato al otro esclavo (ESP32-2)
            enviarDatosPorI2c(message);

            Serial.print("Estamos pidiendo datos EPS (Corriente y voltaje)");
        }

        if (message == 0xA2)
        { // Si es el carácter 0xA1

            // Enviar dato al otro esclavo (ESP32-2)
            enviarDatosPorI2c(message);

            Serial.print("Estamos pidiendo datos al Giroscopio del ADCS");
        }

        if (message == 0xB1)
        { // Si es el carácter 0xA1

            // Enviar dato al otro esclavo (ESP32-2)
            enviarDatosPorI2c(message);

            Serial.print("Reset al ADC");
        }

        if (message == 0xB2)
        { // Si es el carácter 0xA1

            // Enviar dato al otro esclavo (ESP32-2)
            enviarDatosPorI2c(message);

            Serial.print("Reset al OBC");
        }

        if (message == 0xB3)
        { // Si es el carácter 0xA1

            // Enviar dato al otro esclavo (ESP32-2)
            enviarDatosPorI2c(message);

            Serial.print("Reset al COM");
        }

        if (message == 0xC1)
        { // Si es el carácter 0xA1

            // Enviar dato al otro esclavo (ESP32-2)
            enviarDatosPorI2c(message);

            Serial.print("Modo Seguro Activado");
        }

        if (message == 0xC2)
        { // Si es el carácter 0xA1

            // Enviar dato al otro esclavo (ESP32-2)
            enviarDatosPorI2c(message);

            Serial.print("Modo Seguro Desactivado");
        }

        if (message == 0xD1)
        { // Si es el carácter 0xA1

            // Enviar dato al otro esclavo (ESP32-2)
            enviarDatosPorI2c(message);

            Serial.print("PlayLoad Encendido");
        }

        if (message == 0xD2)
        { // Si es el carácter 0xA1

            // Enviar dato al otro esclavo (ESP32-2)
            enviarDatosPorI2c(message);

            Serial.print("PlayLoad Apgado");
        }
    }

    /*Estrucutra..............*/
}
