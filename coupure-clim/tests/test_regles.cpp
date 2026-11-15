// Tests des règles, sur ordinateur :
//   g++ -std=c++17 -o test tests/test_regles.cpp && ./test
#include <cassert>
#include <cmath>
#include <cstdio>
#include "../firmware/coupure-clim/regles.h"

static const Reglages R{15, 22 * 60, 7 * 60, 24, 32.0f, 45};

// Une situation « normale » : clim allumée à 25 °C, quelqu'un présent, 14 h, 27 °C dans la pièce.
static Situation normale() { return Situation{true, 25, true, 0, 60, 14 * 60, 27.0f, false}; }

int main() {
  // Rien à faire quand tout va bien.
  Decision d = decider(normale(), R);
  assert(d.action == Action::Rien && !d.alerteSurchauffe);

  // Clim éteinte : jamais d'action.
  Situation s = normale(); s.climAllumee = false; s.presence = false; s.minutesSansPresence = 99;
  assert(decider(s, R).action == Action::Rien);

  // Pièce vide 14 min : on attend encore. 15 min : on éteint.
  s = normale(); s.presence = false; s.minutesSansPresence = 14;
  assert(decider(s, R).action == Action::Rien);
  s.minutesSansPresence = 15;
  assert(decider(s, R).action == Action::Eteindre);

  // La nuit (23 h et 3 h), on n'éteint jamais automatiquement, même pièce « vide ».
  s.heureMin = 23 * 60;
  assert(decider(s, R).action != Action::Eteindre);
  s.heureMin = 3 * 60;
  assert(decider(s, R).action != Action::Eteindre);
  s.heureMin = 7 * 60;  // 7 h pile : la nuit est finie
  assert(decider(s, R).action == Action::Eteindre);

  // Plage de nuit qui ne passe pas minuit (ex. sieste 13 h → 16 h).
  assert(estLaNuit(14 * 60, 13 * 60, 16 * 60) && !estLaNuit(17 * 60, 13 * 60, 16 * 60));
  assert(!estLaNuit(-1, 22 * 60, 7 * 60));  // heure inconnue

  // Mode éco : réglée à 18 °C → remonter, une seule fois.
  s = normale(); s.consigne = 18;
  assert(decider(s, R).action == Action::RemonterConsigne);
  s.ecoDejaApplique = true;  // quelqu'un l'a rebaissée exprès : on respecte
  assert(decider(s, R).action == Action::Rien);
  Reglages sansEco = R; sansEco.consigneMin = 0;
  s.ecoDejaApplique = false;
  assert(decider(s, sansEco).action == Action::Rien);

  // Éteindre passe avant le mode éco.
  s = normale(); s.consigne = 18; s.presence = false; s.minutesSansPresence = 30;
  assert(decider(s, R).action == Action::Eteindre);

  // Surchauffe : 33 °C après 60 min de clim → alerte. Pas avant 45 min, ni sans capteur.
  s = normale(); s.temperaturePiece = 33;
  assert(decider(s, R).alerteSurchauffe);
  s.minutesDepuisAllumage = 20;
  assert(!decider(s, R).alerteSurchauffe);
  s.minutesDepuisAllumage = 60; s.temperaturePiece = NAN;
  assert(!decider(s, R).alerteSurchauffe);

  std::puts("Tous les tests passent.");
  return 0;
}
