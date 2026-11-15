// Tests des calculs solaires, sur ordinateur :
//   g++ -std=c++17 -o test tests/test_solaire.cpp && ./test
#include <cassert>
#include <cmath>
#include <cstdio>
#include "../firmware/suivi-solaire/solaire.h"

static bool proche(double a, double b, double t) { return std::fabs(a - b) <= t; }

int main() {
  // 20 W pendant 1 h = 20 Wh ; 10 W pendant 30 min = 5 Wh.
  assert(proche(energieWh(20, 3600), 20, 1e-9));
  assert(proche(energieWh(10, 1800), 5, 1e-9));

  // Coefficient -0,4 %/°C : à 65 °C (40 °C de plus que 25), la perte est de 16 %.
  assert(proche(perteChaleurPct(65, -0.40), 16, 1e-9));
  assert(perteChaleurPct(25, -0.40) == 0);
  assert(perteChaleurPct(15, -0.40) == 0);  // plus froid que 25 °C : pas de « perte »

  // À 65 °C le panneau donne 16,8 W ; à 25 °C il aurait donné 16,8 / 0,84 = 20 W.
  assert(proche(puissanceA25(16.8, 65, -0.40), 20, 1e-9));
  assert(proche(puissanceA25(20, 25, -0.40), 20, 1e-9));

  assert(proche(pourcentageNominal(17, 20), 85, 1e-9));
  assert(pourcentageNominal(17, 0) == 0);  // pas de division par zéro si la config est vide

  std::puts("Tous les tests passent.");
  return 0;
}
