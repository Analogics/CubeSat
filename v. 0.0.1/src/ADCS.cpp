/*---------- Sistema de ADCS v 0.0.1
-------------Desarrollado por @A.N.A */

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

// Borra por si no funciona el envio de datos del giroscopio
int16_t ax = -10, ay = -20, az = 30;
int16_t gx = 40, gy = -50, gz = -60;
// fin de borrar por si algo no funciona.

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

/*- En este caso estamos asignando los pines 25 y 26 como los esclavos de esa comunicacion*/
#define SDA_SLAVE 25
#define SCL_SLAVE 26

#define BLINK 2

#define I2C_ADDR_SLAVE_A 0x66 // Dirección del esclavo ADCS

TwoWire I2C_bus_B = TwoWire(1); // Para esclavo

// Función para recibir datos como esclavo
void onReceiveB(int len)
{
  int received = I2C_bus_B.read();
  Serial.print("[Slave ESP32-2] Recibido: ");
  Serial.println(received);
}

// Función para enviar datos como esclavo
void onRequestB()
{

  digitalWrite(BLINK, HIGH);

  byte buffer[12];

  // Empaquetar como int16_t (enteros de 16 bits con signo)
  buffer[0] = ax >> 8;   // High byte ax
  buffer[1] = ax & 0xFF; // Low byte ax
  buffer[2] = ay >> 8;
  buffer[3] = ay & 0xFF;
  buffer[4] = az >> 8;
  buffer[5] = az & 0xFF;
  buffer[6] = gx >> 8;
  buffer[7] = gx & 0xFF;
  buffer[8] = gy >> 8;
  buffer[9] = gy & 0xFF;
  buffer[10] = gz >> 8;
  buffer[11] = gz & 0xFF;

  I2C_bus_B.write(buffer, 12);

  delay(1000);

  digitalWrite(BLINK, LOW);
  /*
  int response = 40;
  I2C_bus_B.write(response);
  Serial.print("[Slave ESP32-2] Respondido: ");
  Serial.println(response);
  Serial.println("============================================");
  */
}

void setup()
{

  Serial.begin(115200);
  delay(1000);
  Serial.println("ADCS INICIADO - DOS LABORES PARA MANEJAR UNA LCD");

  // initialize LCD
  lcd.init();
  // turn on LCD backlight
  lcd.backlight();

  pinMode(BLINK, OUTPUT);
  digitalWrite(BLINK, LOW);

  I2C_bus_B.setPins(SDA_SLAVE, SCL_SLAVE);
  bool success = I2C_bus_B.begin(I2C_ADDR_SLAVE_A);

  if (success)
  {
    Serial.println("Esclavo iniciado en SDA 25, SCL 26");
    I2C_bus_B.onReceive(onReceiveB);
    I2C_bus_B.onRequest(onRequestB);
  }
  else
  {
    Serial.println("Error iniciando esclavo");
  }
}

void loop()
{
  // set cursor to first column, first row
  lcd.setCursor(0, 0);
  // print static message
  lcd.print("GIROSCOPIO 2.0");

  // print scrolling message
  delay(500);
}
