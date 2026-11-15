/*
  LE POSTE ÉLECTRIQUE ET SES RÉPONSES MODBUS (sans rien qui dépende de l'ESP32 : testé sur ordinateur)
  Même mémoire que automate.py :
    bobines 0-2    disjoncteurs (1 = fermé)        registres 0-2  seuils de protection (dixièmes d'A)
    mesures 0-2    courants (dixièmes d'A)          mesures 3, 4   tension (dixièmes de V), fréquence (centièmes de Hz)
  Comme un vrai automate Modbus, il obéit à QUICONQUE : c'est la cible du laboratoire.
*/
#pragma once
#include <stdint.h>
#include <stddef.h>

struct Poste {
  uint8_t bobines[3] = {1, 1, 1};
  uint16_t registres[3] = {2000, 2800, 3500};
  uint16_t mesures[5] = {0, 0, 0, 2300, 5000};
};

inline uint16_t lire16(const uint8_t* t) { return (uint16_t)((t[0] << 8) | t[1]); }
inline void ecrire16(uint8_t* t, uint16_t v) { t[0] = v >> 8; t[1] = v & 0xFF; }

/** Réponse d'exception : fonction + 0x80, puis le code (1 fonction, 2 adresse, 3 valeur). */
inline size_t exception(const uint8_t* t, uint8_t* r, uint8_t code) {
  for (int i = 0; i < 4; i++) r[i] = t[i];       // transaction et protocole recopiés
  ecrire16(r + 4, 3);
  r[6] = t[6];
  r[7] = t[7] | 0x80;
  r[8] = code;
  return 9;
}

/**
 * Traite UNE trame Modbus TCP complète (t, n octets) et écrit la réponse dans r (au moins 260 octets).
 * Renvoie la taille de la réponse, ou 0 si la trame est illisible (la connexion doit alors être fermée).
 */
inline size_t traiterTrame(Poste& p, const uint8_t* t, size_t n, uint8_t* r) {
  if (n < 8 || lire16(t + 2) != 0 || lire16(t + 4) != n - 6) return 0;
  const uint8_t f = t[7];
  const uint8_t* d = t + 8;
  const size_t nd = n - 8;
  for (int i = 0; i < 8; i++) r[i] = t[i];  // en-tête et fonction recopiés ; la longueur est corrigée à la fin
  size_t taille = 8;

  if (f == 1 || f == 3 || f == 4) {  // lectures
    if (nd != 4) return exception(t, r, 3);
    const uint16_t a = lire16(d), q = lire16(d + 2);
    const size_t max = f == 1 ? 3 : (f == 3 ? 3 : 5);
    if (q == 0 || q > 125) return exception(t, r, 3);
    if (a + q > max) return exception(t, r, 2);
    if (f == 1) {
      uint8_t bits = 0;
      for (int i = 0; i < q; i++) bits |= p.bobines[a + i] << i;
      r[taille++] = 1;
      r[taille++] = bits;
    } else {
      const uint16_t* source = f == 3 ? p.registres : p.mesures;
      r[taille++] = 2 * q;
      for (int i = 0; i < q; i++, taille += 2) ecrire16(r + taille, source[a + i]);
    }
  } else if (f == 5 || f == 6) {  // écrire une bobine / un registre
    if (nd != 4) return exception(t, r, 3);
    const uint16_t a = lire16(d), v = lire16(d + 2);
    if (a >= 3) return exception(t, r, 2);
    if (f == 5) {
      if (v != 0xFF00 && v != 0) return exception(t, r, 3);
      p.bobines[a] = v ? 1 : 0;
    } else {
      p.registres[a] = v;
    }
    for (int i = 0; i < 4; i++) r[taille++] = d[i];  // réponse = écho
  } else if (f == 15 || f == 16) {  // écrire plusieurs bobines / registres
    if (nd < 5) return exception(t, r, 3);
    const uint16_t a = lire16(d), q = lire16(d + 2);
    const uint8_t nb = d[4];
    const size_t attendu = f == 15 ? (q + 7) / 8 : 2 * q;
    if (q == 0 || nb != attendu || nd != 5u + nb) return exception(t, r, 3);
    if (a + q > 3) return exception(t, r, 2);
    for (int i = 0; i < q; i++) {
      if (f == 15) p.bobines[a + i] = (d[5 + i / 8] >> (i % 8)) & 1;
      else p.registres[a + i] = lire16(d + 5 + 2 * i);
    }
    for (int i = 0; i < 4; i++) r[taille++] = d[i];  // réponse : adresse et quantité
  } else {
    return exception(t, r, 1);
  }
  ecrire16(r + 4, taille - 6);
  return taille;
}
