/*
  Simulador de lector/secuenciador de ADN con LEGO.

  Mapeo de colores:
  - Azul     -> A
  - Rojo     -> T
  - Verde    -> G
  - Amarillo -> C

  Flujo:
  - Avanzar: mueve una posicion, lee el color y agrega una base.
  - Retroceder: vuelve una posicion y elimina la ultima base leida.
  - Reset: inicia una nueva secuencia.
*/

#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);

const uint8_t I2C_SDA = 8;
const uint8_t I2C_SCL = 9;

const uint8_t motorPin1 = 4;
const uint8_t motorPin2 = 3;
const uint8_t motorPin3 = 2;
const uint8_t motorPin4 = 1;

const uint8_t btnReset = 5;
const uint8_t btnBackward = 6;
const uint8_t btnForward = 7;

const int numSteps = 8;
const int stepsLookup[numSteps] = {
  B1000, B1100, B0100, B0110, B0010, B0011, B0001, B1001
};

const int stepsPerBase = 845;
const int stepDelayMs = 1;
const int readSettleMs = 250;
const uint16_t minClearToRead = 250;
const uint8_t maxSequenceLength = 64;

int stepCounter = 0;
uint16_t r, g, b, c;
char sequence[maxSequenceLength + 1] = "";
uint8_t sequenceLength = 0;

bool forwardWasPressed = false;
bool backwardWasPressed = false;
bool resetWasPressed = false;

Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_154MS,
  TCS34725_GAIN_4X
);

void setOutput(int step) {
  digitalWrite(motorPin1, bitRead(stepsLookup[step], 0));
  digitalWrite(motorPin2, bitRead(stepsLookup[step], 1));
  digitalWrite(motorPin3, bitRead(stepsLookup[step], 2));
  digitalWrite(motorPin4, bitRead(stepsLookup[step], 3));
}

void releaseMotor() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, LOW);
}

void clockwiseStep() {
  stepCounter++;
  if (stepCounter >= numSteps) {
    stepCounter = 0;
  }
  setOutput(stepCounter);
}

void anticlockwiseStep() {
  stepCounter--;
  if (stepCounter < 0) {
    stepCounter = numSteps - 1;
  }
  setOutput(stepCounter);
}

void moveSteps(int steps, bool forward) {
  for (int i = 0; i < steps; i++) {
    if (forward) {
      anticlockwiseStep();
    } else {
      clockwiseStep();
    }
    delay(stepDelayMs);
  }
  releaseMotor();
}

void readColor() {
  tcs.getRawData(&r, &g, &b, &c);
}

char identifyBase(uint16_t red, uint16_t green, uint16_t blue, uint16_t clear) {
  if (clear < minClearToRead) {
    return 'N';
  }

  uint32_t total = (uint32_t)red + green + blue;
  if (total == 0) {
    return 'N';
  }

  uint16_t rn = (uint32_t)red * 1000 / total;
  uint16_t gn = (uint32_t)green * 1000 / total;
  uint16_t bn = (uint32_t)blue * 1000 / total;

  if (bn > 430 && bn > rn + 90 && bn > gn + 60) {
    return 'A';
  }
  if (rn > 430 && rn > gn + 80 && rn > bn + 100) {
    return 'T';
  }
  if (gn > 420 && gn > rn + 70 && gn > bn + 70) {
    return 'G';
  }
  if (rn > 360 && gn > 330 && bn < 300) {
    return 'C';
  }

  return 'N';
}

const char *baseName(char base) {
  switch (base) {
    case 'A': return "A Azul";
    case 'T': return "T Rojo";
    case 'G': return "G Verde";
    case 'C': return "C Amarillo";
    default: return "N No leido";
  }
}

void appendBase(char base) {
  if (sequenceLength >= maxSequenceLength) {
    return;
  }
  sequence[sequenceLength++] = base;
  sequence[sequenceLength] = '\0';
}

void removeLastBase() {
  if (sequenceLength == 0) {
    return;
  }
  sequence[--sequenceLength] = '\0';
}

void resetSequence() {
  sequenceLength = 0;
  sequence[0] = '\0';
  Serial.println();
  Serial.println(">nueva_secuencia");
}

void printSequence(char lastBase) {
  Serial.print("Base ");
  Serial.print(sequenceLength);
  Serial.print(": ");
  Serial.print(lastBase);
  Serial.print(" | Secuencia: ");
  Serial.println(sequence);
}

void drawDisplay(char lastBase) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 9, "ADN LEGO Sequencer");

  u8g2.drawStr(0, 21, baseName(lastBase));

  char line[24];
  snprintf(line, sizeof(line), "R:%u G:%u", r, g);
  u8g2.drawStr(0, 33, line);
  snprintf(line, sizeof(line), "B:%u C:%u", b, c);
  u8g2.drawStr(0, 44, line);

  snprintf(line, sizeof(line), "Bases:%u", sequenceLength);
  u8g2.drawStr(0, 55, line);

  const uint8_t visibleChars = 18;
  const char *visibleSequence = sequence;
  if (sequenceLength > visibleChars) {
    visibleSequence = sequence + sequenceLength - visibleChars;
  }
  u8g2.drawStr(55, 55, visibleSequence);
  u8g2.sendBuffer();
}

void setup() {
  Serial.begin(115200);

  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
  releaseMotor();

  pinMode(btnReset, INPUT_PULLUP);
  pinMode(btnForward, INPUT_PULLUP);
  pinMode(btnBackward, INPUT_PULLUP);

  Wire.begin(I2C_SDA, I2C_SCL);
  u8g2.begin();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 12, "Iniciando sensor...");
  u8g2.sendBuffer();

  if (!tcs.begin()) {
    Serial.println("Error: sensor TCS34725 no encontrado");
    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "TCS34725 no encontrado");
    u8g2.drawStr(0, 26, "Revise SDA/SCL/3V3/GND");
    u8g2.sendBuffer();
  } else {
    Serial.println("TCS34725 OK");
    resetSequence();
  }
}

void loop() {
  readColor();
  char currentBase = identifyBase(r, g, b, c);
  drawDisplay(currentBase);

  bool forwardPressed = digitalRead(btnForward) == LOW;
  bool backwardPressed = digitalRead(btnBackward) == LOW;
  bool resetPressed = digitalRead(btnReset) == LOW;

  if (forwardPressed && !forwardWasPressed) {
    moveSteps(stepsPerBase, true);
    delay(readSettleMs);
    readColor();
    currentBase = identifyBase(r, g, b, c);
    appendBase(currentBase);
    printSequence(currentBase);
    drawDisplay(currentBase);
  }

  if (backwardPressed && !backwardWasPressed) {
    moveSteps(stepsPerBase, false);
    removeLastBase();
    Serial.print("Retroceso | Secuencia: ");
    Serial.println(sequence);
    drawDisplay(currentBase);
  }

  if (resetPressed && !resetWasPressed) {
    resetSequence();
    drawDisplay(currentBase);
  }

  forwardWasPressed = forwardPressed;
  backwardWasPressed = backwardPressed;
  resetWasPressed = resetPressed;

  delay(25);
}
