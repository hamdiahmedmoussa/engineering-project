/*
  SUIVI D'UN PANNEAU SOLAIRE · ESP32 + INA219 (tension, courant) + DS18B20 (température)

  Chaque seconde : tension et courant du panneau → puissance → énergie du jour.
  Toutes les 2 secondes : température du panneau.
  Chaque minute : on garde la moyenne (puissance, température) pour la courbe de la journée.
  Un petit serveur web affiche tout sur le téléphone.

  Bibliothèques à installer (Outils → Gérer les bibliothèques) :
    « Adafruit INA219 », « OneWire », « DallasTemperature ».
  Avant de téléverser : copie config.exemple.h en config.h et remplis-le (voir README).
*/
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <time.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"
#include "securite.h"
#include "solaire.h"
#include "page.h"

Adafruit_INA219 ina219;
OneWire busTemperature(BROCHE_TEMPERATURE);
DallasTemperature sonde(&busTemperature);
WebServer serveur(80);

// --- Mesures du moment ---
double tension = 0, courant = 0, puissance = 0;  // V, A, W
double temperature = NAN;                          // °C (NAN tant que la sonde n'a rien lu)
double energieJour = 0;                            // Wh
int jourEnCours = -1;                              // pour remettre l'énergie à zéro à minuit

// --- Courbe de la journée : une moyenne par minute, sur 24 h ---
const int NB_MINUTES = 24 * 60;
float courbePuissance[NB_MINUTES];
float courbeTemperature[NB_MINUTES];
int nbPoints = 0;          // points enregistrés (max NB_MINUTES)
int prochainPoint = 0;     // où écrire le prochain (tampon circulaire)
double sommeP = 0, sommeT = 0;
int nbP = 0, nbT = 0;

unsigned long derniereMesure = 0, derniereTemperature = 0, derniereMinute = 0;

/** L'heure exacte est-elle connue (récupérée sur internet) ? */
bool heureConnue() {
  return time(nullptr) > 1700000000;  // après novembre 2023 : l'heure a bien été reçue
}

void mesurer() {
  if (MODE_SIMULATION) {
    // Une journée de soleil : la production suit une courbe en cloche de 6 h à 18 h,
    // le panneau chauffe avec le soleil, et perd donc un peu de puissance.
    struct tm t;
    time_t maintenant = time(nullptr);
    localtime_r(&maintenant, &t);
    const double heure = heureConnue() ? t.tm_hour + t.tm_min / 60.0 : 12 + (millis() / 60000.0);
    const double soleil = fmax(0, sin(PI * (fmod(heure, 24) - 6) / 12));
    temperature = 28 + 36 * soleil + random(-5, 5) / 10.0;
    const double brute = PUISSANCE_NOMINALE_W * 0.95 * soleil * (1 + random(-3, 3) / 100.0);
    puissance = brute * (1 - perteChaleurPct(temperature, COEF_TEMPERATURE) / 100);
    tension = soleil > 0 ? 17.5 + random(-3, 3) / 10.0 : 0;
    courant = tension > 0 ? puissance / tension : 0;
    return;
  }
  // Tension aux bornes de la charge = tension du bus + chute dans la résistance de mesure (shunt).
  tension = ina219.getBusVoltage_V() + ina219.getShuntVoltage_mV() / 1000.0;
  courant = fmax(0, ina219.getCurrent_mA() / 1000.0);
  puissance = tension * courant;
}

void lireTemperature() {
  if (MODE_SIMULATION) return;
  const float t = sonde.getTempCByIndex(0);
  if (t != DEVICE_DISCONNECTED_C) temperature = t;
  sonde.requestTemperatures();  // lance la mesure suivante (prête dans moins d'une seconde)
}

void enregistrerMinute() {
  courbePuissance[prochainPoint] = nbP ? sommeP / nbP : 0;
  courbeTemperature[prochainPoint] = nbT ? sommeT / nbT : NAN;
  prochainPoint = (prochainPoint + 1) % NB_MINUTES;
  if (nbPoints < NB_MINUTES) nbPoints++;
  sommeP = sommeT = 0;
  nbP = nbT = 0;
}

void remiseAZeroAMinuit() {
  if (!heureConnue()) return;
  struct tm t;
  time_t maintenant = time(nullptr);
  localtime_r(&maintenant, &t);
  if (jourEnCours != -1 && t.tm_yday != jourEnCours) energieJour = 0;
  jourEnCours = t.tm_yday;
}

void envoyerMesures() {
  char temp[12] = "null";  // « null » en JSON tant que la sonde n'a rien lu
  if (!isnan(temperature)) snprintf(temp, sizeof(temp), "%.1f", temperature);
  char json[320];
  snprintf(json, sizeof(json),
           "{\"tension\":%.2f,\"courant\":%.3f,\"puissance\":%.2f,\"temperature\":%s,\"energie\":%.3f,"
           "\"nominale\":%.1f,\"coef\":%.2f,\"perte\":%.1f,\"p25\":%.2f,\"heure\":%ld,\"depuis\":%lu,\"simulation\":%s}",
           tension, courant, puissance, temp, energieJour,
           PUISSANCE_NOMINALE_W, COEF_TEMPERATURE,
           isnan(temperature) ? 0.0 : perteChaleurPct(temperature, COEF_TEMPERATURE),
           isnan(temperature) ? puissance : puissanceA25(puissance, temperature, COEF_TEMPERATURE),
           heureConnue() ? (long)time(nullptr) : 0L, millis() / 1000, MODE_SIMULATION ? "true" : "false");
  serveur.send(200, "application/json", json);
}

/** La courbe de la journée, du plus ancien au plus récent point (une valeur par minute). */
void envoyerCourbe() {
  serveur.setContentLength(CONTENT_LENGTH_UNKNOWN);  // envoi en plusieurs morceaux : le JSON est long
  serveur.send(200, "application/json", "");
  serveur.sendContent("{\"points\":[");
  const int debut = (prochainPoint - nbPoints + NB_MINUTES) % NB_MINUTES;
  char morceau[40];
  for (int i = 0; i < nbPoints; i++) {
    const int k = (debut + i) % NB_MINUTES;
    if (isnan(courbeTemperature[k])) snprintf(morceau, sizeof(morceau), "%s[%.1f,null]", i ? "," : "", courbePuissance[k]);
    else snprintf(morceau, sizeof(morceau), "%s[%.1f,%.1f]", i ? "," : "", courbePuissance[k], courbeTemperature[k]);
    serveur.sendContent(morceau);
  }
  serveur.sendContent("]}");
  serveur.sendContent("");  // fin de l'envoi
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
    configTime(DECALAGE_UTC_S, 0, "pool.ntp.org");  // l'heure exacte, si la box a internet
    Serial.print("Connecté. Ouvre sur ton téléphone : http://");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(RESEAU_BOITIER_NOM, RESEAU_BOITIER_MOT_DE_PASSE);
    Serial.print("Wi-Fi introuvable. Connecte-toi au réseau « " RESEAU_BOITIER_NOM " », puis ouvre http://");
    Serial.println(WiFi.softAPIP());
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(MODE_SIMULATION ? "Suivi solaire (MODE SIMULATION)" : "Suivi solaire");

  if (!MODE_SIMULATION) {
    if (!ina219.begin()) Serial.println("INA219 introuvable : vérifie SDA (21), SCL (22), 3,3 V et GND.");
    ina219.setCalibration_32V_2A();  // jusqu'à 26 V et environ 3 A : suffisant pour un panneau 12 V de 10 à 30 W
    sonde.begin();
    sonde.setWaitForConversion(false);  // ne pas bloquer le programme pendant la mesure de température
    sonde.requestTemperatures();
  }

  connecterWifi();
  serveur.on("/", []() { if (autorise(serveur)) serveur.send_P(200, "text/html; charset=utf-8", PAGE_SOLAIRE); });
  serveur.on("/mesures", []() { if (autorise(serveur)) envoyerMesures(); });
  serveur.on("/courbe", []() { if (autorise(serveur)) envoyerCourbe(); });
  serveur.begin();
  derniereMesure = derniereTemperature = derniereMinute = millis();
}

void loop() {
  serveur.handleClient();
  const unsigned long maintenant = millis();

  if (maintenant - derniereMesure >= 1000) {
    const double dureeS = (maintenant - derniereMesure) / 1000.0;
    derniereMesure = maintenant;
    mesurer();
    remiseAZeroAMinuit();
    energieJour += energieWh(puissance, dureeS);
    sommeP += puissance;
    nbP++;
    if (!isnan(temperature)) {
      sommeT += temperature;
      nbT++;
    }
  }
  if (maintenant - derniereTemperature >= 2000) {
    derniereTemperature = maintenant;
    lireTemperature();
  }
  if (maintenant - derniereMinute >= 60000) {
    derniereMinute = maintenant;
    enregistrerMinute();
    Serial.printf("%.1f W  %.1f V  %.2f A  %.1f °C  %.1f Wh\n", puissance, tension, courant, temperature, energieJour);
  }
}
