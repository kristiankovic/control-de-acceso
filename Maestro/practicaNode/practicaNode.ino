#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h> // libreria para enviar datos a la nube
#include <WiFiClient.h>
#include <Wire.h> 

const char* ssid = "nombre de red";
const char* password = "contrasenia de red";

// URL del servidor 
const String urlHosting = "www.dominio.com";
unsigned long ultimoEnvioNube = 0;

ESP8266WebServer server(80);

#define DIRECCION_ESCLAVO 0x08

float distanciaActual = 0.0;
float temperaturaActual = 0.0;
String estadoPuerta = "Cerrada";

struct RegistroDatos {
  unsigned long timestampSimulado;
  float distancia;
  float temperatura;
};

const int MAX_HISTORIAL = 10;
RegistroDatos historial[MAX_HISTORIAL];
int contadorRegistros = 0;

unsigned long ultimoPedidoI2C = 0;

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta charset='UTF-8'><title>Dashboard I2C Control de Acceso</title>";
  html += "<style>";
  html += "body{font-family:Arial; text-align:center; background:#f4f4f4; color:#333; margin:0; padding:20px;}";
  html += ".card{background:white; padding:20px; margin:15px auto; max-width:500px; border-radius:10px; box-shadow:0 4px 8px rgba(0,0,0,0.1);}";
  html += "h1{color:#2c3e50;} .val{font-size:24px; font-weight:bold; color:#2980b9;}";
  html += "table{width:100%; max-width:500px; margin:20px auto; border-collapse:collapse; background:white;}";
  html += "th, td{border:1px solid #ddd; padding:8px; text-align:center;} th{background:#2c3e50; color:white;}";
  html += "</style></head><body>";

  html += "<h1>SISTEMA DE CONTROL DE ACCESO (I2C)</h1>";
  
  html += "<div class='card'>";
  html += "<h2>Monitoreo en Tiempo Real</h2>";
  html += "<p>Estado del Acceso: <span class='val'>" + estadoPuerta + "</span></p>";
  html += "<p>Proximidad del Usuario: <span class='val'>" + String(distanciaActual) + " cm</span></p>";
  html += "<p>Temperatura del Entorno: <span class='val'>" + String(temperaturaActual) + " &deg;C</span></p>";
  html += "</div>";

  html += "<h2>Historial de Registros Local (I2C Resiliente)</h2>";
  html += "<table><tr><th>Tiempo (s)</th><th>Distancia</th><th>Temperatura</th></tr>";
  
  if (contadorRegistros == 0) {
    html += "<tr><td colspan='3'>Esperando sincronización de datos con el hardware...</td></tr>";
  } else {
    for (int i = contadorRegistros - 1; i >= 0; i--) {
      html += "<tr>";
      html += "<td>" + String(historial[i].timestampSimulado) + " s</td>";
      html += "<td>" + String(historial[i].distancia) + " cm</td>";
      html += "<td>" + String(historial[i].temperatura) + " &deg;C</td>";
      html += "</tr>";
    }
  }
  html += "</table></body></html>";

  server.send(200, "text/html", html);
}

// FUNCIÓN NUEVA: Envía las variables a la nube de forma asíncrona
void enviarAInternet(float dist, float temp) {

  if (WiFi.status() == WL_CONNECTED) { // Si hay salida a internet activa
    WiFiClient client;
    HTTPClient http;
    
    // Construir la URL con los parámetros dinámicos (?distancia=XX&temperatura=YY)
    String urlEnvio = urlHosting + "?distancia=" + String(dist) + "&temperatura=" + String(temp) + "&origen=nodemcu=";
    
    http.begin(client, urlEnvio);
    http.setTimeout(5000);
    
    // CABECERAS EXTREMAS ANTI-CACHÉ
    http.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    http.addHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
    http.addHeader("Connection", "keep-alive");

    int codigoHTTP = http.GET(); // "Visitar" el enlace
    
    if (codigoHTTP > 0) {
      Serial.print("[NUBE] Datos enviados. Servidor respondio: ");
      Serial.println(http.getString());
    } else {
      Serial.println("[NUBE] Error en el envío (Posible entorno local sin WAN)");
    }
    http.end();
  } else {
    Serial.println("[NUBE] Servidor offline. Guardando únicamente en RAM local.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Wire.begin(4, 5); 

  delay(200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando a la red Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.print(".");
  }
  
  Serial.println("\n[WIFI] ¡Conectado con éxito!");
  Serial.print("[WIFI] Escribe esta dirección IP en tu navegador: ");
  Serial.println(WiFi.localIP()); 
  
  server.on("/", handleRoot);
  server.begin();
  Serial.println("[WEB] Servidor HTTP listo.");
}

void loop() {
  server.handleClient(); 
  
  unsigned long tiempoActual = millis();
  if (tiempoActual - ultimoPedidoI2C >= 2000) {
    ultimoPedidoI2C = tiempoActual;
    
    Wire.requestFrom(DIRECCION_ESCLAVO, 15); 
    String datosRecibidos = "";
    
    while (Wire.available()) {
      char c = Wire.read();
      if (c == '\0' || c == '\n') break;
      datosRecibidos += c;
    }
    
    datosRecibidos.trim();
    
    int indiceComa = datosRecibidos.indexOf(',');
    
    if (indiceComa != -1) {
      String strDistancia = datosRecibidos.substring(0, indiceComa);
      String strTemperatura = datosRecibidos.substring(indiceComa + 1);
      
      distanciaActual = strDistancia.toFloat();
      temperaturaActual = strTemperatura.toFloat();
      
      if (distanciaActual <= 50.0 && distanciaActual > 0) {
        estadoPuerta = "Abierta (Usuario Detectado)";
      } else {
        estadoPuerta = "Cerrada (Área Despejada)";
      }
      
      // 1. Guardar en Historial Local (RAM)
      if (contadorRegistros < MAX_HISTORIAL) {
        historial[contadorRegistros].timestampSimulado = millis() / 1000;
        historial[contadorRegistros].distancia = distanciaActual;
        historial[contadorRegistros].temperatura = temperaturaActual;
        contadorRegistros++;
      } else {
        for (int i = 1; i < MAX_HISTORIAL; i++) {
          historial[i - 1] = historial[i];
        }
        historial[MAX_HISTORIAL - 1].timestampSimulado = millis() / 1000;
        historial[MAX_HISTORIAL - 1].distancia = distanciaActual;
        historial[MAX_HISTORIAL - 1].temperatura = temperaturaActual;
      }
      
      Serial.println("[I2C MASTER] Datos sincronizados de forma exitosa.");
      
      // 2. EJECUTAR ENVÍO REMOTO: Manda los datos capturados a la nube

      if (tiempoActual - ultimoEnvioNube >= 15000) { // Enviar cada 15 segundos (Reduces el consumo un 750%)
        ultimoEnvioNube = tiempoActual;
        enviarAInternet(distanciaActual, temperaturaActual);
      }
    }
  }
}