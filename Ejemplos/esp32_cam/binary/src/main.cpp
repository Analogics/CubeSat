#include "Arduino.h"
#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include <EEPROM.h>

// Tamaño de la EEPROM para almacenar el número de la última foto guardada
#define EEPROM_SIZE 1
// Variable global para llevar el conteo de las fotos tomadas
int pictureNumber = 0;

// Definición de los pines utilizados por el módulo de cámara AI-Thinker ESP32-CAM
#define PWDN_GPIO_NUM     32  // Pin de power down de la cámara
#define RESET_GPIO_NUM    -1  // Pin de reset de la cámara (no usado)
#define XCLK_GPIO_NUM      0  // Pin de reloj externo para la cámara
#define SIOD_GPIO_NUM     26  // Pin de datos I2C (SDA) para configuración de la cámara
#define SIOC_GPIO_NUM     27  // Pin de reloj I2C (SCL) para configuración de la cámara
#define Y9_GPIO_NUM       35  // Pines de datos de imagen (D7-D0)
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25  // Pin de sincronización vertical
#define HREF_GPIO_NUM     23  // Pin de referencia de línea
#define PCLK_GPIO_NUM     22  // Pin de reloj de píxel

void setup() {
  // Desactiva el detector de brownout para evitar reinicios por caídas de voltaje
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Inicializa la comunicación serie para depuración
  Serial.begin(115200);
  // Espera un poco para que se establezca la conexión serial, si es necesario.
  // Si no abres el monitor serial inmediatamente, puedes comentar la siguiente línea.
  // while (!Serial) { delay(10); }


  // Configura los parámetros de la cámara (pines, formato, calidad, etc.)
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; // Asegúrate de que sea JPEG para un tamaño manejable

  // Selecciona la resolución, calidad y número de framebuffers según si hay PSRAM
  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA;      // Resolución SVGA (800x600)
    config.jpeg_quality = 10;                // Mejor calidad JPEG (menor valor = mejor calidad)
    config.fb_count = 2;                     // Dos framebuffers para mejor rendimiento
  } else {
    config.frame_size = FRAMESIZE_SVGA;      // O considera una resolución menor si no hay PSRAM y tienes problemas
    config.jpeg_quality = 12;                // Calidad ligeramente menor si no hay PSRAM
    config.fb_count = 1;                     // Solo un framebuffer
  }

  // Inicializa la cámara con la configuración anterior y verifica errores
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000); // Pausa para leer el error
    ESP.restart(); // Reinicia si falla la cámara
    return; 
  }

  // Ajusta parámetros del sensor de la cámara para mejorar la imagen (opcional, ajusta según necesidad)
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 0);     // Brillo (-2 a 2)
    s->set_contrast(s, 0);       // Contraste (-2 a 2)
    s->set_saturation(s, 0);     // Saturación (-2 a 2)
    // ... (puedes añadir más configuraciones del sensor si lo necesitas o mantener las que tenías)
    s->set_quality(s, 10); // Calidad JPEG (0-63) menor es mejor pero más grande
    s->set_framesize(s, FRAMESIZE_SVGA);
  }


  // Inicializa la tarjeta SD usando el bus SD_MMC y verifica si está montada correctamente
  Serial.println("Initializing SD card...");
  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed"); 
    // Decide si quieres continuar sin SD o detenerte. Para este ejemplo, continuaremos
    // pero no guardaremos en SD si falla. Podrías optar por reiniciar o entrar en un bucle.
  } else {
    uint8_t cardType = SD_MMC.cardType();
    if(cardType == CARD_NONE){
      Serial.println("No SD Card attached");
    } else {
      Serial.println("SD Card initialized.");
    }
  }
  
  // Captura una imagen y obtiene el framebuffer (memoria temporal de la imagen)
  Serial.println("Taking picture...");
  camera_fb_t *fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb); // Aunque sea null, es buena práctica por si acaso
    delay(1000);
    ESP.restart();
    return;
  }
  Serial.printf("Picture taken! Size: %u bytes\n", fb->len);

  // Si la tarjeta SD está disponible, guarda la imagen
  if(SD_MMC.cardType() != CARD_NONE) {
    // Lee el número de foto anterior de la EEPROM, lo incrementa y genera el nombre del archivo
    EEPROM.begin(EEPROM_SIZE);
    pictureNumber = EEPROM.read(0) + 1;
    // Si pictureNumber excede 255 (máximo para un byte), se reiniciará a 1.
    // Puedes manejar esto de forma diferente si necesitas más de 255 fotos antes de sobreescribir la EEPROM.
    if (pictureNumber == 0) pictureNumber = 1; // Evitar picture0 si EEPROM estaba en 255
    
    String path = "/picture" + String(pictureNumber) + ".jpg";
    
    Serial.printf("Saving file to: %s\n", path.c_str());
    File file = SD_MMC.open(path.c_str(), FILE_WRITE);
    if(!file){
      Serial.println("Failed to open file in writing mode");
    } else {
      file.write(fb->buf, fb->len); // Escribe los datos de la imagen en el archivo
      Serial.printf("Saved file to path: %s\n", path.c_str());
      EEPROM.write(0, pictureNumber); // Actualiza el número de foto en la EEPROM
      EEPROM.commit();                // Guarda los cambios en la EEPROM
    }
    file.close(); // Cierra el archivo
    EEPROM.end(); // Libera la EEPROM
  } else {
    Serial.println("SD Card not available. Skipping save to SD.");
  }

  // Envía la imagen capturada por el puerto serie en formato BINARIO
  Serial.println("===IMAGE_START_BINARY==="); // Delimitador para el inicio de la imagen
  Serial.write(fb->buf, fb->len);             // Envía los bytes de la imagen directamente
  Serial.println("===IMAGE_END_BINARY===");   // Delimitador para el final de la imagen (se imprimirá después de los datos binarios)
                                             // Es importante notar que este println final podría no ser visible inmediatamente
                                             // en algunos terminales si no envían un newline después de los datos binarios.
                                             // El receptor debe saber que espera fb->len bytes después de IMAGE_START_BINARY.
  
  // Libera el framebuffer para que la cámara pueda capturar otra imagen en el futuro
  esp_camera_fb_return(fb);

  // Configura el pin GPIO4 en bajo y lo mantiene durante el deep sleep (si lo vas a usar)
  // Si no usas deep sleep y quieres que se reinicie o haga otra cosa, ajusta aquí.
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  rtc_gpio_hold_en(GPIO_NUM_4); 

  Serial.println("Image sent. Going to sleep now.");
  Serial.flush(); // Asegura que todos los datos seriales se envíen antes de dormir
  delay(200); // Pequeña pausa para asegurar que el mensaje se envíe

  esp_deep_sleep_start(); // El ESP32 entra en modo de bajo consumo
}

// La función loop está vacía porque todo el proceso se realiza una sola vez en setup()
void loop() {
  // El código no debería llegar aquí si esp_deep_sleep_start() se ejecuta en setup()
  delay(10000); 
}