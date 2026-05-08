# ADN LEGO

Simulador didactico de lector/secuenciador de ADN hecho con una placa ESP32-S3 SuperMini, un sensor de color TCS34725, una pantalla OLED I2C, botones de control y un motor paso a paso. La muestra fisica se representa con piezas o marcas de color que pasan frente al sensor; cada color se traduce a una base de ADN.

## Idea del proyecto

El equipo no secuencia ADN real. Simula el principio de lectura por posiciones: el motor mueve una tira o mecanismo LEGO una distancia fija, el sensor detecta el color en la nueva posicion y el firmware lo convierte en una base:

| Color | Base |
| --- | --- |
| Azul | A |
| Rojo | T |
| Verde | G |
| Amarillo | C |
| Sin lectura clara | N |

La secuencia se acumula en memoria, se muestra en la OLED y se imprime por el monitor Serial.

## Estructura del repositorio

```text
.
|-- lego_adnV2/
|   `-- lego_adnV2.ino      # Firmware Arduino/ESP32
`-- pcb/
    |-- adn_Lego.sch        # Esquematico EAGLE
    |-- adn_Lego.brd        # PCB EAGLE
    |-- adn_Lego_*.job      # Jobs/CAM de EAGLE
    `-- eagle.epf           # Proyecto EAGLE
```

## Hardware documentado en el PCB

El esquematico `pcb/adn_Lego.sch` contiene estos bloques:

| Nombre en PCB | Funcion |
| --- | --- |
| `ESP32S3_SUPERMINI` | Controlador principal |
| `U$1` | Pantalla OLED 128x64 I2C |
| `RGB` | Conector I2C para sensor de color TCS34725 |
| `I2C2` | Segundo conector I2C auxiliar |
| `SV1` | Conector de motor |
| `SW1` | Boton reset/nueva secuencia |
| `SW2` | Boton retroceder |
| `SW3` | Boton avanzar/leer base |
| `EXP/S3` | Header de expansion |

## Mapa de pines usado por el firmware

| Senal | GPIO ESP32-S3 |
| --- | --- |
| Motor IN1 | GPIO4 |
| Motor IN2 | GPIO3 |
| Motor IN3 | GPIO2 |
| Motor IN4 | GPIO1 |
| Boton reset | GPIO5 |
| Boton retroceder | GPIO6 |
| Boton avanzar | GPIO7 |
| I2C SDA | GPIO8 |
| I2C SCL | GPIO9 |

El firmware fuerza `Wire.begin(8, 9)` para coincidir con el PCB.

## Dependencias de Arduino

Instalar desde el Library Manager:

- `Adafruit TCS34725`
- `Adafruit BusIO`
- `U8g2`

Seleccionar una placa compatible con ESP32-S3 SuperMini en el core de ESP32 para Arduino.

## Uso

1. Abrir `lego_adnV2/lego_adnV2.ino` en Arduino IDE.
2. Seleccionar la placa ESP32-S3 y el puerto correcto.
3. Cargar el sketch.
4. Abrir el monitor Serial a `115200` baudios.
5. Colocar la tira LEGO o plantilla de colores frente al lector.
6. Pulsar avanzar para mover una posicion y leer una base.
7. Pulsar retroceder para volver una posicion y borrar la ultima base.
8. Pulsar reset para iniciar una nueva secuencia.

La salida Serial tiene este formato:

```text
>nueva_secuencia
Base 1: A | Secuencia: A
Base 2: T | Secuencia: AT
Base 3: G | Secuencia: ATG
```

## Cambios realizados al firmware

- Se reemplazo la lectura suelta de colores por un flujo de secuenciacion.
- Se agrego acumulacion de secuencia hasta 64 bases.
- Se agrego salida Serial con indice de base y secuencia completa.
- Se corrigio el I2C para usar SDA `GPIO8` y SCL `GPIO9`, como indica el PCB.
- Se eliminaron dependencias no usadas (`Stepper`, `AsyncStepperLib`, `cstring`).
- Se agrego liberacion de bobinas del motor despues de cada movimiento.
- Se normalizo la clasificacion de color usando proporciones RGB, menos sensible a cambios de brillo que valores absolutos.
- Se agrego lectura `N` cuando el color no es confiable.

## Ajustes necesarios para que funcione bien como simulador

Estos parametros estan al inicio de `lego_adnV2.ino` y probablemente deben calibrarse en el prototipo fisico:

| Parametro | Valor actual | Que ajustar |
| --- | --- | --- |
| `stepsPerBase` | `845` | Pasos entre dos colores consecutivos de la tira LEGO |
| `stepDelayMs` | `1` | Velocidad del motor; subir si pierde pasos |
| `readSettleMs` | `250` | Pausa despues de mover antes de leer color |
| `minClearToRead` | `250` | Umbral minimo de luz para aceptar lectura |
| Reglas de `identifyBase()` | Umbrales RGB normalizados | Afinar si un color se confunde con otro |

Para una version mas robusta, el siguiente cambio recomendado es agregar un modo de calibracion: mantener pulsado reset, presentar Azul/Rojo/Verde/Amarillo, guardar sus valores en memoria y clasificar por distancia al color calibrado.

## Notas del PCB

- El PCB usa I2C compartido para la OLED y el sensor TCS34725.
- La OLED esta en `3V3`.
- El conector `RGB` aparece alimentado desde `5V`; verificar el modulo TCS34725 usado. Muchos modulos toleran `3V3-5V`, pero si el sensor no tiene regulador o conversores de nivel, conviene alimentarlo a `3V3`.
- Los botones estan cableados a GND y el firmware usa `INPUT_PULLUP`.
- El motor requiere una etapa driver externa si el conector `SV1` va a bobinas o a un motor tipo 28BYJ-48. No se debe alimentar un motor directamente desde GPIO del ESP32.

## Estado actual

El proyecto queda preparado como simulador funcional de secuenciador: mueve por pasos, lee colores, traduce a bases A/T/G/C/N, muestra la lectura y construye una secuencia. La precision final depende de la calibracion mecanica de `stepsPerBase`, la iluminacion alrededor del sensor y el contraste de los colores usados.
