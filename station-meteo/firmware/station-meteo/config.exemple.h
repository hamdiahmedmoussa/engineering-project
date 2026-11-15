/*
  RÉGLAGES DE LA STATION MÉTÉO
  1. Copie ce fichier et renomme la copie « config.h » (dans le même dossier).
  2. Remplis le Wi-Fi.
  config.h n'est jamais envoyé sur GitHub : ton mot de passe reste chez toi.
*/
#pragma once

#define WIFI_NOM "nom-de-ton-wifi"
#define WIFI_MOT_DE_PASSE "mot-de-passe-du-wifi"
#define RESEAU_BOITIER_NOM "Station-Meteo"
#define RESEAU_BOITIER_MOT_DE_PASSE "change-moi-2026"
#define DECALAGE_UTC_S (3 * 3600)   // Djibouti : UTC+3

// --- Simulation : true = pas de capteurs, l'ESP32 invente une journée chaude et poussiéreuse. ---
#define MODE_SIMULATION true

// --- Particules : on réveille le capteur toutes les 5 min, on attend 30 s qu'il se stabilise,
//     on mesure, puis on l'endort. Son ventilateur s'use : il dure ainsi beaucoup plus longtemps. ---
#define INTERVALLE_AIR_S 300
#define CHAUFFE_AIR_S 30

// --- Branchements ---
// BME280 (température, humidité, pression) : I2C, SDA = GPIO21, SCL = GPIO22, adresse 0x76 (parfois 0x77)
#define ADRESSE_BME280 0x76
#define BROCHE_PMS_RX 16    // ← fil TX du PMS5003
#define BROCHE_PMS_TX 17    // → fil RX du PMS5003
#define BROCHE_PMS_SET 4    // → fil SET du PMS5003 (HAUT = réveillé, BAS = endormi)

// --- Accès au tableau de bord : OBLIGATOIRE. Le programme REFUSE de compiler avec les valeurs
//     d'exemple ci-dessous (elles sont publiques sur GitHub). 10 caractères minimum. ---
#define UTILISATEUR_TABLEAU "hamdi"
#define MOT_DE_PASSE_TABLEAU "change-moi"
