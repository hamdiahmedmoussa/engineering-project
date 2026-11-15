/*
  LIRE ET VÉRIFIER UNE LIGNE REÇUE PAR USB (sans rien qui dépende de l'ESP32 : testé sur ordinateur)
    "A 90 45 120 60"  → 4 angles (base, épaule, coude, pince)
    "R"               → position de repos
  Tout le reste est REFUSÉ, y compris un angle hors limites : le bras ne bouge jamais sur un
  message abîmé (câble, bug, mauvais programme qui envoie n'importe quoi).
*/
#pragma once
#include <stdio.h>
#include <ctype.h>
#include "limites.h"

enum class TypeCommande { Refusee, Angles, Repos };

struct Commande {
  TypeCommande type;
  int angles[NB_SERVOS];
};

inline Commande lireLigne(const char* ligne) {
  Commande c{TypeCommande::Refusee, {0, 0, 0, 0}};
  while (*ligne == ' ') ligne++;

  if (ligne[0] == 'R') {
    for (const char* p = ligne + 1; *p; p++) if (!isspace((unsigned char)*p)) return c;
    c.type = TypeCommande::Repos;
    return c;
  }

  int lus = 0;
  // %n : nombre de caractères lus, pour vérifier qu'il n'y a rien d'autre après les 4 angles.
  if (sscanf(ligne, "A %d %d %d %d%n", &c.angles[0], &c.angles[1], &c.angles[2], &c.angles[3], &lus) != 4) return c;
  for (const char* p = ligne + lus; *p; p++) if (!isspace((unsigned char)*p)) return c;
  for (int i = 0; i < NB_SERVOS; i++) {
    if (c.angles[i] < LIMITE_MIN[i] || c.angles[i] > LIMITE_MAX[i]) return c;
  }
  c.type = TypeCommande::Angles;
  return c;
}
