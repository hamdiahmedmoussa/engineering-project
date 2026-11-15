// Tests de la lecture des commandes, sur ordinateur :
//   g++ -std=c++17 -o test firmware/tests/test_protocole.cpp && ./test
#include <cassert>
#include <cstdio>
#include "../bras-gestes/protocole.h"

int main() {
  Commande c = lireLigne("A 90 45 120 60");
  assert(c.type == TypeCommande::Angles);
  assert(c.angles[0] == 90 && c.angles[1] == 45 && c.angles[2] == 120 && c.angles[3] == 60);

  assert(lireLigne("A 90 45 120 60\r").type == TypeCommande::Angles);  // fin de ligne Windows
  assert(lireLigne("R").type == TypeCommande::Repos);
  assert(lireLigne("R\r").type == TypeCommande::Repos);

  // Refusées : hors limites, incomplètes, abîmées, inconnues.
  assert(lireLigne("A 180 45 120 60").type == TypeCommande::Refusee);  // base > 170
  assert(lireLigne("A 90 45 120 10").type == TypeCommande::Refusee);   // pince < 30
  assert(lireLigne("A 90 45 120").type == TypeCommande::Refusee);
  assert(lireLigne("A 90 45 120 60 99").type == TypeCommande::Refusee);
  assert(lireLigne("A 90 4x 120 60").type == TypeCommande::Refusee);
  assert(lireLigne("Repos").type == TypeCommande::Refusee);
  assert(lireLigne("").type == TypeCommande::Refusee);
  assert(lireLigne("B 90 45 120 60").type == TypeCommande::Refusee);

  std::puts("Tous les tests passent.");
  return 0;
}
