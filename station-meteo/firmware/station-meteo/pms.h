/*
  LIRE UNE TRAME DU CAPTEUR DE PARTICULES PMS5003 (sans bibliothèque : testé sur ordinateur)
  Le capteur envoie en continu des trames de 32 octets :
    42 4D | longueur (= 28) | 13 valeurs de 2 octets | somme de contrôle (2 octets)
  Les valeurs « atmosphériques » (pour l'air extérieur) sont aux octets 10 à 15 : PM1, PM2,5, PM10.
  On vérifie le début, la longueur ET la somme de contrôle : une trame abîmée est ignorée.
*/
#pragma once
#include <stdint.h>
#include <stddef.h>

struct MesurePM {
  uint16_t pm1, pm25, pm10;  // µg/m³
};

inline uint16_t mot(const uint8_t* t, size_t i) { return (uint16_t)((t[i] << 8) | t[i + 1]); }

inline bool lireTramePMS(const uint8_t* t, size_t n, MesurePM& m) {
  if (n < 32 || t[0] != 0x42 || t[1] != 0x4D) return false;
  if (mot(t, 2) != 28) return false;
  uint16_t somme = 0;
  for (size_t i = 0; i < 30; i++) somme += t[i];
  if (somme != mot(t, 30)) return false;
  m.pm1 = mot(t, 10);
  m.pm25 = mot(t, 12);
  m.pm10 = mot(t, 14);
  return true;
}
