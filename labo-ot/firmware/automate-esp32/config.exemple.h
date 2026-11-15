/*
  RÉGLAGES DE L'AUTOMATE DU LABORATOIRE
  Copie ce fichier en « config.h » (même dossier) et change le mot de passe.
  config.h n'est jamais envoyé sur GitHub.
*/
#pragma once

// L'ESP32 crée SON PROPRE réseau Wi-Fi : l'automate volontairement vulnérable n'est jamais
// sur le Wi-Fi de la maison. Connecte le PC du laboratoire à ce réseau.
#define RESEAU_LABO_NOM "Labo-OT"
#define RESEAU_LABO_MOT_DE_PASSE "change-moi-2026"   // 8 caractères minimum, à CHANGER (obligatoire)

// Les 3 « disjoncteurs » : une LED par départ (allumée = départ alimenté), avec une résistance de 220 Ω.
const int LED_DEPARTS[3] = {25, 26, 27};
