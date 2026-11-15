/*
  COUPURE INTELLIGENTE DE CLIMATISEUR · ESP32 + infrarouge + radar de présence + DHT22

  Le boîtier parle à la clim EN INFRAROUGE, comme sa télécommande : aucun fil 220 V.
  Il écoute aussi la vraie télécommande, pour toujours savoir si la clim est allumée et à combien.

  Chaque seconde, il applique les règles de regles.h :
    pièce vide depuis 15 min → éteindre (jamais la nuit) ; trop froid → remonter à 24 °C ;
    pièce encore très chaude après 45 min → alerte.
  Le téléphone affiche tout, garde un journal, et permet d'allumer ou d'éteindre.

  Bibliothèques (Outils → Gérer les bibliothèques) : « IRremoteESP8266 », « DHT sensor library » (Adafruit).
  Avant de téléverser : copie config.exemple.h en config.h et remplis-le (voir README).
*/
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <stdarg.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <DHT.h>
#include "config.h"
#include "regles.h"
#include "page.h"

// En simulation, une « minute » dure une seconde : on voit les règles agir sans attendre.
const unsigned long UNITE_MS = MODE_SIMULATION ? 1000UL : 60000UL;

IRac emetteur(BROCHE_IR_LED);
IRrecv recepteur(BROCHE_IR_RECEPTEUR, 1024, 50, true);
decode_results signalRecu;
DHT dht(BROCHE_DHT, DHT22);
WebServer serveur(80);

const Reglages REGLES{DELAI_SANS_PRESENCE_MIN, NUIT_DEBUT_H * 60, NUIT_FIN_H * 60, CONSIGNE_MIN,
                      SEUIL_SURCHAUFFE_C, DELAI_SURCHAUFFE_MIN};

// --- Ce que l'on sait de la clim ---
stdAc::state_t clim;            // état connu (allumée ? consigne ? mode ?)
unsigned long allumageMs = 0;   // quand elle a été allumée
bool ecoApplique = false;       // consigne déjà remontée depuis cet allumage
bool alerte = false;

// --- La pièce ---
bool presence = true;
unsigned long dernierePresenceMs = 0;
float temperature = NAN, humidite = NAN;

// --- Temps de clim évité (après une extinction automatique, jusqu'au rallumage) ---
double eviteS = 0;
unsigned long eteinteAutoMs = 0;   // 0 = pas d'extinction automatique en cours
int jourEnCours = -1;

// --- Le journal : les 12 derniers événements, affichés sur le téléphone ---
struct Evenement {
  time_t heure;          // heure exacte (0 si inconnue)
  unsigned long depuisS; // secondes depuis le démarrage
  char texte[96];
};
const int TAILLE_JOURNAL = 12;
Evenement journal[TAILLE_JOURNAL];
int nbEvenements = 0, prochainEvenement = 0;

bool heureConnue() { return time(nullptr) > 1700000000; }

/** Ajoute une ligne au journal (et l'écrit dans le Moniteur série). Pas de guillemets dans les textes. */
void noter(const char* format, ...) {
  Evenement& e = journal[prochainEvenement];
  va_list args;
  va_start(args, format);
  vsnprintf(e.texte, sizeof(e.texte), format, args);
  va_end(args);
  e.heure = heureConnue() ? time(nullptr) : 0;
  e.depuisS = millis() / 1000;
  prochainEvenement = (prochainEvenement + 1) % TAILLE_JOURNAL;
  if (nbEvenements < TAILLE_JOURNAL) nbEvenements++;
  Serial.println(e.texte);
}

/** Met à jour ce que l'on sait de la clim (après un envoi, ou en écoutant la télécommande). */
void changerEtat(const stdAc::state_t& nouvel) {
  const unsigned long maintenant = millis();
  if (nouvel.power && !clim.power) {
    allumageMs = maintenant;
    ecoApplique = false;
    alerte = false;
    if (eteinteAutoMs) {  // rallumée après une extinction automatique : on compte le temps évité
      eviteS += (maintenant - eteinteAutoMs) * 60.0 / UNITE_MS;
      eteinteAutoMs = 0;
    }
  }
  clim = nouvel;
}

/** Envoie un nouvel état à la clim par infrarouge, puis le retient. */
void envoyer(const stdAc::state_t& nouvel) {
  if (!MODE_SIMULATION) emetteur.sendAc(nouvel, &clim);
  changerEtat(nouvel);
}

stdAc::state_t eteinte() {
  stdAc::state_t e = clim;
  e.power = false;
  return e;
}

stdAc::state_t allumeeA(float degres) {
  stdAc::state_t e = clim;
  e.power = true;
  e.mode = stdAc::opmode_t::kCool;
  e.degrees = degres;
  return e;
}

/** La vraie télécommande a été utilisée : on lit son message pour connaître le nouvel état. */
void ecouterTelecommande() {
  if (!recepteur.decode(&signalRecu)) return;
  const decode_type_t protocole = signalRecu.decode_type;
  if (IRac::isProtocolSupported(protocole)) {
    stdAc::state_t lu;
    if (IRAcUtils::decodeToState(&signalRecu, &lu, &clim)) {
      const bool etaitAllumee = clim.power;
      changerEtat(lu);
      if (lu.power && !etaitAllumee) noter("Télécommande : allumée à %.0f °C", lu.degrees);
      else if (!lu.power && etaitAllumee) noter("Télécommande : éteinte");
      else if (lu.power) noter("Télécommande : réglée sur %.0f °C", lu.degrees);
    }
  }
  // Utile à la première installation : le nom du protocole à mettre dans config.h.
  Serial.print("Signal infrarouge reçu, protocole : ");
  Serial.println(typeToString(protocole));
  recepteur.resume();
}

void lirePresence() {
  if (MODE_SIMULATION) {
    // Quelqu'un est là 50 s sur 180 ; en revenant, il rallume la clim (trop froid, pour voir le mode éco).
    presence = (millis() / 1000) % 180 < 50;
    if (presence && !clim.power && eteinteAutoMs) {
      changerEtat(allumeeA(20));
      noter("Télécommande : allumée à 20 °C (simulation)");
    }
  } else {
    presence = digitalRead(BROCHE_PRESENCE) == HIGH;
  }
  if (presence) dernierePresenceMs = millis();
}

void lireTemperature() {
  if (MODE_SIMULATION) {
    const float minutesAllumee = clim.power ? (millis() - allumageMs) / (float)UNITE_MS : 0;
    temperature = clim.power ? fmaxf(clim.degrees + 1, 31 - minutesAllumee * 0.3f) : 31;
    humidite = 60;
    return;
  }
  temperature = dht.readTemperature();
  humidite = dht.readHumidity();
}

int heureEnMinutes() {
  if (MODE_SIMULATION) return 15 * 60;  // un après-midi simulé : le mode nuit ne bloque pas la démonstration
  if (!heureConnue()) return -1;
  struct tm t;
  time_t maintenant = time(nullptr);
  localtime_r(&maintenant, &t);
  return t.tm_hour * 60 + t.tm_min;
}

void appliquerRegles() {
  const unsigned long maintenant = millis();
  Situation s;
  s.climAllumee = clim.power;
  s.consigne = (int)clim.degrees;
  s.presence = presence;
  s.minutesSansPresence = presence ? 0 : (maintenant - dernierePresenceMs) / UNITE_MS;
  s.minutesDepuisAllumage = (maintenant - allumageMs) / UNITE_MS;
  s.heureMin = heureEnMinutes();
  s.temperaturePiece = temperature;
  s.ecoDejaApplique = ecoApplique;

  const Decision d = decider(s, REGLES);
  if (d.action == Action::Eteindre) {
    envoyer(eteinte());
    eteinteAutoMs = maintenant;
    noter("Éteinte automatiquement : personne depuis %d min", s.minutesSansPresence);
  } else if (d.action == Action::RemonterConsigne) {
    envoyer(allumeeA(CONSIGNE_MIN));
    ecoApplique = true;
    noter("Mode éco : consigne remontée de %d à %d °C", s.consigne, CONSIGNE_MIN);
  }
  if (d.alerteSurchauffe && !alerte) {
    noter("Alerte : encore %.0f °C après %d min de clim", temperature, s.minutesDepuisAllumage);
  }
  alerte = d.alerteSurchauffe;
}

void remiseAZeroAMinuit() {
  if (!heureConnue()) return;
  struct tm t;
  time_t maintenant = time(nullptr);
  localtime_r(&maintenant, &t);
  if (jourEnCours != -1 && t.tm_yday != jourEnCours) {
    eviteS = 0;
    if (eteinteAutoMs) eteinteAutoMs = millis();  // le temps évité repart de minuit
  }
  jourEnCours = t.tm_yday;
}

// ---------- Le serveur web ----------

String nombreOuNull(float v) { return isnan(v) ? String("null") : String(v, 1); }

void envoyerEtat() {
  const unsigned long maintenant = millis();
  const double evite = eviteS + (eteinteAutoMs ? (maintenant - eteinteAutoMs) * 60.0 / UNITE_MS : 0);
  const int h = heureEnMinutes();
  String json;
  json.reserve(1800);
  json += "{\"allumee\":" + String(clim.power ? "true" : "false");
  json += ",\"consigne\":" + String(clim.degrees, 0);
  json += ",\"temperature\":" + nombreOuNull(temperature);
  json += ",\"humidite\":" + nombreOuNull(humidite);
  json += ",\"presence\":" + String(presence ? "true" : "false");
  json += ",\"sansPresence\":" + String(presence ? 0UL : (maintenant - dernierePresenceMs) / UNITE_MS);
  json += ",\"delai\":" + String(DELAI_SANS_PRESENCE_MIN);
  json += ",\"nuit\":" + String(estLaNuit(h, NUIT_DEBUT_H * 60, NUIT_FIN_H * 60) ? "true" : "false");
  json += ",\"nuitDebut\":" + String(NUIT_DEBUT_H) + ",\"nuitFin\":" + String(NUIT_FIN_H);
  json += ",\"eco\":" + String(CONSIGNE_MIN);
  json += ",\"allumage\":" + String(CONSIGNE_ALLUMAGE);
  json += ",\"alerte\":" + String(alerte ? "true" : "false");
  json += ",\"evite\":" + String((unsigned long)evite);
  json += ",\"puissanceClim\":" + String(PUISSANCE_CLIM_W);
  json += ",\"heure\":" + String(heureConnue() ? (unsigned long)time(nullptr) : 0UL);
  json += ",\"depuis\":" + String(maintenant / 1000);
  json += ",\"simulation\":" + String(MODE_SIMULATION ? "true" : "false");
  json += ",\"journal\":[";
  for (int i = 0; i < nbEvenements; i++) {  // du plus récent au plus ancien
    const Evenement& e = journal[(prochainEvenement - 1 - i + TAILLE_JOURNAL) % TAILLE_JOURNAL];
    if (i) json += ",";
    json += "{\"h\":" + String((unsigned long)e.heure) + ",\"d\":" + String(e.depuisS) + ",\"t\":\"" + e.texte + "\"}";
  }
  json += "]}";
  serveur.send(200, "application/json", json);
}

void recevoirCommande() {
  const String action = serveur.arg("action");
  if (action == "eteindre") {
    envoyer(eteinte());
    eteinteAutoMs = 0;  // éteinte volontairement : ce n'est pas du temps « évité » par le boîtier
    noter("Éteinte depuis le téléphone");
  } else if (action == "allumer") {
    envoyer(allumeeA(CONSIGNE_ALLUMAGE));
    noter("Allumée depuis le téléphone à %d °C", CONSIGNE_ALLUMAGE);
  } else {
    serveur.send(400, "text/plain", "action inconnue");
    return;
  }
  serveur.send(204);
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
  Serial.println(MODE_SIMULATION ? "Coupure clim (MODE SIMULATION : 1 minute = 1 seconde)" : "Coupure clim");

  // État de départ : la marque de la clim, en mode froid.
  clim.protocol = strToDecodeType(PROTOCOLE_CLIM);
  clim.mode = stdAc::opmode_t::kCool;
  clim.celsius = true;
  clim.degrees = CONSIGNE_ALLUMAGE;
  clim.fanspeed = stdAc::fanspeed_t::kAuto;
  clim.power = false;
  if (!IRac::isProtocolSupported(clim.protocol)) {
    Serial.println("Protocole de clim inconnu : vérifie PROTOCOLE_CLIM dans config.h (étape B du README).");
  }

  if (MODE_SIMULATION) {
    changerEtat(allumeeA(18));  // la clim simulée démarre allumée, réglée trop froid
  } else {
    pinMode(BROCHE_PRESENCE, INPUT);
    dht.begin();
    recepteur.enableIRIn();
  }
  dernierePresenceMs = millis();

  connecterWifi();
  serveur.on("/", []() { serveur.send_P(200, "text/html; charset=utf-8", PAGE_CLIM); });
  serveur.on("/etat", envoyerEtat);
  serveur.on("/commande", HTTP_POST, recevoirCommande);
  serveur.begin();
  noter("Boîtier démarré");
}

unsigned long derniereSeconde = 0, derniereTemperature = 0;

void loop() {
  serveur.handleClient();
  if (!MODE_SIMULATION) ecouterTelecommande();

  const unsigned long maintenant = millis();
  if (maintenant - derniereTemperature >= 5000 || derniereTemperature == 0) {
    derniereTemperature = maintenant;
    lireTemperature();
  }
  if (maintenant - derniereSeconde >= 1000) {
    derniereSeconde = maintenant;
    lirePresence();
    remiseAZeroAMinuit();
    appliquerRegles();
  }
}
