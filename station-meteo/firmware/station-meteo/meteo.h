/*
  LES CALCULS MÉTÉO ET QUALITÉ DE L'AIR (sans rien qui dépende de l'ESP32 : testés sur ordinateur)

  1. Température ressentie (« indice de chaleur ») : quand il fait chaud ET humide, la sueur s'évapore mal,
     le corps se refroidit moins bien, et on a plus chaud. Formule du service météo américain (NOAA).
  2. Niveaux de danger de la chaleur ressentie (mêmes seuils que la NOAA).
  3. Niveaux de qualité de l'air pour les particules PM2,5 et PM10 (seuils de l'agence américaine EPA, 2024).
*/
#pragma once
#include <math.h>

inline double celsiusVersF(double c) { return c * 9.0 / 5.0 + 32.0; }
inline double fVersCelsius(double f) { return (f - 32.0) * 5.0 / 9.0; }

/** Température ressentie (°C). En dessous de 27 °C, l'humidité change peu de chose : on renvoie la température. */
inline double ressenti(double temperatureC, double humidite) {
  if (temperatureC < 27.0) return temperatureC;
  const double T = celsiusVersF(temperatureC), R = humidite;
  double hi = 0.5 * (T + 61.0 + (T - 68.0) * 1.2 + R * 0.094);  // formule simple
  if ((hi + T) / 2.0 >= 80.0) {                                // il fait vraiment chaud : formule complète
    hi = -42.379 + 2.04901523 * T + 10.14333127 * R - 0.22475541 * T * R - 0.00683783 * T * T
         - 0.05481717 * R * R + 0.00122874 * T * T * R + 0.00085282 * T * R * R - 0.00000199 * T * T * R * R;
    if (R < 13 && T >= 80 && T <= 112) hi -= ((13 - R) / 4) * sqrt((17 - fabs(T - 95)) / 17);
    else if (R > 85 && T >= 80 && T <= 87) hi += ((R - 85) / 10) * ((87 - T) / 5);
  }
  return fVersCelsius(hi);
}

/** 0 normal · 1 prudence · 2 grande prudence · 3 danger · 4 danger extrême */
inline int niveauChaleur(double ressentiC) {
  if (ressentiC < 27) return 0;
  if (ressentiC < 32) return 1;
  if (ressentiC < 41) return 2;
  if (ressentiC < 54) return 3;
  return 4;
}

/** PM2,5 (µg/m³) → 0 bon · 1 moyen · 2 mauvais pour les personnes sensibles · 3 mauvais · 4 très mauvais · 5 dangereux */
inline int niveauPM25(double v) {
  if (v <= 9.0) return 0;
  if (v <= 35.4) return 1;
  if (v <= 55.4) return 2;
  if (v <= 125.4) return 3;
  if (v <= 225.4) return 4;
  return 5;
}

/** PM10 (µg/m³), mêmes niveaux. La poussière de sable est surtout du PM10. */
inline int niveauPM10(double v) {
  if (v <= 54) return 0;
  if (v <= 154) return 1;
  if (v <= 254) return 2;
  if (v <= 354) return 3;
  if (v <= 424) return 4;
  return 5;
}
