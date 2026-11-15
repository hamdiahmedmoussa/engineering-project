/*
  LES CALCULS SOLAIRES (sans rien qui dépende de l'ESP32)
  Testés sur ordinateur : voir tests/test_solaire.cpp.

  À retenir : un panneau solaire produit MOINS quand il chauffe. Sa fiche technique donne
  un « coefficient de température » (souvent autour de -0,4 % par °C au-dessus de 25 °C).
  À Djibouti, un panneau au soleil dépasse facilement 60 °C : il perd alors environ 15 %.
*/
#pragma once

/** Énergie (Wh) produite par une puissance (W) pendant une durée (s). */
inline double energieWh(double puissanceW, double dureeS) {
  return puissanceW * dureeS / 3600.0;
}

/**
 * Pourcentage de production perdu à cause de la chaleur, par rapport à 25 °C.
 * coefPctParDegre est négatif (ex. -0.40). Un panneau plus froid que 25 °C ne « perd » rien : 0.
 */
inline double perteChaleurPct(double temperatureC, double coefPctParDegre) {
  const double perte = -coefPctParDegre * (temperatureC - 25.0);
  return perte > 0 ? perte : 0;
}

/**
 * Ce que le panneau produirait s'il était à 25 °C, avec le même soleil.
 * Utile pour comparer deux jours (ou avant / après nettoyage) sans que la chaleur fausse tout.
 */
inline double puissanceA25(double puissanceW, double temperatureC, double coefPctParDegre) {
  const double facteur = 1.0 + coefPctParDegre / 100.0 * (temperatureC - 25.0);
  return facteur > 0.05 ? puissanceW / facteur : puissanceW;  // garde-fou contre une division absurde
}

/** Production en pourcentage de la puissance annoncée sur l'étiquette du panneau. */
inline double pourcentageNominal(double puissanceW, double puissanceNominaleW) {
  return puissanceNominaleW > 0 ? puissanceW / puissanceNominaleW * 100.0 : 0;
}
