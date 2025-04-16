#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>

#pragma region Constantes
const int TRIG = 8;
const int ECHO = 9;
const int BUZZER = 6;
const int LED_ROUGE = 4;
const int LED_BLEUE = 5;

#define MOTOR_INTERFACE_TYPE 4
#define IN1 31
#define IN2 33
#define IN3 35
#define IN4 37

LiquidCrystal_I2C lcd(0x27, 16, 2);
AccelStepper myStepper(MOTOR_INTERFACE_TYPE, IN1, IN3, IN2, IN4);
String numeroEtudiant = "2412384";
#pragma endregion

#pragma region Variables LCD
bool lcdIntroFini = false;
unsigned long lcdIntroStart = 0;
#pragma endregion

#pragma region Tasks

int getDistance(unsigned long ct) {
  static unsigned long lastTime = 0;
  static int lastResult = 0;
  int result = 0;
  const unsigned long rate = 50;

  if (ct - lastTime < rate) return lastResult;
  lastTime = ct;

  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);
  result = duration * 0.034 / 2;

  lastResult = result;
  return result;
}

void lcdTask(unsigned long ct, int distance) {
  static unsigned long lastTime = 0;
  const unsigned long rate = 100;
  if (ct - lastTime < rate) return;
  lastTime = ct;

  if (!lcdIntroFini) {
    if (lcdIntroStart == 0) {
      lcdIntroStart = ct;
      lcd.setCursor(0, 0);
      lcd.print(numeroEtudiant);
      lcd.setCursor(0, 1);
      lcd.print("Labo 05");
    } else if (ct - lcdIntroStart >= 2000) {
      lcd.clear();
      lcdIntroFini = true;
    }
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print("Dist : ");
  lcd.print(distance);
  lcd.print(" cm   ");

  lcd.setCursor(0, 1);
  lcd.print("Porte: ");

  int angleCible = 0;
  if (distance < 30) {
    lcd.print("Ouverte   ");
    angleCible = 170;
  } else if (distance > 60) {
    lcd.print("Fermee    ");
    angleCible = 10;
  } else {
    int angle = map(distance, 30, 60, 170, 10);
    lcd.print(angle);
    lcd.print((char)223);
    lcd.print(" deg ");
    angleCible = angle;
  }

  long positionEnPas = map(angleCible, 0, 360, 0, 2038);
  myStepper.moveTo(positionEnPas);

  
  Serial.print("etd:");
  Serial.print(numeroEtudiant);
  Serial.print(",dist:");
  Serial.print(distance);
  Serial.print(",deg:");
  Serial.println(angleCible);
}
#pragma endregion

#pragma region FSM

enum AppState { CALME, ALERTE, ATTENTE_FIN };
AppState appState = CALME;

int distance = 100;
bool presenceDetectee = false;
unsigned long tempsDebutAbsence = 0;

void stateManager(unsigned long ct) {
  switch (appState) {
    case CALME:
      calmeState(ct);
      break;
    case ALERTE:
      alerteState(ct);
      break;
    case ATTENTE_FIN:
      attenteFinState(ct);
      break;
  }
}

void calmeState(unsigned long ct) {
  static bool firstTime = true;
  static unsigned long lastTime = 0;
  const unsigned long rate = 100;

  if (firstTime) {
    firstTime = false;
    
    digitalWrite(BUZZER, LOW);
    digitalWrite(LED_ROUGE, LOW);
    digitalWrite(LED_BLEUE, LOW);
    return;
  }

  if (ct - lastTime < rate) return;
  lastTime = ct;

  distance = getDistance(ct);
  presenceDetectee = (distance <= 15);

  if (presenceDetectee) {
    firstTime = true;
    appState = ALERTE;
  }
}

void alerteState(unsigned long ct) {
  static bool firstTime = true;
  static unsigned long lastTime = 0;
  const unsigned long rate = 100;
  static bool ledState = false;

  if (firstTime) {
    firstTime = false;
    
    return;
  }

  if (ct - lastTime < rate) return;
  lastTime = ct;

  distance = getDistance(ct);
  presenceDetectee = (distance <= 15);

  digitalWrite(BUZZER, HIGH);
  ledState = !ledState;
  digitalWrite(LED_ROUGE, ledState);
  digitalWrite(LED_BLEUE, !ledState);

  if (!presenceDetectee) {
    tempsDebutAbsence = ct;
    firstTime = true;
    appState = ATTENTE_FIN;
  }
}

void attenteFinState(unsigned long ct) {
  static bool firstTime = true;
  static unsigned long lastTime = 0;
  const unsigned long rate = 100;

  if (firstTime) {
    firstTime = false;
    
    digitalWrite(BUZZER, LOW);
    digitalWrite(LED_ROUGE, LOW);
    digitalWrite(LED_BLEUE, LOW);
    return;
  }

  if (ct - lastTime < rate) return;
  lastTime = ct;

  distance = getDistance(ct);
  presenceDetectee = (distance <= 15);

  if (presenceDetectee) {
    firstTime = true;
    appState = ALERTE;
    return;
  }

  if (ct - tempsDebutAbsence >= 3000) {
    firstTime = true;
    appState = CALME;
  }
}

#pragma endregion

#pragma region setup-loop
unsigned long currentTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_ROUGE, OUTPUT);
  pinMode(LED_BLEUE, OUTPUT);

  lcd.init();
  lcd.backlight();

  myStepper.setMaxSpeed(1000);
  myStepper.setAcceleration(200);
  myStepper.setSpeed(300);
  myStepper.setCurrentPosition(0);
}

void loop() {
  currentTime = millis();
  stateManager(currentTime);
  lcdTask(currentTime, distance);
  myStepper.run();
}
#pragma endregion
