#include <Wire.h>
#include <Servo.h>
#include "DHT.h"

#include <SoftwareSerial.h> // Ya no se usa para comunicarse con NodeMCU

#define DIRECCION_I2C 0x08 // Dirección fija de este Arduino en el bus I2C

const int pinTrig = 2;
const int pinEcho = 3;
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

Servo miPuerta;
const int pinServo = 9;

volatile unsigned long tiempoInicio = 0;
volatile unsigned long tiempoFin = 0;
volatile bool nuevoDatoUltrasonico = false;
unsigned long ultimoDisparoUltrasonico = 0;
unsigned long ultimaLecturaTemperatura = 0;

float distanciaActual = 0.0;
float temperaturaActual = 0.0;
bool puertaAbierta = false;

// Variables para empaquetar los datos en el envío I2C
char paqueteDatos[32] = "0.0,0.0";

void setup() {
  Serial.begin(9600);
  dht.begin();
  
  miPuerta.attach(pinServo);
  miPuerta.write(90); // Posición inicial cerrada
  
  pinMode(pinTrig, OUTPUT);
  pinMode(pinEcho, INPUT);
  digitalWrite(pinTrig, LOW);
  
  attachInterrupt(digitalPinToInterrupt(pinEcho), echoInterrupt, CHANGE);
  
  // INICIALIZAR ARDUINO COMO ESCLAVO I2C
  Wire.begin(DIRECCION_I2C);
  Wire.onRequest(enviarDatosI2C); // Registrar el evento de petición del Maestro (Segundo plano nativo)
  
  Serial.println("--- Esclavo Configurado en Protocolo I2C (0x08) ---");
}

void loop() {
  unsigned long tiempoActual = millis();
  
  // variables globales para esperar a cerrar la puerta
  unsigned long tiempoCierre = 0;
  bool esperandoCerrar = false;

  if (tiempoActual - ultimoDisparoUltrasonico >= 100) {
    ultimoDisparoUltrasonico = tiempoActual;
    digitalWrite(pinTrig, HIGH);
    delayMicroseconds(10);
    digitalWrite(pinTrig, LOW);
  }

  if (nuevoDatoUltrasonico) {
    unsigned long duracion = tiempoFin - tiempoInicio;
    distanciaActual = (duracion * 0.0343) / 2.0;
    
    if (distanciaActual <= 50.0 && distanciaActual > 0) {
      
      if (!puertaAbierta) {

        Serial.println("[I2C LOCAL] - [Objeto Detectado] - Abriendo...");
        Serial.print("[INFO] - Distancia: ");
        Serial.print(distanciaActual);
        Serial.println("cm");

        for (int grado = 90; grado <= 180; grado += 2) {
          miPuerta.write(grado);
          delay(40);
        }

        puertaAbierta = true;
        Serial.println("[INFO] - [Puerta Abierta]");
      }
    } 
    
    else {

      if (puertaAbierta) {
        Serial.println("[I2C LOCAL] - [Objeto Removido] - Cerrando...");
        for (int grado = 180; grado >= 90; grado -= 2) {
          miPuerta.write(grado);
          delay(105);
        }
        puertaAbierta = false;
        Serial.println("[INFO] - [Puerta Cerrada]");
      }
    }
    nuevoDatoUltrasonico = false;
  }

  // Leer temperatura cada 10 segundos y actualizar el paquete global
  if (tiempoActual - ultimaLecturaTemperatura >= 10000) {
    ultimaLecturaTemperatura = tiempoActual;
    float t = dht.readTemperature();
    if (!isnan(t)) {
      temperaturaActual = t;

      //Serial.println("[I2C LOCAL] - [Temperatura] - Calculando...");
      Serial.print("[INFO] - Temperatura: ");
      Serial.print(temperaturaActual);
      Serial.println("°C");
      
      // Convertir las variables flotantes en una sola cadena de texto guardada en 'paqueteDatos'
      dtostrf(distanciaActual, 4, 2, paqueteDatos);
      strcat(paqueteDatos, ",");
      char tempStr[10];
      dtostrf(temperaturaActual, 4, 2, tempStr);
      strcat(paqueteDatos, tempStr);
    }
  }
}

void echoInterrupt() {
  if (digitalRead(pinEcho) == HIGH) {
    tiempoInicio = micros();
  } else {
    tiempoFin = micros();
    nuevoDatoUltrasonico = true;
  }
}

// FUNCIÓN EN SEGUNDO PLANO: Se ejecuta automáticamente por hardware cuando el NodeMCU pide datos
void enviarDatosI2C() {
  Wire.write(paqueteDatos); // Envía la cadena "DISTANCIA,TEMPERATURA"
}