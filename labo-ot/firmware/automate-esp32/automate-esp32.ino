/*
  L'AUTOMATE DU LABORATOIRE SUR ESP32 · un vrai équipement Modbus TCP (port 502)

  Même poste que automate.py : 3 départs (Hôpital, Quartier résidentiel, Zone portuaire),
  dont les disjoncteurs sont 3 LED. Les courants sont simulés, et la protection coupe un départ
  dont le courant dépasse son seuil, comme sur un vrai poste.

  L'ESP32 crée son propre réseau Wi-Fi « Labo-OT » (adresse 192.168.4.1) : le PC du laboratoire
  s'y connecte, puis lance pare_feu.py --automate 192.168.4.1:502 et supervision.py.

  Avant de téléverser : copie config.exemple.h en config.h et change le mot de passe.
*/
#include <WiFi.h>
#include "config.h"
#include "poste.h"

constexpr bool memes(const char* a, const char* b) { return *a == *b && (*a == '\0' || memes(a + 1, b + 1)); }
constexpr size_t longueur(const char* s) { return *s ? 1 + longueur(s + 1) : 0; }
static_assert(!memes(RESEAU_LABO_MOT_DE_PASSE, "change-moi-2026"), "Change RESEAU_LABO_MOT_DE_PASSE dans config.h.");
static_assert(longueur(RESEAU_LABO_MOT_DE_PASSE) >= 8, "RESEAU_LABO_MOT_DE_PASSE : 8 caractères minimum.");

const char* DEPARTS[3] = {"Hôpital", "Quartier résidentiel", "Zone portuaire"};
const uint16_t COURANT_NORMAL[3] = {1200, 1800, 2400};  // dixièmes d'ampère

Poste poste;
WiFiServer serveur(502);

// Jusqu'à 4 clients en même temps (supervision, pare-feu, démonstration…)
const int MAX_CLIENTS = 4;
WiFiClient clients[MAX_CLIENTS];
uint8_t tampons[MAX_CLIENTS][260];
size_t remplis[MAX_CLIENTS] = {0};
uint8_t etatAvant[3] = {1, 1, 1};

void afficherChangements() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_DEPARTS[i], poste.bobines[i] ? HIGH : LOW);
    if (poste.bobines[i] != etatAvant[i]) {
      Serial.printf("Disjoncteur « %s » %s\n", DEPARTS[i], poste.bobines[i] ? "fermé" : "OUVERT");
      etatAvant[i] = poste.bobines[i];
    }
  }
}

void simuler() {
  const float t = millis() / 1000.0f;
  for (int i = 0; i < 3; i++) {
    if (!poste.bobines[i]) {
      poste.mesures[i] = 0;
      continue;
    }
    const float courant = COURANT_NORMAL[i] * (0.85f + 0.15f * sinf(t / 20 + i)) + random(-30, 30);
    poste.mesures[i] = courant > 0 ? (uint16_t)courant : 0;
    if (poste.mesures[i] > poste.registres[i]) {
      poste.bobines[i] = 0;
      Serial.printf("PROTECTION : « %s » coupé (%u A > seuil %u A)\n", DEPARTS[i], poste.mesures[i] / 10, poste.registres[i] / 10);
    }
  }
  poste.mesures[3] = 2300 + random(-20, 20);
  poste.mesures[4] = 5000 + random(-3, 3);
}

void servirClient(int k) {
  WiFiClient& c = clients[k];
  while (c.available() && remplis[k] < sizeof(tampons[k])) tampons[k][remplis[k]++] = c.read();
  while (remplis[k] >= 7) {
    const size_t total = 6 + ((tampons[k][4] << 8) | tampons[k][5]);
    if (total < 8 || total > 260) {  // longueur absurde : on coupe la connexion
      c.stop();
      remplis[k] = 0;
      return;
    }
    if (remplis[k] < total) return;  // trame incomplète : on attend la suite
    uint8_t reponse[260];
    const size_t n = traiterTrame(poste, tampons[k], total, reponse);
    if (n == 0) {
      c.stop();
      remplis[k] = 0;
      return;
    }
    c.write(reponse, n);
    memmove(tampons[k], tampons[k] + total, remplis[k] - total);
    remplis[k] -= total;
  }
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 3; i++) pinMode(LED_DEPARTS[i], OUTPUT);
  WiFi.softAP(RESEAU_LABO_NOM, RESEAU_LABO_MOT_DE_PASSE);
  serveur.begin();
  afficherChangements();
  Serial.print("Automate du laboratoire prêt. Réseau « " RESEAU_LABO_NOM " », Modbus sur ");
  Serial.print(WiFi.softAPIP());
  Serial.println(":502");
}

unsigned long derniereSimulation = 0;

void loop() {
  // Nouveau client : on le place dans une case libre (ou on le refuse si tout est plein).
  WiFiClient nouveau = serveur.available();
  if (nouveau) {
    int k = 0;
    while (k < MAX_CLIENTS && clients[k].connected()) k++;
    if (k < MAX_CLIENTS) {
      clients[k] = nouveau;
      remplis[k] = 0;
    } else {
      nouveau.stop();
    }
  }
  for (int k = 0; k < MAX_CLIENTS; k++) if (clients[k].connected()) servirClient(k);

  if (millis() - derniereSimulation >= 500) {
    derniereSimulation = millis();
    simuler();
  }
  afficherChangements();
}
