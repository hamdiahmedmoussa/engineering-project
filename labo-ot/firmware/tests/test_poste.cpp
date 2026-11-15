// Tests de l'automate ESP32, sur ordinateur :
//   g++ -std=c++17 -o test firmware/tests/test_poste.cpp && ./test
#include <cassert>
#include <cstdio>
#include <vector>
#include "../automate-esp32/poste.h"

using Octets = std::vector<uint8_t>;

static Octets trame(std::vector<uint8_t> pdu) {
  Octets t = {0x00, 0x07, 0x00, 0x00, 0x00, (uint8_t)(pdu.size() + 1), 0x01};
  t.insert(t.end(), pdu.begin(), pdu.end());
  return t;
}
static Octets envoyer(Poste& p, const Octets& t) {
  uint8_t r[260];
  const size_t n = traiterTrame(p, t.data(), t.size(), r);
  return Octets(r, r + n);
}

int main() {
  Poste p;
  // Lire les 3 seuils : 2000, 2800, 3500
  Octets r = envoyer(p, trame({3, 0, 0, 0, 3}));
  assert((r == Octets{0, 7, 0, 0, 0, 9, 1, 3, 6, 0x07, 0xD0, 0x0A, 0xF0, 0x0D, 0xAC}));
  // Lire les disjoncteurs : 3 bits à 1
  assert((envoyer(p, trame({1, 0, 0, 0, 3})) == Octets{0, 7, 0, 0, 0, 4, 1, 1, 1, 0b111}));
  // Ouvrir le disjoncteur 1 (fonction 5) : écho, et la mémoire change
  r = envoyer(p, trame({5, 0, 1, 0x00, 0x00}));
  assert(r.size() == 12 && r[7] == 5 && p.bobines[1] == 0);
  // Le sabotage de la démonstration : 3 seuils à 1 A (fonction 16) — l'automate obéit, c'est la faille
  envoyer(p, trame({16, 0, 0, 0, 3, 6, 0, 10, 0, 10, 0, 10}));
  assert(p.registres[0] == 10 && p.registres[2] == 10);
  // Ouvrir les 3 d'un coup (fonction 15)
  envoyer(p, trame({15, 0, 0, 0, 3, 1, 0b000}));
  assert(p.bobines[0] == 0 && p.bobines[2] == 0);

  // Exceptions : adresse inexistante (2), valeur de bobine invalide (3), fonction inconnue (1)
  assert((envoyer(p, trame({3, 0, 2, 0, 5})) == Octets{0, 7, 0, 0, 0, 3, 1, 0x83, 2}));
  assert(envoyer(p, trame({5, 0, 0, 0x12, 0x34})).back() == 3);
  assert(envoyer(p, trame({43, 14, 1, 0})).back() == 1);
  // Trames illisibles : pas de réponse (la connexion sera fermée)
  Octets mauvaise = trame({3, 0, 0, 0, 3});
  mauvaise[5] = 99;
  assert(envoyer(p, mauvaise).empty());
  mauvaise = trame({3, 0, 0, 0, 3});
  mauvaise[3] = 1;  // protocole ≠ 0
  assert(envoyer(p, mauvaise).empty());

  std::puts("Tous les tests passent.");
  return 0;
}
