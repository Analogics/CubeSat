/*-------------- CODIGO DEL MAESTRO ------------
Creado por: @multex ----------------------------
Revisado por: @A.N.A -------------------------*/

#include <Arduino.h>
#include <Wire.h>

// Direcciones I2C
// estamos asignando una direccion a cada una de las dos conexiones de I2C que usaremos

#define I2C_ADDR_SLAVE_A 0x55 // ESP32-2 como esclavo
#define I2C_ADDR_SLAVE_B 0x66 // ESP32-1 como esclavo

// Pines para el Master (comunicación hacia ESP32-1)
/*- En este caso estamos asignando los pines 25 y 26 como los maestros de esa comunicacion*/

#define SDA_MASTER 25
#define SCL_MASTER 26

// Pines para el Slave (comunicación desde ESP32-1)
/*- En este caso estamos asignando los pines 21 y 22 como los esclavos de esa comunicacion*/
#define SDA_SLAVE 21
#define SCL_SLAVE 22


// Asignamos el objeto TwoWire para ver cual sera maestro y esclavo.
// 0 es maestro - 1 es esclavo
TwoWire I2C_bus_A = TwoWire(0); // Para maestro (Wire)
TwoWire I2C_bus_B = TwoWire(1); // Para esclavo

uint32_t msgCountA = 0;
uint32_t msgCountB = 0;

// === Funciones para esclavo (Wire1) ===
// ===== SE EJECUTA CUANDO! ============
/*que hacer cuando el maestro desde otro MCU nos DA datos .....*/

void onReceiveB(int len) {

   //crea una variable para almacenar lo que esta recibiendo.....
  String received = "";
  
   //crea una variable para almacenar lo que esta recibiendo.....
  while (I2C_bus_B.available()) {
    char c = I2C_bus_B.read();
    received += c;
  }

   //crea una variable para almacenar lo que esta recibiendo.....
  Serial.println("[Slave ESP32-1] Recibido desde ESP32-2: " + received);
}


// ==== SE EJECUTA CUANDO! ==============
/*que hacer cuando el maestro desde otro MCU nos PIDE datos .....*/

void onRequestB() {

  char response[32];

/* 
    snprintf(destino, tamaño_max, "formato", valores);
    
    snprintf es una función de C que crea una cadena de texto formateada 
    y la guarda en un arreglo de caracteres
    
    *Destino → variable tipo char[] donde guardarás la cadena.
    *tamaño_max → límite máximo de caracteres para no desbordar el arreglo.
    *"formato" → cadena con marcadores como %lu, %d, %s, etc.
    *valores → variables que quieres insertar en los marcadores.*/

  snprintf(response, sizeof(response), "%lu de ESP32-1", msgCountB++);
  
  // parte mas importante, esta devuelve un dato al maestro con .write
  I2C_bus_B.write(response);

  //imprimimos los datos en el serial
  Serial.print("[Slave ESP32-1] Respondido: ");
  Serial.println(response);
  Serial.println("============================================");
}

void setup() {

  
  //configuramos la velocidad de transmision.
  Serial.begin(115200);

  
  //dejamos pasar un segundo para asegurar la carga.
  delay(1000);

  
 //imprimimos algo en el serial para poder saber que iniciamos.
  Serial.println("=== ESP32-1 v.2 MASTER ===");

  // Iniciar como maestro
  /*TwoWwire(0-1) crea el objeto y hacemos que inicien con .begin()
  pero dentro debemos de poner los pines de esta comunicacion. ... 
  ================================================================
  En este caso es el maestro y esta usando los pines 25 y 26*/

  I2C_bus_A.begin(SDA_MASTER, SCL_MASTER);
  Serial.println("I2C Maestro (A) iniciado en SDA 25, SCL 26");


  // Iniciar como esclavo
  // Iniciar como esclavo
  /* Para el ESP32, cuando actúa como esclavo, primero necesitas fijar 
  los pines antes de llamar a .begin().*/
  I2C_bus_B.setPins(SDA_SLAVE, SCL_SLAVE);

  /*========================================================
  .begin(direccion) devuelve true "True" la inicialización fue exitosa.
  Esa respuesta se guarda en la variable booleana:*/

  bool success = I2C_bus_B.begin(I2C_ADDR_SLAVE_B);

  if (success) { // si se inicio el esclavo

    //mandamos un mensaje simple en el serial para guiarnos.
    Serial.println("I2C Esclavo (B) iniciado en SDA 21, SCL 22");
    
    /*Se configuran las funciones de callback:
    onReceiveB(): qué hacer cuando recibe datos.
    onRequestB(): qué responder cuando el maestro le pide datos.*/

    I2C_bus_B.onReceive(onReceiveB); //qué hacer cuando recibe datos
    I2C_bus_B.onRequest(onRequestB); //qué responder cuando el maestro le pide datos.
  } else {

     //que pasa si no se inicia el esclavo.
    Serial.println("Error iniciando I2C esclavo (B)");

  }
}

void loop() {
  delay(5000); // Cada 5 segundos

  // Enviar mensaje al esclavo ESP32-2
  String message = "Hola desde ESP32-1";

  /*En este caso, I2C_bus_A es maestro asi que puede iniciar una comunicacion,
  para eso unsa .beginTransmission(Direccion del esclavo.) ..................
  Hay que recordar que la direccion del esclavo debe ser la misma que se configuro
  en el otro MCU ................................................................*/
  I2C_bus_A.beginTransmission(I2C_ADDR_SLAVE_A);
  I2C_bus_A.print(message); // envia un dato 
  I2C_bus_A.endTransmission();

  //Imprimimos en el serial que mandamos un mensaje al esclavo y concatenamos el mensaje.
  Serial.println("[Master -> Slave ESP32-2] Enviado: " + message);

  /*Aqui seguimos actuando como maestro, asi que lo que hacemos es pedirle un dato al esclavo,
  luego de que le enviamos un dato .............. 
  Escribimos la funcion .requestFrom(direccion del esclavo, numero de bits como maximo que aceptara)*/
  I2C_bus_A.requestFrom(I2C_ADDR_SLAVE_A, 32);

  //Creamos una variable para guardar el dato que recibimos....
  String response = "";

  while (I2C_bus_A.available()) { //mientras el bus siga recibiendo datos
    response += (char)I2C_bus_A.read(); //añade esto a la cadena
  }

  /*Cuando termine el while habra terminado de enviar datos, asi que agrega todo
  a un mensaje por el serial....................................................*/
  Serial.println("[Master <- Slave ESP32-2] Respuesta recibida: " + response);
  Serial.println("--------------------------------------------");
}
