/*
  BRAS ROBOT PILOTÉ PAR LA MAIN · côté ESP32
  La page web (Chrome ou Edge) envoie par USB des lignes « A base épaule coude pince ».
  L'ESP32 vérifie chaque ligne (protocole.h), puis déplace les 4 servos DOUCEMENT vers les angles
  demandés (au plus 3° toutes les 15 ms), même si la page envoyait un saut brutal.

  Bibliothèque à installer : « ESP32Servo » (Outils → Gérer les bibliothèques).
  ⚠️ Les servos s'alimentent avec une alimentation 5 V séparée, JAMAIS par la broche 5 V de l'ESP32.
*/
#include <ESP32Servo.h>
#include "limites.h"
#include "protocole.h"

Servo servos[NB_SERVOS];
float actuels[NB_SERVOS];
int cibles[NB_SERVOS];

char tampon[48];
int longueur = 0;
unsigned long dernierPas = 0, derniereCommande = 0;
bool enAttente = false;

const float PAS_MAX = 3.0;          // degrés par pas…
const unsigned long PERIODE_MS = 15; // … toutes les 15 ms (environ 200 °/s au maximum)

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
  for (int i = 0; i < NB_SERVOS; i++) {
    servos[i].setPeriodHertz(50);               // les servos de modélisme attendent 50 impulsions par seconde
    servos[i].attach(BROCHES[i], 500, 2400);    // impulsion de 0,5 ms (0°) à 2,4 ms (180°)
    actuels[i] = cibles[i] = REPOS[i];
    servos[i].write(REPOS[i]);
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
      servos[i].write((int)roundf(actuels[i]));
    }
  }

  // 3. Plus de commande depuis 3 s (page fermée, main perdue) : le bras reste où il est.
  if (!enAttente && derniereCommande && millis() - derniereCommande > 3000) {
    enAttente = true;
    Serial.println("Plus de commande : le bras reste immobile");
  }
}
