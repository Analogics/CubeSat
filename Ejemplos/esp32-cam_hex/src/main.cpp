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
  config.pixel_format = PIXFORMAT_JPEG;

  // Selecciona la resolución, calidad y número de framebuffers según si hay PSRAM
  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA;      // Resolución SVGA (800x600)
    config.jpeg_quality = 10;                // Mejor calidad JPEG (menor valor = mejor calidad)
    config.fb_count = 2;                     // Dos framebuffers para mejor rendimiento
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;                // Calidad ligeramente menor si no hay PSRAM
    config.fb_count = 1;                     // Solo un framebuffer
  }

  // Inicializa la cámara con la configuración anterior y verifica errores
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return; // Si falla, termina el setup
  }

  // Ajusta parámetros del sensor de la cámara para mejorar la imagen
  sensor_t *s = esp_camera_sensor_get();
  s->set_brightness(s, 0);     // Brillo (-2 a 2)
  s->set_contrast(s, 0);       // Contraste (-2 a 2)
  s->set_saturation(s, 0);     // Saturación (-2 a 2)
  s->set_sharpness(s, 0);      // Enfoque (-2 a 2)
  s->set_denoise(s, 0);        // Reducción de ruido (0 = desactivado)
  s->set_whitebal(s, 1);       // Balance de blancos (0 = desactivado)
  s->set_awb_gain(s, 1);       // Ganancia WB automática (0 = desactivado)
  s->set_wb_mode(s, 0);        // Modo WB (0 = automático)
  s->set_exposure_ctrl(s, 1);  // Control de exposición (0 = desactivado)
  s->set_aec2(s, 0);           // AEC2 (0 = desactivado)
  s->set_ae_level(s, 0);       // Nivel de exposición (-2 a 2)
  s->set_aec_value(s, 300);    // Valor de exposición (0-1200)
  s->set_gain_ctrl(s, 1);      // Control de ganancia (0 = desactivado)
  s->set_agc_gain(s, 1);       // Ganancia AGC (0 = automático)
  s->set_gainceiling(s, (gainceiling_t)0);  // Límite de ganancia (0-6)
  s->set_bpc(s, 0);            // Corrección de píxeles negros (0 = desactivado)
  s->set_wpc(s, 1);            // Corrección de píxeles blancos (0 = desactivado)
  s->set_raw_gma(s, 1);        // Gamma RAW (0 = desactivado)
  s->set_lenc(s, 1);           // Corrección de lente (0 = desactivado)
  s->set_hmirror(s, 0);        // Espejo horizontal (0 = desactivado)
  s->set_vflip(s, 0);          // Volteo vertical (0 = desactivado)
  s->set_dcw(s, 1);            // Downscale (0 = desactivado)

  // Inicializa la tarjeta SD usando el bus SD_MMC y verifica si está montada correctamente
  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed"); // Error si no se puede montar la SD
    return;
  }
  
  // Verifica que la tarjeta SD esté presente y sea reconocida
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached"); // Error si no hay tarjeta SD
    return;
  }

  // Captura una imagen y obtiene el framebuffer (memoria temporal de la imagen)
  camera_fb_t *fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed"); // Error si no se pudo capturar imagen
    return;
  }

  // Lee el número de foto anterior de la EEPROM, lo incrementa y genera el nombre del archivo
  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0) + 1;
  String path = "/picture" + String(pictureNumber) + ".jpg";
  
  // Abre un archivo en la SD para guardar la imagen capturada
  File file = SD_MMC.open(path.c_str(), FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file in writing mode"); // Error al abrir archivo
  } else {
    file.write(fb->buf, fb->len); // Escribe los datos de la imagen en el archivo
    Serial.printf("Saved file to path: %s\n", path.c_str());
    EEPROM.write(0, pictureNumber); // Actualiza el número de foto en la EEPROM
    EEPROM.commit();                // Guarda los cambios en la EEPROM
  }
  file.close(); // Cierra el archivo

  // Envía la imagen capturada por el puerto serie en formato hexadecimal
  Serial.println("===IMAGE_START===");
  for (size_t i = 0; i < fb->len; i++) {
    Serial.printf("%02x", fb->buf[i]); // Imprime cada byte en hexadecimal
  }
  Serial.println("\n===IMAGE_END===");

  // Libera el framebuffer para que la cámara pueda capturar otra imagen en el futuro
  esp_camera_fb_return(fb);

  // Configura el pin GPIO4 en bajo y lo mantiene durante el deep sleep
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  rtc_gpio_hold_en(GPIO_NUM_4); // Mantiene el estado del pin durante el sueño profundo

  // Mensaje de depuración y entra en modo deep sleep para ahorrar energía
  Serial.println("Going to sleep now");
  delay(100); // Pequeña pausa para asegurar que el mensaje se envíe
  esp_deep_sleep_start(); // El ESP32 entra en modo de bajo consumo hasta que sea reiniciado o despierte por una señal externa
}

// La función loop está vacía porque todo el proceso se realiza una sola vez en setup()
void loop() {}