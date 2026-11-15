/*
  LIRE ET VÉRIFIER UNE LIGNE REÇUE PAR USB (sans rien qui dépende de l'ESP32 : testé sur ordinateur)
    "A 90 90 90 90 20 20 20 20 20" → 9 angles (voir l'ordre dans limites.h)
    "R"                            → position de repos
  Tout le reste est REFUSÉ, y compris un angle hors limites : le bras ne bouge jamais sur un
  message abîmé (câble, bug, mauvais programme qui envoie n'importe quoi).
*/
#pragma once
#include <ctype.h>
#include <stdlib.h>
#include "limites.h"

enum class TypeCommande { Refusee, Angles, Repos };

struct Commande {
  TypeCommande type;
  int angles[NB_SERVOS];
};

/** Vrai s'il ne reste que des espaces (ou \r) à partir de p. */
inline bool finDeLigne(const char* p) {
  for (; *p; p++) if (!isspace((unsigned char)*p)) return false;
  return true;
}

inline Commande lireLigne(const char* ligne) {
  Commande c{TypeCommande::Refusee, {}};
  while (*ligne == ' ') ligne++;

  if (ligne[0] == 'R') {
    if (finDeLigne(ligne + 1)) c.type = TypeCommande::Repos;
    return c;
  }
  if (ligne[0] != 'A' || ligne[1] != ' ') return c;

  const char* p = ligne + 2;
  for (int i = 0; i < NB_SERVOS; i++) {
    while (*p == ' ') p++;
    if (!isdigit((unsigned char)*p)) return c;      // pas de signe moins, pas de lettre
    char* fin;
    const long v = strtol(p, &fin, 10);
    if (*fin != ' ' && *fin != '\0' && *fin != '\r' && *fin != '\n') return c;  // ex. « 4x »
    if (v < LIMITE_MIN[i] || v > LIMITE_MAX[i]) return c;
    c.angles[i] = (int)v;
    p = fin;
  }
  if (!finDeLigne(p)) return c;  // trop de nombres
  c.type = TypeCommande::Angles;
  return c;
}
