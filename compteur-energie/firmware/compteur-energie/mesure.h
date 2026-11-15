/*
  LES CALCULS ÉLECTRIQUES (sans rien qui dépende de l'ESP32)
  Ce fichier est testé sur ordinateur : voir tests/test_mesure.cpp.

  Rappel : la pince SCT-013-030 donne une TENSION alternative proportionnelle au courant
  (1 V efficace pour 30 A efficaces). On la décale à +1,65 V pour que l'ESP32, qui ne lit
  que des tensions positives, puisse la mesurer. On retire donc d'abord ce décalage
  (la moyenne), puis on calcule la valeur efficace (RMS).
*/
#pragma once
#include <math.h>
#include <stddef.h>

/**
 * Valeur efficace (RMS) d'un signal, après avoir retiré sa moyenne (le décalage de 1,65 V).
 * RMS = racine( moyenne( (x - moyenne)² ) )
 * Les échantillons doivent couvrir un nombre ENTIER de périodes (ex. 10 périodes à 50 Hz = 200 ms).
 */
inline double valeurEfficace(const float* echantillons, size_t n) {
  if (n == 0) return 0;
  double somme = 0;
  for (size_t i = 0; i < n; i++) somme += echantillons[i];
  const double moyenne = somme / n;

  double carres = 0;
  for (size_t i = 0; i < n; i++) {
    const double ecart = echantillons[i] - moyenne;
    carres += ecart * ecart;
  }
  return sqrt(carres / n);
}

/**
 * Courant efficace (A) à partir de la tension efficace de la pince (V).
 * ampereParVolt : 30 pour une SCT-013-030. calibration : 1.0, puis ajusté avec un multimètre.
 * seuil : en dessous, on affiche 0 (le bruit électrique donne toujours quelques dizaines de mA).
 */
inline double courantEfficace(double tensionPinceV, double ampereParVolt, double calibration, double seuil) {
  const double courant = tensionPinceV * ampereParVolt * calibration;
  return courant < seuil ? 0 : courant;
}

/** Énergie (kWh) consommée par une puissance (W) pendant une durée (s). 1 kWh = 3 600 000 J. */
inline double energieKWh(double puissanceW, double dureeS) {
  return puissanceW * dureeS / 3600000.0;
}
