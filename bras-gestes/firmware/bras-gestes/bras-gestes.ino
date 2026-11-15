/*
  BRAS ET MAIN ROBOTIQUES PILOTÉS PAR LA MAIN · côté ESP32
  La page web (Chrome ou Edge) envoie par USB des lignes de 9 angles (voir protocole.h).
  L'ESP32 vérifie chaque ligne, puis déplace les 9 servos sans à-coups (au plus 3° toutes les 10 ms).

  Les servos sont branchés sur une carte PCA9685 (16 sorties), reliée à l'ESP32 par I2C :
  elle génère elle-même les signaux des servos, et reçoit leur alimentation sur son bornier « V+ ».

  Bibliothèque à installer : « Adafruit PWM Servo Driver Library ».
  ⚠️ Les servos s'alimentent avec une alimentation 5-6 V séparée et puissante (voir README).
*/
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "limites.h"
#include "protocole.h"

Adafruit_PWMServoDriver carte;   // adresse I2C par défaut : 0x40 ; SDA = GPIO21, SCL = GPIO22
float actuels[NB_SERVOS];
int cibles[NB_SERVOS];

char tampon[64];
int longueur = 0;
unsigned long dernierPas = 0, derniereCommande = 0;
bool enAttente = false;

const float PAS_MAX = 3.0;            // degrés par pas…
const unsigned long PERIODE_MS = 10;  // … toutes les 10 ms : 300 °/s au maximum, comme la page (VITESSE_MAX)

/** Un angle (0 à 180°) devient une impulsion de 500 à 2400 µs, envoyée sur la sortie du servo. */
void ecrireAngle(int servo, float angle) {
  const int us = 500 + (int)(angle / 180.0f * 1900.0f);
  carte.writeMicroseconds(CANAUX[servo], us);
}

void traiterLigne() {
  tampon[longueur] = '\0';
  const Commande c = lireLigne(tampon);
  if (c.type == TypeCommande::Angles) {
    for (int i = 0; i < NB_SERVOS; i++) cibles[i] = c.angles[i];
    derniereCommande = millis();
    if (enAttente) Serial.println("Commandes reçues, le bras suit");
    enAttente = false;
  } else if (c.type == TypeCommande::Repos) {
    for (int i = 0; i < NB_SERVOS; i++) cibles[i] = REPOS[i];
    derniereCommande = millis();
    Serial.println("Retour en position de repos");
  } else {
    Serial.print("Commande refusée : ");
    Serial.println(tampon);
  }
}

void setup() {
  Serial.begin(115200);
  carte.begin();
  carte.setOscillatorFrequency(27000000);
  carte.setPWMFreq(50);  // les servos de modélisme attendent 50 impulsions par seconde
  delay(10);
  for (int i = 0; i < NB_SERVOS; i++) {
    actuels[i] = cibles[i] = REPOS[i];
    ecrireAngle(i, REPOS[i]);
  }
  Serial.println("Bras prêt, en position de repos");
}

void loop() {
  // 1. Lire les caractères reçus ; une ligne complète = une commande.
  while (Serial.available()) {
    const char ch = Serial.read();
    if (ch == '\n') {
      traiterLigne();
      longueur = 0;
    } else if (longueur < (int)sizeof(tampon) - 1) {
      tampon[longueur++] = ch;
    } else {
      longueur = 0;  // ligne trop longue : on l'ignore entièrement
    }
  }

  // 2. Avancer doucement vers les angles demandés.
  if (millis() - dernierPas >= PERIODE_MS) {
    dernierPas = millis();
    for (int i = 0; i < NB_SERVOS; i++) {
      const float ecart = cibles[i] - actuels[i];
      if (ecart == 0) continue;
      actuels[i] += ecart > PAS_MAX ? PAS_MAX : (ecart < -PAS_MAX ? -PAS_MAX : ecart);
      ecrireAngle(i, actuels[i]);
    }
  }

  // 3. Plus de commande depuis 3 s (page fermée, main perdue) : le bras reste où il est.
  if (!enAttente && derniereCommande && millis() - derniereCommande > 3000) {
    enAttente = true;
    Serial.println("Plus de commande : le bras reste immobile");
  }
}
