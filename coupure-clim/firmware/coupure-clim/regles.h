/*
  LES RÈGLES DE DÉCISION (sans rien qui dépende de l'ESP32)
  Testées sur ordinateur : voir tests/test_regles.cpp.

  À chaque instant, on regarde la situation et on décide :
    1. La clim est éteinte → rien à faire.
    2. Personne dans la pièce depuis assez longtemps → l'éteindre…
       … SAUF la nuit : quelqu'un qui dort bouge très peu, on ne prend aucun risque.
    3. Réglée trop froid (ex. 18 °C) → remonter la consigne une fois (mode éco).
       Si quelqu'un la rebaisse ensuite avec la télécommande, on respecte son choix.
    4. Allumée depuis longtemps mais la pièce reste très chaude → alerte (filtre sale, gaz, panne ?).
*/
#pragma once
#include <math.h>

struct Reglages {
  int delaiSansPresenceMin;   // pièce vide depuis ce temps → on éteint
  int nuitDebutMin;           // début de la nuit, en minutes depuis minuit (22 h = 1320)
  int nuitFinMin;             // fin de la nuit (7 h = 420)
  int consigneMin;            // consigne minimale en mode éco (°C) ; 0 = mode éco désactivé
  float seuilSurchauffeC;     // au-dessus, alerte…
  int delaiSurchauffeMin;     // … si la clim tourne depuis au moins ce temps
};

struct Situation {
  bool climAllumee;
  int consigne;               // °C demandés à la clim
  bool presence;              // quelqu'un est détecté en ce moment
  int minutesSansPresence;
  int minutesDepuisAllumage;
  int heureMin;               // minutes depuis minuit, ou -1 si l'heure est inconnue
  float temperaturePiece;     // °C, ou NAN si le capteur ne répond pas
  bool ecoDejaApplique;       // la consigne a déjà été remontée depuis l'allumage
};

enum class Action { Rien, Eteindre, RemonterConsigne };

struct Decision {
  Action action;
  bool alerteSurchauffe;
};

/** Est-on dans la plage de nuit ? Gère une nuit qui passe minuit (22 h → 7 h). */
inline bool estLaNuit(int heureMin, int debut, int fin) {
  if (heureMin < 0) return false;  // heure inconnue
  if (debut < fin) return heureMin >= debut && heureMin < fin;
  return heureMin >= debut || heureMin < fin;
}

inline Decision decider(const Situation& s, const Reglages& r) {
  Decision d{Action::Rien, false};
  if (!s.climAllumee) return d;

  d.alerteSurchauffe = s.minutesDepuisAllumage >= r.delaiSurchauffeMin && !isnan(s.temperaturePiece) &&
                       s.temperaturePiece >= r.seuilSurchauffeC;

  const bool pieceVide = !s.presence && s.minutesSansPresence >= r.delaiSansPresenceMin;
  if (pieceVide && !estLaNuit(s.heureMin, r.nuitDebutMin, r.nuitFinMin)) {
    d.action = Action::Eteindre;
    return d;
  }
  if (r.consigneMin > 0 && s.consigne < r.consigneMin && !s.ecoDejaApplique) d.action = Action::RemonterConsigne;
  return d;
}
