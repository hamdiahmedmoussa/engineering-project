/*
  RÉGLAGES DU SUIVI SOLAIRE
  1. Copie ce fichier et renomme la copie « config.h » (dans le même dossier).
  2. Remplis le Wi-Fi et les caractéristiques de TON panneau (étiquette au dos).
  config.h n'est jamais envoyé sur GitHub : ton mot de passe reste chez toi.
*/
#pragma once

// --- Wi-Fi de la maison (avec internet : l'ESP32 récupère alors l'heure exacte) ---
#define WIFI_NOM "nom-de-ton-wifi"
#define WIFI_MOT_DE_PASSE "mot-de-passe-du-wifi"

// --- Réseau créé par le boîtier si le Wi-Fi est introuvable (8 caractères minimum, à CHANGER : obligatoire) ---
#define RESEAU_BOITIER_NOM "Suivi-Solaire"
#define RESEAU_BOITIER_MOT_DE_PASSE "change-moi-2026"

// --- Simulation : true = pas besoin de panneau ni de capteurs, l'ESP32 invente une journée de soleil.
//     Mets false quand tout est branché. ---
#define MODE_SIMULATION true

// --- Ton panneau (étiquette au dos) ---
#define PUISSANCE_NOMINALE_W 20.0     // « Pmax » ou « Rated power »
#define COEF_TEMPERATURE -0.40        // « Temperature coefficient of Pmax », en % par °C (négatif)

// --- Branchements ---
#define BROCHE_TEMPERATURE 4          // sonde DS18B20 (avec une résistance de 4,7 kΩ vers 3,3 V)
// L'INA219 utilise l'I2C : SDA = GPIO21, SCL = GPIO22 (valeurs par défaut de l'ESP32)

// --- Fuseau horaire de Djibouti : UTC+3, sans heure d'été ---
#define DECALAGE_UTC_S (3 * 3600)

// --- Accès au tableau de bord : OBLIGATOIRE. Le programme REFUSE de compiler avec les valeurs
//     d'exemple ci-dessous (elles sont publiques sur GitHub). 10 caractères minimum. ---
#define UTILISATEUR_TABLEAU "hamdi"
#define MOT_DE_PASSE_TABLEAU "change-moi"
