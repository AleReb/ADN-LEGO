/*
A azul
T Rojo
G Verde
C Amarillo
*/




#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include "AsyncStepperLib.h"
#include <Stepper.h>
#include <U8g2lib.h>
#include <cstring>  // Required for strcmp()

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE); //U8G2_R0 rotacion

// Configuración del motor
const int motorPin1 = 4;
const int motorPin2 = 3;
const int motorPin3 = 2;
const int motorPin4 = 1;
const int numSteps = 8;
const int stepsLookup[8] = { B1000, B1100, B0100, B0110, B0010, B0011, B0001, B1001 };
int stepCounter = 0;
const int stepsPerRevolution = 4076;

void clockwise() {
  stepCounter++;
  if (stepCounter >= numSteps) stepCounter = 0;
  setOutput(stepCounter);
}

void anticlockwise() {
  stepCounter--;
  if (stepCounter < 0) stepCounter = numSteps - 1;
  setOutput(stepCounter);
}

void setOutput(int step) {
  digitalWrite(motorPin1, bitRead(stepsLookup[step], 0));
  digitalWrite(motorPin2, bitRead(stepsLookup[step], 1));
  digitalWrite(motorPin3, bitRead(stepsLookup[step], 2));
  digitalWrite(motorPin4, bitRead(stepsLookup[step], 3));
}

// Ahora que las funciones están definidas, podemos crear el objeto stepper1
AsyncStepper stepper1(stepsPerRevolution, clockwise, anticlockwise);

// Configuraciones del sensor de color
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);

const int DELTA = 1500;
const int btnCal = 5;
const int btnBackward = 6;
const int btnforW = 7;
bool isBackwardButtonPressedBefore = false;
int stepsTaken = 0;
bool isButtonPressedBefore = false;

struct Color {
  int r, g, b;
  char name;
};

// Define colors using single-character identifiers
Color colors[] = {
  {10000, 13000, 11000, 'B'},  // Blanco
  {650, 1500, 2800, 'A'},     // Azul
  {10500, 8530, 3400, 'C'},    // Amarillo
  {1600, 4100, 2270, 'V'},     // Verde
  {3000, 1000, 700, 'R'},      // Rojo
  {130, 150, 130, 'n'}        // n (for not identified)
};
bool isWithinRange(int value, int target) {
  return value >= target - DELTA && value <= target + DELTA;
}

char identifyColor(int r, int g, int b) {
  for (int i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
    if (isWithinRange(r, colors[i].r) && 
        isWithinRange(g, colors[i].g) &&
        isWithinRange(b, colors[i].b)) {
      return colors[i].name;
    }
  }
  return 'n';  // Return 'n' if no color matches
}


void setup() {
  Serial.begin(115200);
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
  pinMode(btnCal, INPUT_PULLUP);
  pinMode(btnforW, INPUT_PULLUP);
  pinMode(btnBackward, INPUT_PULLUP);
  stepper1.SetSpeedRpm(15);
  u8g2.begin();

  if (!tcs.begin()) {
    Serial.println("No TCS34725 found ... check your connections");
  } else {
    Serial.println("Found sensor");
  }
}

bool isMotorRunning = false;  // Declaración de la variable
const int FIXED_STEPS = 845; // Establece la cantidad de pasos que el motor debe avanzar al presionar el botón
  uint16_t r, g, b, c;
void loop() {
  // Constantly check and display the color
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, ("R: " + String(r)).c_str());
  u8g2.drawStr(0, 20, ("G: " + String(g)).c_str());
  u8g2.drawStr(0, 30, ("B: " + String(b)).c_str());
  u8g2.drawStr(0, 40, ("C: " + String(c)).c_str());
  
   char colorName = identifyColor(r, g, b);
      if (colorName) {
        if (colorName == 'C') {
            u8g2.drawStr(0, 50, "Amarillo");
        } else if (colorName == 'A') {
            u8g2.drawStr(0, 50, "Azul");
        } else if (colorName == 'R') {
            u8g2.drawStr(0, 50, "Rojo");
        } else if (colorName == 'V') {
            u8g2.drawStr(0, 50, "verde");
        }
    }
//  u8g2.drawStr(0, 50, String(colorName));
  u8g2.sendBuffer();

  // Check for button presses
  int btnForward = digitalRead(btnforW);
  int btnBack = digitalRead(btnBackward);
  int rstcoun = digitalRead(btnCal);

  // Avanzar una cantidad fija de pasos al presionar el botón
  if (btnForward == LOW && !isButtonPressedBefore) {
    for(int i = 0; i < FIXED_STEPS; i++) {
        anticlockwise();
        delay(1); // Pausa entre pasos.
    }
    isButtonPressedBefore = true;

    // After moving, print the corresponding letter for the color
   char colorName = identifyColor(r, g, b);
    if (colorName) {
        if (colorName == 'C') {
            Serial.print("C");
        } else if (colorName == 'A') {
            Serial.print("A");
        } else if (colorName == 'R') {
            Serial.print("T");
        } else if (colorName == 'V') {
            Serial.print("G");
        }
    }
    Serial.println("boton avanzar");
  } else if (btnForward == HIGH) {
    isButtonPressedBefore = false;
  }

  // Retroceder una cantidad fija de pasos al presionar el botón
  if (btnBack == LOW && !isBackwardButtonPressedBefore) {
    for(int i = 0; i < FIXED_STEPS; i++) {
        clockwise();
        delay(1);
    }
    Serial.println("boton retroceder");
    isBackwardButtonPressedBefore = true;
  } else if (btnBack == HIGH) {
    isBackwardButtonPressedBefore = false;
  }

  // Restablecer contador de pasos
  if (rstcoun == LOW) {
    stepsTaken = 0;
    Serial.println("Nueva secuecia");
  }
}