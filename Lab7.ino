#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <U8g2lib.h>
#include "Alarm.h"
#include "PorteAutomatique.h"

// --- Pins ---
#define TRIG 8
#define ECHO 9
#define BUZZER 6
#define LED_ROUGE 4
#define LED_BLEUE 5
#define LED_VERTE 3 

#define IN1 31
#define IN2 33
#define IN3 35
#define IN4 37

#define CLK_PIN 30
#define DIN_PIN 34
#define CS_PIN 32

// --- Objets globaux ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
U8G2_MAX7219_8X8_F_4W_SW_SPI u8g2(U8G2_R0, CLK_PIN, DIN_PIN, CS_PIN, U8X8_PIN_NONE, U8X8_PIN_NONE);

float distance = 100.0;
int distanceAlarme = 15;
int limInf = 10;
int limSup = 170;
unsigned long currentTime = 0;

// --- Objets du système ---
Alarm alarm(LED_ROUGE, LED_VERTE, LED_BLEUE, BUZZER, &distance);
PorteAutomatique porte(IN1, IN2, IN3, IN4, distance);

// --- MAX7219 ---
bool enCoursAffichageCarre = false;
unsigned long startAffichageCarre = 0;

String numeroEtudiant = "2412384";
bool lcdIntroFini = false;
unsigned long lcdIntroStart = 0;

// --- Lecture capteur ---
float getDistance(unsigned long ct) {
  static unsigned long lastTime = 0;
  static float lastResult = 100.0;
  const unsigned long rate = 50;
  if (ct - lastTime < rate) return lastResult;
  lastTime = ct;

  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);
  lastResult = duration * 0.034 / 2;
  return lastResult;
}

// --- LCD ---
void lcdTask(unsigned long ct) {
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
      lcd.print("Labo7");
    } else if (ct - lcdIntroStart >= 2000) {
      lcd.clear();
      lcdIntroFini = true;
    }
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print("Dist : ");
  lcd.print((int)distance);
  lcd.print(" cm   ");

  lcd.setCursor(0, 1);
  lcd.print("Porte: ");

  const char* etat = porte.getEtatTexte();
  if (strcmp(etat, "Ouverte") == 0 || strcmp(etat, "Fermee") == 0) {
    lcd.print(etat);
    lcd.print("       "); 
  } else {
    lcd.print((int)porte.getAngle());
    lcd.print((char)223);
    lcd.print(" deg ");
  }
}


// --- MAX7219 feedback ---
void afficherCarreMAX7219(unsigned long ct) {
  if (!enCoursAffichageCarre) return;
  if (ct - startAffichageCarre >= 3000) {
    u8g2.clear();
    enCoursAffichageCarre = false;
  } else {
    u8g2.clearBuffer();
    u8g2.drawBox(0, 0, 8, 8);
    u8g2.sendBuffer();
  }
}

void afficherConfirmation() {
  unsigned long start = millis();
  while (millis() - start < 3000) {
    u8g2.clearBuffer();
    u8g2.drawLine(1, 6, 3, 8);
    u8g2.drawLine(3, 8, 7, 0);
    u8g2.sendBuffer();
  }
  u8g2.clear();
}

void afficherErreur() {
  unsigned long start = millis();
  while (millis() - start < 3000) {
    u8g2.clearBuffer();
    u8g2.drawCircle(4, 4, 3);
    u8g2.drawLine(2, 2, 6, 6);
    u8g2.sendBuffer();
  }
  u8g2.clear();
}

void afficherInconnu() {
  unsigned long start = millis();
  while (millis() - start < 3000) {
    u8g2.clearBuffer();
    u8g2.drawLine(1, 1, 6, 6);
    u8g2.drawLine(6, 1, 1, 6);
    u8g2.sendBuffer();
  }
  u8g2.clear();
}

// --- Communication série ---
void gererCommandes() {
  if (!Serial.available()) return;
  String tampon = Serial.readStringUntil('\n');
  tampon.trim();
  tampon.toLowerCase();

  if (tampon == "g_dist") {
    Serial.println((int)distance);
    startAffichageCarre = millis();
    enCoursAffichageCarre = true;
    startAffichageCarre = millis();
    enCoursAffichageCarre = true;
  } else if (tampon.startsWith("cfg;alm;")) {
    int valeur = tampon.substring(8).toInt();
    distanceAlarme = valeur;
    afficherConfirmation();
  } else if (tampon.startsWith("cfg;lim_inf;")) {
    int valeur = tampon.substring(12).toInt();
    if (valeur < 170) {
      limInf = valeur;
      afficherConfirmation();
    } else {
      afficherErreur();
    }
  } else if (tampon.startsWith("cfg;lim_sup;")) {
    int valeur = tampon.substring(12).toInt();
    if (valeur > 10) {
      limSup = valeur;
      afficherConfirmation();
    } else {
      afficherErreur();
    }
  } else {
    afficherInconnu();
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  lcd.init();
  lcd.backlight();

  u8g2.begin();
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.setContrast(5);

  // Config alarm
  alarm.setDistance(15);
  alarm.setTimeout(3000);
  alarm.setVariationTiming(300);
  alarm.setColourA(1, 0, 0); // Rouge 
  alarm.setColourB(0, 0, 1); // Bleu 
  alarm.turnOn(); 

  // Config porte
  porte.setAngleOuvert(170);
  porte.setAngleFerme(10);
  porte.setPasParTour(2048);
  porte.setDistanceOuverture(30);
  porte.setDistanceFermeture(60);
}

// --- LOOP ---
void loop() {
  currentTime = millis();
  distance = getDistance(currentTime);

  gererCommandes();
  alarm.update();
  porte.update();

  lcdTask(currentTime);
  afficherCarreMAX7219(currentTime);
}
