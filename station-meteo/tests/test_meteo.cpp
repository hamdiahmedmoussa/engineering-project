// Tests des calculs, sur ordinateur :
//   g++ -std=c++17 -o test tests/test_meteo.cpp && ./test
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "../firmware/station-meteo/meteo.h"
#include "../firmware/station-meteo/pms.h"

static bool proche(double a, double b, double t) { return std::fabs(a - b) <= t; }

int main() {
  // Ressenti : valeurs du tableau officiel de la NOAA.
  assert(proche(celsiusVersF(ressenti(fVersCelsius(90), 70)), 106, 1));  // 90 °F à 70 % → 106 °F
  assert(proche(celsiusVersF(ressenti(fVersCelsius(96), 50)), 108, 1));  // 96 °F à 50 % → 108 °F
  assert(proche(celsiusVersF(ressenti(fVersCelsius(84), 40)), 84, 1.5)); // air sec : presque la température
  assert(ressenti(22, 90) == 22);                                         // en dessous de 27 °C : inchangé
  // Un après-midi humide à Djibouti : 34 °C et 60 % → environ 42 °C ressentis.
  assert(proche(ressenti(34, 60), 42, 1.5));

  assert(niveauChaleur(25) == 0 && niveauChaleur(30) == 1 && niveauChaleur(38) == 2);
  assert(niveauChaleur(45) == 3 && niveauChaleur(60) == 4);

  assert(niveauPM25(5) == 0 && niveauPM25(9.0) == 0 && niveauPM25(9.1) == 1);
  assert(niveauPM25(40) == 2 && niveauPM25(100) == 3 && niveauPM25(200) == 4 && niveauPM25(300) == 5);
  assert(niveauPM10(30) == 0 && niveauPM10(100) == 1 && niveauPM10(500) == 5);

  // Une trame PMS5003 correcte : PM1 = 12, PM2,5 = 25, PM10 = 160 (une journée de poussière).
  uint8_t t[32] = {0x42, 0x4D, 0, 28};
  t[10] = 0; t[11] = 12; t[12] = 0; t[13] = 25; t[14] = 0; t[15] = 160;
  uint16_t somme = 0;
  for (int i = 0; i < 30; i++) somme += t[i];
  t[30] = somme >> 8; t[31] = somme & 0xFF;
  MesurePM m{};
  assert(lireTramePMS(t, 32, m) && m.pm1 == 12 && m.pm25 == 25 && m.pm10 == 160);

  uint8_t abimee[32];
  std::memcpy(abimee, t, 32);
  abimee[13] = 99;                       // un octet changé : la somme de contrôle ne correspond plus
  assert(!lireTramePMS(abimee, 32, m));
  std::memcpy(abimee, t, 32);
  abimee[0] = 0x00;                      // mauvais début
  assert(!lireTramePMS(abimee, 32, m));
  assert(!lireTramePMS(t, 20, m));       // trame incomplète

  std::puts("Tous les tests passent.");
  return 0;
}
