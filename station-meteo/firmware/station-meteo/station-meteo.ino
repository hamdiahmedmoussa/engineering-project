/*
  STATION MÉTÉO ET QUALITÉ DE L'AIR · ESP32 + BME280 + PMS5003

  Toutes les 10 s : température, humidité, pression (BME280) → température ressentie.
  Toutes les 5 min : on réveille le capteur de particules, on attend 30 s, on mesure PM1 / PM2,5 / PM10.
  Toutes les 5 min : on garde un point pour la courbe des 24 dernières heures.
  Le téléphone affiche tout, en phrases, avec des conseils selon la chaleur et la poussière.

  Bibliothèque à installer : « Adafruit BME280 Library » (elle installe aussi « Adafruit Unified Sensor »).
  Avant de téléverser : copie config.exemple.h en config.h et remplis-le (voir README).
*/
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <time.h>
#include <Adafruit_BME280.h>
#include "config.h"
#include "securite.h"
#include "meteo.h"
#include "pms.h"
#include "page.h"

Adafruit_BME280 bme;
HardwareSerial lienPMS(2);
WebServer serveur(80);

// --- Mesures du moment ---
float temperature = NAN, humidite = NAN, pression = NAN;
MesurePM air{0, 0, 0};
bool airMesure = false;
unsigned long derniereMesureAir = 0;   // millis() de la dernière mesure de particules

// --- Cycle du capteur de particules ---
enum class EtatPMS { Endormi, Chauffe };
EtatPMS etatPMS = EtatPMS::Endormi;
unsigned long debutEtatPMS = 0;
uint32_t sommePM1 = 0, sommePM25 = 0, sommePM10 = 0, nbTrames = 0;
uint8_t trame[32];
int position = 0;

// --- Courbe des 24 h : un point toutes les 5 min (288 points) ---
const int NB_POINTS = 288;
float courbeT[NB_POINTS], courbeH[NB_POINTS], courbePM[NB_POINTS], courbeP[NB_POINTS];
int nbPoints = 0, prochainPoint = 0;

bool heureConnue() { return time(nullptr) > 1700000000; }

void lireBME() {
  if (MODE_SIMULATION) {
    // Une journée chaude : 29 °C la nuit, 37 °C vers 14 h ; l'humidité baisse quand il fait chaud.
    // Heure locale (Djibouti) si elle est connue ; sinon, une journée défile en 24 minutes.
    const double h = heureConnue() ? ((time(nullptr) + DECALAGE_UTC_S) % 86400) / 3600.0 : fmod(10 + millis() / 60000.0, 24);
    const double jour = 0.5 + 0.5 * sin((h - 8) / 24.0 * 2 * PI);
    temperature = 29 + 8 * jour + random(-3, 3) / 10.0;
    humidite = 75 - 30 * jour + random(-2, 2);
    pression = 1008 - 2 * jour - millis() / 3600000.0;  // baisse lente : le temps change
    return;
  }
  temperature = bme.readTemperature();
  humidite = bme.readHumidity();
  pression = bme.readPressure() / 100.0f;  // en hPa
}

/** Lit les octets du PMS5003 et additionne les trames valides (pendant la chauffe). */
void lireOctetsPMS() {
  while (lienPMS.available()) {
    const uint8_t o = lienPMS.read();
    // On se cale sur le début de trame 42 4D.
    if (position == 0 && o != 0x42) continue;
    if (position == 1 && o != 0x4D) { position = 0; continue; }
    trame[position++] = o;
    if (position == 32) {
      MesurePM m;
      if (lireTramePMS(trame, 32, m) && millis() - debutEtatPMS > 20000) {  // les 20 premières secondes sont instables
        sommePM1 += m.pm1; sommePM25 += m.pm25; sommePM10 += m.pm10; nbTrames++;
      }
      position = 0;
    }
  }
}

void cycleAir() {
  const unsigned long maintenant = millis();
  if (etatPMS == EtatPMS::Endormi && (!derniereMesureAir || maintenant - derniereMesureAir >= INTERVALLE_AIR_S * 1000UL)) {
    etatPMS = EtatPMS::Chauffe;
    debutEtatPMS = maintenant;
    sommePM1 = sommePM25 = sommePM10 = nbTrames = 0;
    if (!MODE_SIMULATION) digitalWrite(BROCHE_PMS_SET, HIGH);  // réveil : le ventilateur démarre
  }
  if (etatPMS != EtatPMS::Chauffe) return;
  if (!MODE_SIMULATION) lireOctetsPMS();
  if (maintenant - debutEtatPMS < CHAUFFE_AIR_S * 1000UL) return;

  if (MODE_SIMULATION) {
    // Un peu de poussière, avec parfois un coup de vent de sable.
    const bool vent = random(0, 4) == 0;
    air = {(uint16_t)random(5, 12), (uint16_t)(vent ? random(40, 70) : random(12, 30)), (uint16_t)(vent ? random(180, 320) : random(40, 90))};
    airMesure = true;
  } else if (nbTrames > 0) {
    air = {(uint16_t)(sommePM1 / nbTrames), (uint16_t)(sommePM25 / nbTrames), (uint16_t)(sommePM10 / nbTrames)};
    airMesure = true;
  }
  if (!MODE_SIMULATION) digitalWrite(BROCHE_PMS_SET, LOW);  // au repos jusqu'à la prochaine mesure
  etatPMS = EtatPMS::Endormi;
  derniereMesureAir = maintenant;
}

void enregistrerPoint() {
  courbeT[prochainPoint] = temperature;
  courbeH[prochainPoint] = humidite;
  courbePM[prochainPoint] = airMesure ? air.pm25 : NAN;
  courbeP[prochainPoint] = pression;
  prochainPoint = (prochainPoint + 1) % NB_POINTS;
  if (nbPoints < NB_POINTS) nbPoints++;
}

/** Variation de la pression sur les 3 dernières heures (36 points de 5 min), ou NAN si pas assez de recul. */
float tendancePression() {
  if (nbPoints < 37 || isnan(pression)) return NAN;
  const float avant = courbeP[(prochainPoint - 37 + NB_POINTS) % NB_POINTS];
  return isnan(avant) ? NAN : pression - avant;
}

// ---------- Le serveur web ----------

String nombre(float v, int decimales = 1) { return isnan(v) ? String("null") : String(v, decimales); }

void envoyerMesures() {
  const float ress = isnan(temperature) || isnan(humidite) ? NAN : ressenti(temperature, humidite);
  String json;
  json.reserve(400);
  json += "{\"temperature\":" + nombre(temperature);
  json += ",\"humidite\":" + nombre(humidite, 0);
  json += ",\"pression\":" + nombre(pression);
  json += ",\"ressenti\":" + nombre(ress);
  json += ",\"niveauChaleur\":" + String(isnan(ress) ? -1 : niveauChaleur(ress));
  json += ",\"air\":" + String(airMesure ? "true" : "false");
  json += ",\"pm1\":" + String(air.pm1) + ",\"pm25\":" + String(air.pm25) + ",\"pm10\":" + String(air.pm10);
  json += ",\"niveauPM25\":" + String(niveauPM25(air.pm25)) + ",\"niveauPM10\":" + String(niveauPM10(air.pm10));
  json += ",\"ageAir\":" + String(airMesure ? (millis() - derniereMesureAir) / 1000 : 0UL);
  json += ",\"tendance\":" + nombre(tendancePression());
  json += ",\"heure\":" + String(heureConnue() ? (unsigned long)time(nullptr) : 0UL);
  json += ",\"simulation\":" + String(MODE_SIMULATION ? "true" : "false") + "}";
  serveur.send(200, "application/json", json);
}

/** La courbe des 24 h : [température, humidité, PM2,5] toutes les 5 min, du plus ancien au plus récent. */
void envoyerCourbe() {
  serveur.setContentLength(CONTENT_LENGTH_UNKNOWN);
  serveur.send(200, "application/json", "");
  serveur.sendContent("{\"points\":[");
  const int debut = (prochainPoint - nbPoints + NB_POINTS) % NB_POINTS;
  for (int i = 0; i < nbPoints; i++) {
    const int k = (debut + i) % NB_POINTS;
    String p = String(i ? "," : "") + "[" + nombre(courbeT[k]) + "," + nombre(courbeH[k], 0) + "," + nombre(courbePM[k], 0) + "]";
    serveur.sendContent(p);
  }
  serveur.sendContent("]}");
  serveur.sendContent("");
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
    configTime(DECALAGE_UTC_S, 0, "pool.ntp.org");
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
  Serial.println(MODE_SIMULATION ? "Station météo (MODE SIMULATION)" : "Station météo");
  if (!MODE_SIMULATION) {
    if (!bme.begin(ADRESSE_BME280)) Serial.println("BME280 introuvable : vérifie SDA (21), SCL (22) et l'adresse (0x76 ou 0x77).");
    lienPMS.begin(9600, SERIAL_8N1, BROCHE_PMS_RX, BROCHE_PMS_TX);
    pinMode(BROCHE_PMS_SET, OUTPUT);
    digitalWrite(BROCHE_PMS_SET, LOW);
  }
  connecterWifi();
  serveur.on("/", []() { if (autorise(serveur)) serveur.send_P(200, "text/html; charset=utf-8", PAGE_METEO); });
  serveur.on("/mesures", []() { if (autorise(serveur)) envoyerMesures(); });
  serveur.on("/courbe", []() { if (autorise(serveur)) envoyerCourbe(); });
  serveur.begin();
  lireBME();
}

unsigned long derniereLecture = 0, dernierPoint = 0;

void loop() {
  serveur.handleClient();
  cycleAir();
  const unsigned long maintenant = millis();
  if (maintenant - derniereLecture >= 10000) {
    derniereLecture = maintenant;
    lireBME();
  }
  if (maintenant - dernierPoint >= 300000UL || dernierPoint == 0) {
    dernierPoint = maintenant;
    enregistrerPoint();
    Serial.printf("%.1f °C  %.0f %%  %.1f hPa  PM2,5 %u  PM10 %u\n", temperature, humidite, pression, air.pm25, air.pm10);
  }
}
