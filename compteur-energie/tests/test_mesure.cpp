// Tests des calculs, sur ordinateur (pas besoin d'ESP32) :
//   g++ -std=c++17 -o test tests/test_mesure.cpp && ./test
#include <cassert>
#include <cmath>
#include <cstdio>
#include "../firmware/compteur-energie/mesure.h"

static bool proche(double a, double b, double tolerance) { return std::fabs(a - b) <= tolerance; }

int main() {
  const double PI = 3.14159265358979;

  // Une sinusoïde de 50 Hz, décalée de 1,65 V, échantillonnée sur 10 périodes (200 ms).
  // Amplitude 0,707 V → valeur efficace 0,5 V → 15 A avec une pince 30 A / 1 V.
  const int n = 2000;
  static float signal[n];
  for (int i = 0; i < n; i++) {
    const double t = i * 0.2 / n;
    signal[i] = 1.65 + 0.7071 * std::sin(2 * PI * 50 * t);
  }
  const double vrms = valeurEfficace(signal, n);
  assert(proche(vrms, 0.5, 0.001));                                 // le décalage de 1,65 V est bien retiré
  assert(proche(courantEfficace(vrms, 30, 1.0, 0.1), 15.0, 0.05));  // 0,5 V × 30 A/V = 15 A
  assert(proche(courantEfficace(vrms, 30, 1.1, 0.1), 16.5, 0.05));  // la calibration s'applique

  // Un signal plat (aucun appareil branché) : 0 A, même avec un peu de bruit sous le seuil.
  static float plat[n];
  for (int i = 0; i < n; i++) plat[i] = 1.65 + ((i % 2) ? 0.001 : -0.001);
  assert(courantEfficace(valeurEfficace(plat, n), 30, 1.0, 0.1) == 0);

  // Énergie : 1000 W pendant 1 heure = 1 kWh ; 2000 W pendant 30 min = 1 kWh.
  assert(proche(energieKWh(1000, 3600), 1.0, 1e-9));
  assert(proche(energieKWh(2000, 1800), 1.0, 1e-9));

  assert(valeurEfficace(signal, 0) == 0);  // aucun échantillon : pas de division par zéro

  std::puts("Tous les tests passent.");
  return 0;
}
