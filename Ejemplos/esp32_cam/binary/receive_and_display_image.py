import serial
import time
from PIL import Image # Pillow library for image display
import io # To handle bytes as a file-like object
import serial.tools.list_ports # To list available serial ports

# --- Configuración ---
SERIAL_PORT = None  # Deja como None para que el script pregunte, o pon tu puerto aquí.
                    # Ejemplos: 'COM3' (Windows), '/dev/ttyUSB0' (Linux)
BAUD_RATE = 115200
IMAGE_FILENAME = "received_image.jpg" # Nombre del archivo donde se guardará la imagen
SERIAL_TIMEOUT = 1  # Timeout en segundos para las lecturas individuales del puerto serie
CAPTURE_TIMEOUT = 20 # Timeout total en segundos para capturar una imagen completa

# Delimitadores (como bytes, deben coincidir con los del ESP32)
START_DELIMITER = b"===IMAGE_START_BINARY==="
END_DELIMITER = b"===IMAGE_END_BINARY==="
# Nota: Serial.println() en ESP32 añade \r\n al final de la cadena.
# readline() en Python lee hasta \n y lo incluye.

def list_serial_ports_and_select():
    """ Lista los puertos serie disponibles y permite al usuario seleccionar uno. """
    ports = serial.tools.list_ports.comports()
    available_ports_devices = []
    print("Puertos serie disponibles:")
    if not ports:
        print("No se encontraron puertos serie.")
        return None
    for i, port_info in enumerate(ports):
        print(f"  {i}: {port_info.device} - {port_info.description}")
        available_ports_devices.append(port_info.device)
    
    while True:
        try:
            choice = int(input("Ingresa el número del puerto serie a usar: "))
            if 0 <= choice < len(available_ports_devices):
                return available_ports_devices[choice]
            else:
                print("Opción inválida. Intenta de nuevo.")
        except ValueError:
            print("Entrada inválida. Por favor, ingresa un número.")
        except KeyboardInterrupt:
            print("\nSelección cancelada.")
            return None
    return None


def receive_image_data(ser):
    """
    Espera el delimitador de inicio, captura los datos binarios de la imagen
    hasta que se encuentra el delimitador de fin, o hasta que se agota el tiempo.
    """
    image_data_buffer = bytearray()
    
    print(f"Esperando el delimitador de inicio: {START_DELIMITER.decode()}...")
    # Leer líneas hasta encontrar el delimitador de inicio
    # El timeout del objeto 'ser' se aplica a cada readline()
    found_start = False
    try:
        while True: # Bucle para encontrar el delimitador de inicio
            line = ser.readline() # Lee hasta \n (incluyéndolo)
            if not line: # Timeout de readline, no se leyó nada
                print("Timeout esperando el delimitador de inicio (readline).")
                # Podrías añadir un contador aquí si quieres un timeout general para esta fase
                # o depender del timeout global de la conexión si el ESP no envía nada.
                # Por ahora, si no hay línea, es un timeout de la operación readline.
                # El script podría quedarse aquí si el ESP32 no envía el delimitador.
                # Considera añadir un timeout general para esta espera.
                # Un simple 'pass' aquí puede causar un bucle infinito si el ESP32 no envía nada.
                # Se recomienda que el ESP32 envíe algo periódicamente o que este bucle tenga un timeout.
                time.sleep(0.1) # Evita un bucle apretado si readline retorna vacío inmediatamente
                continue

            # print(f"DEBUG: Leído (buscando inicio): {line.strip()}") # Para depuración
            if START_DELIMITER in line:
                print("¡Delimitador de inicio encontrado!")
                found_start = True
                break # Salir del bucle de búsqueda de inicio
    except serial.SerialException as e:
        print(f"Error serial durante la búsqueda del delimitador de inicio: {e}")
        return None
    
    if not found_start:
        print("No se encontró el delimitador de inicio.")
        return None

    print("Capturando datos de la imagen...")
    # Ahora, los bytes de la imagen vienen directamente después de la línea del delimitador de inicio.
    # Leemos bytes hasta que encontremos el delimitador de fin.
    
    time_capturing_started = time.time()
    while True:
        if time.time() - time_capturing_started > CAPTURE_TIMEOUT:
            print("Timeout durante la captura de datos de la imagen.")
            # Intenta encontrar el delimitador en lo que se ha acumulado
            end_idx = image_data_buffer.find(END_DELIMITER)
            if end_idx != -1:
                print(f"Delimitador de fin encontrado en datos parciales después del timeout. Tamaño: {end_idx} bytes.")
                return image_data_buffer[:end_idx]
            else:
                print("No se pudo encontrar el delimitador de fin en los datos acumulados después del timeout.")
                return None

        bytes_available = ser.in_waiting
        if bytes_available > 0:
            chunk = ser.read(bytes_available)
            image_data_buffer.extend(chunk)
            # print(f"DEBUG: Leído chunk de {len(chunk)} bytes. Buffer total: {len(image_data_buffer)}") # Para depuración
        else: # No hay bytes inmediatamente disponibles, pequeña pausa
            time.sleep(0.01) 

        # Buscar el delimitador de fin en el buffer acumulado
        # El delimitador de fin es enviado por el ESP32 usando Serial.println(),
        # lo que significa que será END_DELIMITER seguido de \r\n.
        # Buscamos solo END_DELIMITER, ya que \r\n puede variar o ser parte de la imagen.
        end_idx = image_data_buffer.find(END_DELIMITER)
        if end_idx != -1:
            print(f"¡Delimitador de fin encontrado! Tamaño de imagen: {end_idx} bytes.")
            # La imagen son los datos ANTES del delimitador de fin
            final_image_data = image_data_buffer[:end_idx]
            
            # Limpiar el buffer de serie para eliminar el delimitador de fin y lo que siga
            # Esto es importante si esperas más mensajes después.
            # El resto del buffer (image_data_buffer[end_idx:]) contiene el delimitador y más.
            # Podríamos intentar leer la línea completa del delimitador de fin.
            # ser.readline() # Para consumir la línea del delimitador de fin
            # O simplemente confiar en que el reset_input_buffer() lo limpie antes de la próxima imagen.
            return final_image_data
    return None # No debería llegar aquí si el bucle tiene timeouts


def main():
    global SERIAL_PORT # Para permitir modificar la variable global si es necesario

    if SERIAL_PORT is None:
        SERIAL_PORT = list_serial_ports_and_select()
        if SERIAL_PORT is None:
            print("No se seleccionó ningún puerto. Saliendo.")
            return

    print(f"Intentando conectar a {SERIAL_PORT} a {BAUD_RATE} baudios...")

    try:
        # El timeout se aplica a las operaciones de lectura como readline() o read()
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=SERIAL_TIMEOUT)
        print(f"Conectado a {SERIAL_PORT}. Esperando imagen...")
        print("Asegúrate de que tu ESP32-CAM esté encendido y configurado para enviar una imagen.")

        # Bucle para recibir imágenes continuamente (útil ya que el ESP32 se reinicia)
        while True:
            # Limpiar cualquier dato antiguo en el buffer de entrada del puerto serie
            ser.reset_input_buffer()
            time.sleep(0.1) # Pequeña pausa para estabilizar

            img_bytes = receive_image_data(ser)

            if img_bytes:
                print(f"Recibidos {len(img_bytes)} bytes de datos de imagen.")
                try:
                    # Guardar la imagen
                    with open(IMAGE_FILENAME, "wb") as f:
                        f.write(img_bytes)
                    print(f"Imagen guardada como {IMAGE_FILENAME}")

                    # Mostrar la imagen
                    print("Intentando mostrar la imagen...")
                    # Usar io.BytesIO para tratar el array de bytes como un archivo en memoria
                    image_stream = io.BytesIO(img_bytes)
                    image = Image.open(image_stream)
                    image.show() # Esto usa el visor de imágenes predeterminado del sistema
                    print(f"Si la imagen no aparece, verifica tu visor de imágenes predeterminado o busca el archivo '{IMAGE_FILENAME}'.")
                
                except FileNotFoundError:
                    print(f"Error: No se pudo escribir en el archivo {IMAGE_FILENAME}. Verifica los permisos.")
                except IOError as e: # Pillow puede lanzar IOError por imágenes corruptas o no reconocidas
                    print(f"Error al procesar la imagen (puede estar corrupta o no ser un formato válido): {e}")
                except Exception as e:
                    print(f"Ocurrió un error inesperado al guardar/mostrar la imagen: {e}")
            else:
                print("No se recibieron datos de imagen o ocurrió un error en la recepción.")

            print(f"\nEsperando la siguiente imagen de {SERIAL_PORT} (el ESP32 debería reiniciarse y enviar de nuevo)...")
            print("Presiona Ctrl+C para salir.")
            # El ESP32 entra en deep sleep y se reinicia. El script esperará la próxima conexión/datos.
            # No es necesario un time.sleep() largo aquí si el ESP32 tarda en reiniciarse,
            # ya que las lecturas del puerto serie tienen timeouts.
            # Sin embargo, un breve sleep puede evitar que el bucle se ejecute demasiado rápido si hay problemas.
            time.sleep(1) # Pausa breve antes de intentar la siguiente captura

    except serial.SerialException as e:
        print(f"Error crítico del puerto serie: {e}")
        print("Asegúrate de que el dispositivo esté conectado y el puerto sea correcto.")
    except KeyboardInterrupt:
        print("\nSaliendo del programa por interrupción del usuario.")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Puerto serie cerrado.")

if __name__ == "__main__":
    main()