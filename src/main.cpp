#include <Arduino.h>
#include <EthernetENC.h>
#include <PubSubClient.h>
#include <SPI.h>
#include "audio_cascos.h"
#include "esp_task_wdt.h"
#include <WiFi.h>
#include <esp_bt.h>

// Watchdog Timer
#define WDT_TIMEOUT 25 // Tiempo en segundos

// Configuración del módulo ENC28J60
#define ETH_CS_PIN 5  // Pin CS del ENC28J60 conectado al GPIO5 del ESP32

// Pin para actuador
#define LED_PIN 2         // Cambiar si se usa otro pin
#define LED_ESTADO_CERO 33  // Indicador de estado 0

// Dirección MAC del dispositivo (debe ser única en tu red)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x00 };

// Direccion IP estatica (segmento de mejora de procesos)
IPAddress ip(192, 168, 146, 207);
IPAddress dnsServer(201, 31, 108, 106);
IPAddress gateway(192, 168, 146, 1);
IPAddress subnet(255, 255, 255, 0);

// Configuración MQTT
const char* mqtt_server = "192.168.252.35";       // Broker MQTT público
const int mqtt_port = 1883;                       // Puerto MQTT (no seguro)
const char* mqtt_topic = "Alarmas/Cascos";            // Tema MQTT
char mqtt_client_id[20];                     // Buffer para el ID del cliente MQTT
//const char* mqtt_client_id = "ESP32Client";       // ID del cliente MQTT
// Falta cambiar nombre de cliente

// Instancias de Ethernet y MQTT
EthernetClient ethClient;
PubSubClient mqttClient(ethClient);

// Alarmas
unsigned long tiempo = 0; //
int estado = 0; //
int alarma = 0;

// Tiempos de activación de los estados como constantes
const unsigned long tiempo_anti_falsos_positivos = 300;
const unsigned long tiempo_adelanto_circulina = 500;      // Tiempo que se adelanta el accionamiento de la circulina al mensaje de audio
const unsigned long tiempo_espera_despues_audio = 1000;        
const unsigned long tiempo_espera_nuevo_ciclo = 4000;             

// Declaraciones de funciones (prototipos)
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();
void audioMascota();
void generateClientID();

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando conexión Ethernet y MQTT...");
    
  // Inicializar el generador de números aleatorios
  randomSeed(analogRead(0));  // Usa una entrada analógica para generar una semilla aleatoria

  // Generar un ID de cliente único
  generateClientID();

  // Configurar el pin del LED o rele como salida
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);    // Inicialmente apagado

  pinMode(LED_ESTADO_CERO, OUTPUT);
  digitalWrite(LED_ESTADO_CERO, HIGH);    // Inicialmente apagado

  // Inicializar el módulo ENC28J60
  Ethernet.init(ETH_CS_PIN);

  // Iniciar la conexión Ethernet
  //if (Ethernet.begin(mac) == 0) {
    //Serial.println("Error al configurar Ethernet usando DHCP");
    // Intentar con una configuración manual si DHCP falla
  Ethernet.begin(mac, ip, dnsServer, gateway, subnet);
  //}

  // Verificar si la conexión Ethernet está activa
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("No se detectó el módulo ENC28J60. Verifica las conexiones.");
    while (true) {
      delay(1);  // Bucle infinito si no hay hardware
    }
  }

  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("El cable Ethernet no está conectado.");
  }

  // Mostrar la dirección IP asignada
  Serial.print("Dirección IP asignada: ");
  Serial.println(Ethernet.localIP());

  // Configurar el servidor MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);  // Función para manejar mensajes recibidos

  // Conectar al broker MQTT
  if (mqttClient.connect(mqtt_client_id)) {
    Serial.println("Conectado al broker MQTT");
    mqttClient.subscribe(mqtt_topic);  // Suscribirse al tema
    Serial.println("Suscrito al tema: " + String(mqtt_topic));
  } else {
    Serial.println("Error al conectar al broker MQTT");
  }
  delay(3000);
  // Configurar el Watchdog Timer
  esp_task_wdt_init(WDT_TIMEOUT, true);  // Tiempo límite y reinicio automático
  esp_task_wdt_add(NULL);  // Añadir la tarea actual (loop) al WDT
}

void loop() {
  esp_task_wdt_reset();  // Alimenta el watchdog

  // Mantener la conexión MQTT activa
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
  
  switch (estado) {
    case 0:
      Serial.println("estado 0");
      digitalWrite(LED_ESTADO_CERO, LOW);
      digitalWrite(LED_PIN, HIGH);
      if (alarma == 1) {  // Botón presionado
        if (tiempo == 0) {  // Si no se había registrado tiempo
          tiempo = millis();
        }
        if (millis() - tiempo >= tiempo_anti_falsos_positivos) {  // Si han pasado 0.3 segundos
          estado = 1;
          tiempo = millis();  // Reiniciar tiempo
        }
      } else {
        tiempo = 0;  // Reiniciar si se suelta antes de 0.5s
      }
    break;

    case 1:
      Serial.println("estado 1");
      digitalWrite(LED_ESTADO_CERO, HIGH);
      digitalWrite(LED_PIN, LOW);
      if (millis()- tiempo >= tiempo_adelanto_circulina) {
        estado = 2;
        tiempo = millis();
      }
    break;

    case 2:
      Serial.println("estado 2");
      audioMascota();
      if (millis()- tiempo >= tiempo_espera_despues_audio) {
        estado = 3;
        tiempo = millis();
      }
    break;

    case 3:
      Serial.println("estado 3");
      //digitalWrite(LED_PIN, HIGH);
      if (millis()- tiempo >= tiempo_espera_nuevo_ciclo) {
        alarma = 0;
        estado = 0;
      }
    break;
  }
}

// Función para manejar mensajes MQTT recibidos
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Convertir el payload a un String
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Mostrar el mensaje recibido
  Serial.print("Mensaje recibido en el tema: ");
  Serial.print(topic);
  Serial.print(" - Mensaje: ");
  Serial.println(message);

  // Comparar el mensaje recibido
  if (message.equals("sinCasco")) {
    Serial.println("Comando recibido: ON");
    // Aquí puedes agregar la lógica para encender algo
    Serial.println("Comando recibido: Activando flag");
    alarma = 1;
  } else {
    Serial.println("Comando no reconocido");
  }
}

// Función para reconectar al broker MQTT
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.println("Intentando reconectar al broker MQTT...");

    // Generar un ID de cliente único
    generateClientID();
    
    if (mqttClient.connect(mqtt_client_id)) {
      Serial.println("Conectado al broker MQTT");
      mqttClient.subscribe(mqtt_topic);  // Suscribirse al tema nuevamente
    } else {
      Serial.println("Error al reconectar, intentando de nuevo en 5 segundos...");
      delay(5000);
    }
  }
}

// Función para generar un ID de cliente único
void generateClientID() {
  // Nombre base del cliente
  String baseName = "alarma-cascos";

  // Generar un número aleatorio de 4 dígitos
  int randomNumber = random(1000, 9999);  // Número entre 1000 y 9999

  // Crear el ID del cliente concatenando el nombre base y el número aleatorio
  String clientID = baseName + String(randomNumber);

  // Convertir el String a un array de caracteres (necesario para PubSubClient)
  clientID.toCharArray(mqtt_client_id, 20);

  Serial.print("ID del cliente generado: ");
  Serial.println(mqtt_client_id);
}