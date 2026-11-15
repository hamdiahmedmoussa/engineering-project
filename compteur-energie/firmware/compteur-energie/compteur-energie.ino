/*
  COMPTEUR D'ÉNERGIE CONNECTÉ · ESP32 + pince ampèremétrique SCT-013-030

  Chaque seconde :
    1. on mesure la tension de la pince pendant 200 ms (10 périodes du secteur à 50 Hz) ;
    2. on calcule le courant efficace, puis la puissance (courant × 220 V) ;
    3. on ajoute l'énergie consommée depuis la mesure précédente.
  Un petit serveur web affiche le tout sur le téléphone (même Wi-Fi).

  Avant de téléverser : copie config.exemple.h en config.h et remplis-le (voir README).
*/
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "config.h"
#include "mesure.h"
#include "page.h"

// 200 ms d'échantillons, un toutes les 100 µs = 2000 échantillons sur 10 périodes entières.
const int NB_ECHANTILLONS = 2000;
const unsigned long PAS_US = 100;
float echantillons[NB_ECHANTILLONS];

WebServer serveur(80);

double courant = 0;     // A
double puissance = 0;   // W (estimée : courant × tension nominale)
double energie = 0;     // kWh depuis la mise en marche
unsigned long derniereMesure = 0;

/** Lit la pince pendant 200 ms et renvoie le courant efficace (A). */
double mesurerCourant() {
  if (MODE_SIMULATION) {
    // Une clim simulée : compresseur allumé 35 s sur 60 (environ 5,5 A), ventilation seule le reste du temps.
    const bool compresseur = (millis() / 1000) % 60 < 35;
    delay(200);
    return (compresseur ? 5.5 : 0.4) + random(-10, 10) / 100.0;
  }

  unsigned long prochain = micros();
  for (int i = 0; i < NB_ECHANTILLONS; i++) {
    while ((long)(micros() - prochain) < 0) {}  // attendre l'instant exact de l'échantillon
    echantillons[i] = analogReadMilliVolts(BROCHE_PINCE) / 1000.0f;  // en volts, calibré par l'ESP32
    prochain += PAS_US;
  }
  const double tensionPince = valeurEfficace(echantillons, NB_ECHANTILLONS);
  return courantEfficace(tensionPince, AMPERE_PAR_VOLT, CALIBRATION, SEUIL_BRUIT_A);
}

void envoyerMesures() {
  char json[200];
  snprintf(json, sizeof(json),
           "{\"courant\":%.3f,\"puissance\":%.1f,\"energie\":%.5f,\"tension\":%.0f,\"depuis\":%lu,\"simulation\":%s}",
           courant, puissance, energie, TENSION_SECTEUR, millis() / 1000, MODE_SIMULATION ? "true" : "false");
  serveur.send(200, "application/json", json);
}

void connecterWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NOM, WIFI_MOT_DE_PASSE);
  Serial.print("Connexion au Wi-Fi");
  const unsigned long debut = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - debut < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connecté. Ouvre sur ton téléphone : http://");
    Serial.println(WiFi.localIP());
  } else {
    // Pas de Wi-Fi : le compteur crée son propre réseau.
    WiFi.mode(WIFI_AP);
    WiFi.softAP(RESEAU_COMPTEUR_NOM, RESEAU_COMPTEUR_MOT_DE_PASSE);
    Serial.print("Wi-Fi introuvable. Connecte-toi au réseau « " RESEAU_COMPTEUR_NOM " », puis ouvre http://");
    Serial.println(WiFi.softAPIP());
  }
  // Sur ordinateur (et iPhone), l'adresse http://compteur.local fonctionne aussi.
  if (MDNS.begin("compteur")) MDNS.addService("http", "tcp", 80);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(MODE_SIMULATION ? "Compteur d'énergie (MODE SIMULATION)" : "Compteur d'énergie");

  connecterWifi();
  serveur.on("/", []() { serveur.send_P(200, "text/html; charset=utf-8", PAGE_TABLEAU); });
  serveur.on("/mesures", envoyerMesures);
  serveur.begin();
  derniereMesure = millis();
}

void loop() {
  serveur.handleClient();

  if (millis() - derniereMesure >= 1000) {
    const unsigned long maintenant = millis();
    const double dureeS = (maintenant - derniereMesure) / 1000.0;
    derniereMesure = maintenant;

    courant = mesurerCourant();
    puissance = courant * TENSION_SECTEUR;
    energie += energieKWh(puissance, dureeS);

    Serial.printf("%.2f A  %.0f W  %.4f kWh\n", courant, puissance, energie);
  }
}
