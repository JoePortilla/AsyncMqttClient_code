/**
 * \file AsyncMqttClient_pub main.cpp
 * \brief Publicar un contador en broker mediante la biblioteca AsyncMqttClient
 * \author Joseph Santiago Portilla. Ing. Electrónico - @JoePortilla
 */

// BIBLIOTECAS
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncMqttClient.h>
#include "secrets.hpp" // Credenciales de servicio y contraseñas de usuario.

// AJUSTES MQTT
// Creación de un objeto de la clase AsyncMqttClient
AsyncMqttClient mqttClient;
// ID de microcontrolador en el broker
const char *MQTT_CLIENTID = "ESP32testing1";
// Tópicos
const char *TOPIC_TEST = "ESP/test";
const char *TOPIC_WELCOME = "ESP/status";

// VARIABLES Y CONSTANTES
const long intervalo = 4000; // Intervalo de tiempo en el que se publican los datos.
unsigned long tActual = 0;   // Almacena el tiempo actual de ejecución.
unsigned long tPrevio = 0;   // Almacena el ultimo tiempo en que el dato fue publicado.
uint16_t contador = 0;       // Inicialización de contador.

// DECLARACIÓN DE FUNCIONES
// Funciones para AsyncMqttClient
void connectToWifi();
void WiFiStatus(WiFiEvent_t event);
// Funciones para AsyncMqttClient
void connectToMqtt();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttSubscribe(uint16_t packetId, uint8_t qos);
void onMqttUnsubscribe(uint16_t packetId);
void onMqttPublish(uint16_t packetId);
void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties,
                   size_t len, size_t index, size_t total);

void setup()
{
  // Inicializar la comunicación serial a 115200 baudios.
  Serial.begin(115200);

  // Callback de eventos WiFi
  WiFi.onEvent(WiFiStatus);

  // Ajustes de broker MQTT
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // mqttClient.setCredentials(MQTT_USER, MQTT_PASSWORD);
  mqttClient.setClientId(MQTT_CLIENTID);

  // Funciones de callback para AsyncMqttClient
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);

  // Conexión a WiFi y Broker MQTT
  connectToWifi();
}

void loop()
{
  // Almacenar tiempo actual de ejecución.
  tActual = millis();

  if (tActual - tPrevio >= intervalo)
  {
    // Aumentar contador
    contador++;
    // Publicar en el topico TOPIC_TEST, con un QOS=1 y RETENCION=false, la variable contador.
    mqttClient.publish(TOPIC_TEST, 1, false, String(contador).c_str());
    Serial.printf("Publicando 'Contador=%d' en tópico [%s]. (QoS 1)\n", contador, TOPIC_TEST);
    // Se almacena el ultimo tiempo en el que un dato fue publicado.
    tPrevio = tActual;
  }
}

// IMPLEMENTACIÓN DE FUNCIONES
// Implementación de funciones para WiFi

// Función para conectar el microcontrolador a WiFi
void connectToWifi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("\nConectandose a la red WiFi: %s\n", WIFI_SSID);
}

// Función de callback. Cada vez que haya un evento relacionado al WiFi se llama y ejecuta
// su operación segun el caso.
void WiFiStatus(WiFiEvent_t event)
{
  // Si el microcontrolador se pudo conectar al router y obtener la respectiva IP.
  if (event == SYSTEM_EVENT_STA_GOT_IP)
  {
    // Imprimir la IP y la fuerza de la señal WiFi.
    Serial.print("WiFi conectado. IP:");
    Serial.print(WiFi.localIP());
    Serial.printf(" RSSI: %d\n", WiFi.RSSI());

    // Si el microcontrolador se conectó a WiFi, intentar conexión a Broker.
    connectToMqtt();
  }

  // Si el microcontrolador no se pudo conectar o desconecto del router.
  if (event == SYSTEM_EVENT_STA_DISCONNECTED)
  {
    Serial.println("WiFi Desconectado");
  }
}

// Implementación de funciones para AsyncMqttClient

// Función para conectar el microcontrolador al broker
void connectToMqtt()
{
  mqttClient.connect();
  Serial.println("Iniciando conexión MQTT.");
}

// Función callback. Cada vez que el microcontrolador se conecte al broker
// se llama a esta función, aqui se envia un mensaje de bienvenida y se suscribe
// a los topicos definidos por el usuario.
void onMqttConnect(bool sessionPresent)
{
  // sessionPresent es una variable booleana que indica si el microcontrolador ha
  // encontrado una sesión previa con el broker al que se está conectando
  Serial.printf("%s conectado a MQTT. (Estado Sesión Previa=%d)\n", MQTT_CLIENTID, sessionPresent);

  // Se envia un mensaje de bienvenida para confirmar la conexión del ESP32 al broker.
  // El mensaje contiene "Nombre definido para identificar el microcontrolador + conectado".
  char WELCOME_MSG[50];
  strcpy(WELCOME_MSG, MQTT_CLIENTID);
  strcat(WELCOME_MSG, " conectado");
  // Publicar WELCOME_MSG en el topic TOPIC_WELCOME
  mqttClient.publish(TOPIC_WELCOME, 1, false, WELCOME_MSG);
}

// Función callback. Cada vez que el microcontrolador se desconecte del broker
// se llama a esta función. Se brindan los codigos de la razon de desconexión
// que se enumeran en AsyncMqttClientDisconnectReason.
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  Serial.printf("MQTT Desconectado\n");
  Serial.println(uint8_t(reason));
}

// Función callback. Cada vez que el microcontrolador se suscriba a un topico
// se llama a esta función. Si el id del paquete es 0 indica que hubo un error
// en la suscripción.
void onMqttSubscribe(uint16_t packetId, uint8_t qos)
{
  if (packetId != 0)
    Serial.printf("Suscripción correcta.\n");
  if (packetId == 0)
    Serial.printf("Error en la suscripción.\n");
}

// Función callback. Cada vez que el microcontrolador se desuscriba de un topico
// se llama a esta función. Si el id del paquete es 0 indica que hubo un error
// en la desuscripción.
void onMqttUnsubscribe(uint16_t packetId)
{
  if (packetId != 0)
    Serial.printf("Suscripción cancelada.\n");
  if (packetId == 0)
    Serial.printf("Error en la cancelación de suscripción\n");
}

// Función callback. Cada vez que el microcontrolador publique hacia un topico
// se llama a esta función. Si el id del paquete es 0 indica que hubo un error
// en la publicación.
void onMqttPublish(uint16_t packetId)
{
  if (packetId != 0)
    Serial.printf("Publicación correcta.\n");
  if (packetId == 0)
    Serial.printf("Error en la publicación.\n");
}

// Función callback. Cada vez que el microcontrolador reciba un mensaje de un topico
// al que se suscribió al conectarse al broker se llama a esta función.
void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties,
                   size_t len, size_t index, size_t total)
{
  // Se crea un string vacio para recibir los mensajes
  String msg = "";

  // Se formatea correctamente la información, concatenando los caracteres del payload entrante.
  for (size_t i = 0; i < len; ++i)
    msg += (char)payload[i];

  // Se eliminan espacios innecesarios al inicio o al final del mensaje
  msg.trim();

  // Se presenta el mensaje recibido, su topico y qos correspondiente.
  Serial.printf("Mensaje recibido [%s] (QoS:%d): %s.\n", topic, properties.qos, msg.c_str());
}