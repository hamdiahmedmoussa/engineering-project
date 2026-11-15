/*
  RÉGLAGES DU COMPTEUR
  1. Copie ce fichier et renomme la copie « config.h » (dans le même dossier).
  2. Mets-y le nom et le mot de passe de ton Wi-Fi.
  config.h n'est jamais envoyé sur GitHub (voir .gitignore) : ton mot de passe reste chez toi.
*/
#pragma once

// --- Wi-Fi de la maison. Si la connexion échoue, le compteur crée son propre réseau (voir README). ---
#define WIFI_NOM "nom-de-ton-wifi"
#define WIFI_MOT_DE_PASSE "mot-de-passe-du-wifi"

// --- Simulation : true = pas besoin de pince, l'ESP32 invente une clim qui s'allume et s'éteint.
//     Pratique pour tester le tableau de bord avant d'avoir le capteur. Mets false avec la vraie pince. ---
#define MODE_SIMULATION true

// --- Mesure ---
#define BROCHE_PINCE 34          // entrée analogique (GPIO34 : entrée seule, idéale pour mesurer)
#define AMPERE_PAR_VOLT 30.0     // SCT-013-030 : 1 V en sortie = 30 A dans le câble
#define CALIBRATION 1.0          // à ajuster en comparant avec une pince multimètre (voir README)
#define SEUIL_BRUIT_A 0.08       // en dessous, on affiche 0 A
#define TENSION_SECTEUR 220.0    // tension nominale à Djibouti (V). La pince ne mesure pas la tension.

// --- Réseau créé par le compteur quand le Wi-Fi de la maison est introuvable (8 caractères minimum). ---
#define RESEAU_COMPTEUR_NOM "Compteur-Energie"
#define RESEAU_COMPTEUR_MOT_DE_PASSE "change-moi-2026"
