// Tests de la lecture des commandes, sur ordinateur :
//   g++ -std=c++17 -o test firmware/tests/test_protocole.cpp && ./test
#include <cassert>
#include <cstdio>
#include "../bras-gestes/protocole.h"

int main() {
  Commande c = lireLigne("A 90 45 120 100 30 40 50 60 70");
  assert(c.type == TypeCommande::Angles);
  const int attendu[NB_SERVOS] = {90, 45, 120, 100, 30, 40, 50, 60, 70};
  for (int i = 0; i < NB_SERVOS; i++) assert(c.angles[i] == attendu[i]);

  assert(lireLigne("A 90 90 90 90 20 20 20 20 20\r").type == TypeCommande::Angles);  // fin de ligne Windows
  assert(lireLigne("R").type == TypeCommande::Repos);
  assert(lireLigne("R\r").type == TypeCommande::Repos);

  // Refusées : hors limites, incomplètes, abîmées, inconnues.
  assert(lireLigne("A 180 90 90 90 20 20 20 20 20").type == TypeCommande::Refusee);  // base > 170
  assert(lireLigne("A 90 90 90 90 20 20 20 20 170").type == TypeCommande::Refusee);  // auriculaire > 160
  assert(lireLigne("A 90 90 90 90 10 20 20 20 20").type == TypeCommande::Refusee);   // pouce < 20
  assert(lireLigne("A 90 90 90 90 20 20 20 20").type == TypeCommande::Refusee);      // 8 angles
  assert(lireLigne("A 90 90 90 90 20 20 20 20 20 20").type == TypeCommande::Refusee); // 10 angles
  assert(lireLigne("A 90 9x 90 90 20 20 20 20 20").type == TypeCommande::Refusee);
  assert(lireLigne("A -90 90 90 90 20 20 20 20 20").type == TypeCommande::Refusee);
  assert(lireLigne("A90 90 90 90 20 20 20 20 20").type == TypeCommande::Refusee);
  assert(lireLigne("Repos").type == TypeCommande::Refusee);
  assert(lireLigne("").type == TypeCommande::Refusee);

  std::puts("Tous les tests passent.");
  return 0;
}
