# Alarma de Cascos con ESP32 + ENC28J60 + MQTT

Este proyecto implementa un sistema de accionamiento de alarma para señal del sistema la detección de ausencia de casco de seguridad en una zona industrial implementado con Yolo (desarrollo de David Ninamancco). El sistema se basa en un ESP32 conectado a una red Ethernet mediante un módulo ENC28J60. Se comunica con un broker MQTT para recibir mensajes de alerta y ejecutar acciones (audio de advertencia y activación de luces).

## Características

- Comunicación MQTT mediante red Ethernet usando ENC28J60
- Activación de señal de alerta acústica y luminosa
- Sistema con 4 estados de alerta para evitar falsos positivos
- Control por mensajes MQTT (tema: `Alarmas/Cascos`)
- Implementación de Watchdog Timer para confiabilidad
- Cliente MQTT con ID único generado aleatoriamente

## Hardware

- ESP32
- Módulo Ethernet ENC28J60 (CS en GPIO5)
- Rele o LED indicador en GPIO2
- LED de estado inicial en GPIO33
- Altavoz o módulo de audio para reproducción de alerta (archivo `audio_cascos.h`)

## Conexión de red

El sistema utiliza una dirección IP estática:

### Ejemplo

IP: 192.168.146.207
Gateway: 192.168.146.1
DNS: 201.31.108.106
Subnet: 255.255.255.0
MAC: DE:AD:BE:EF:FE:00

## Configuración MQTT

- Servidor: `192.168.252.35`
- Puerto: `1883`
- Tema: `Alarmas/Cascos`
- Mensaje de activación esperado: `"sinCasco"`

## Estados del sistema

1. **Estado 0:** Espera de señal (`alarma = 1`). Se descartan falsos positivos (<300 ms).
2. **Estado 1:** Activación de señal luminosa anticipada.
3. **Estado 2:** Reproducción de audio de advertencia.
4. **Estado 3:** Espera para el siguiente ciclo (4 s).

## Dependencias

- `EthernetENC.h`
- `PubSubClient.h`
- `SPI.h`
- `WiFi.h`
- `esp_bt.h`
- `esp_task_wdt.h`
- Archivo `audio_cascos.h` con la rutina de alerta sonora

## Instalación

1. Instala las siguientes librerías en el entorno de Arduino:
   - EthernetENC
   - PubSubClient
2. Conecta el ENC28J60 al ESP32 (CS en GPIO5, SPI estándar).
3. Configura el archivo `audio_cascos.h` con la función `audioMascota()`.
4. Carga el sketch al ESP32 y asegúrate de que el broker MQTT esté operativo.

## Notas

- Se recomienda asegurar la unicidad del cliente MQTT con `generateClientID()`.
- El sistema incluye Watchdog Timer con reinicio tras 25 segundos sin respuesta.

## Créditos

Desarrollado por Juan Alva.