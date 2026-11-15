// Tests des vérifications de mots de passe, sur ordinateur :
//   g++ -std=c++17 -o test commun/test_securite.cpp && ./test
#include <cassert>
#include <cstdio>
#include "securite.h"

int main() {
  static_assert(textesEgaux("change-moi", "change-moi"), "");
  static_assert(!textesEgaux("change-moi", "change-mo"), "");
  static_assert(!textesEgaux("abc", "abd"), "");
  static_assert(longueurTexte("") == 0 && longueurTexte("12345678") == 8, "");
  std::puts("Tous les tests passent.");
  return 0;
}
