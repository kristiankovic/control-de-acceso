# 🚪 Sistema de Control de Acceso Residencial Inteligente con I2C y Sincronización Remota

Este proyecto implementa un sistema embebido coherente y funcional para el monitoreo y control de acceso automatizado. Utiliza una arquitectura multiprocesador **Maestro-Esclavo** mediante el protocolo **I2C**, integrando sensores de proximidad y temperatura locales con un dashboard web asíncrono y almacenamiento redundante (local mediante **DHCP** y remoto en la nube con **InfinityFree**).

---

## 🛠️ Requisitos de Hardware y Conexión

### Componentes:
* 1x Arduino Uno (Esclavo de Hardware)
* 1x NodeMCU ESP8266 (Maestro de Red y Servidor Web)
* 1x Servomotor (SG90 o MG90S)
* 1x Sensor Ultrasónico (HC-SR04)
* 1x Sensor de Temperatura (DHT11)
* Cables de conexión Dupont y Protoboard

### Diagrama de Conexión del Bus I2C:
| NodeMCU ESP8266 | Arduino Uno | Descripción |
| :--- | :--- | :--- |
| **D2 (GPIO 4)** | **A4** | Línea de Datos SDA (Con divisor de voltaje de 5V a 3.3V) |
| **D1 (GPIO 5)** | **A5** | Línea de Reloj SCL (Con divisor de voltaje de 5V a 3.3V) |
| **GND** | **GND / (-)** | Masa común obligatoria |

---

## 💻 Configuración del Entorno de Desarrollo (Arduino IDE)

Sigue estos pasos desde cero para preparar tu computadora:

1. **Descargar e Instalar Arduino IDE:**
   * Ve a la página oficial de [Arduino Software](https://www.arduino.cc/en/software) y descarga la versión más reciente para tu sistema operativo (Windows, macOS o Linux).
   * Ejecuta el instalador y acepta todos los controladores (drivers) firmados que te solicite.

2. **Instalar el Soporte para las Placas ESP8266 (NodeMCU):**
   * Abre Arduino IDE y ve a **Archivo > Preferencias**.
   * En el campo **Gestor de URLs Adicionales de Tarjetas**, pega el siguiente enlace:
     `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
   * Haz clic en **OK**.
   * Ve a **Herramientas > Placa > Gestor de Tarjetas**.
   * Busca `esp8266` por *ESP8266 Community* e instala la versión más reciente.

3. **Instalar Librerías Requeridas:**
   * Ve a **Sketch > Incluir Librería > Administrar Bibliotecas**.
   * Busca e instala las siguientes librerías oficiales:
     * **DHT sensor library** (por Adafruit) -> Selecciona *Instalar todas las dependencias* (esto instalará *Adafruit Unified Sensor* de forma automática).
     * *Nota: Las librerías `Wire.h`, `Servo.h`, `ESP8266WiFi.h` y `ESP8266WebServer.h` ya vienen preinstaladas por defecto en el sistema.*

---

## 🚀 Despliegue y Carga de Códigos

### Paso 1: Configurar el Servidor Remoto (Nube)
1. Inicia sesión en tu panel de **InfinityFree**.
2. Ve al **File Manager** y entra a la carpeta `htdocs`.
3. Crea un archivo llamado `guardar_registros.php` y pega el código backend PHP provisto en el proyecto.
4. Identifica tu dominio asignado (Ejemplo: `http://vv23011.great-site.net/`).

### Paso 2: Compilar y Cargar el Código en el Arduino Uno (Esclavo)
1. Conecta el Arduino Uno a tu computadora mediante el cable USB.
2. En Arduino IDE, ve a **Herramientas**:
   * **Placa:** Selecciona `Arduino Uno`.
   * **Puerto:** Elige el puerto COM asignado al Arduino (Ej: `COM3`).
3. Abre el archivo de código del Arduino, verifica el script y haz clic en el botón **Subir** (Flecha hacia la derecha).

### Paso 3: Compilar y Cargar el Código en el NodeMCU (Maestro)
1. Desconecta el Arduino y conecta la placa NodeMCU a la computadora.
2. Abre el archivo de código del NodeMCU.
3. **Configuración Crucial:** Modifica las líneas del Wi-Fi con tus credenciales y edita la variable global `urlHosting` apuntando a tu dominio real de la nube:
   ```cpp
   const char* ssid = "nombre-red";
   const char* password = "contraseña-red";
   const String urlHosting = "[http://tu-dominio.great-site.net/guardar_registros.php](http://tu-dominio.great-site.net/guardar_registros.php)";
