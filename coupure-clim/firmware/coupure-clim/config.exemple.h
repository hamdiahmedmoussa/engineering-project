/*
  RÉGLAGES DE LA COUPURE INTELLIGENTE
  1. Copie ce fichier et renomme la copie « config.h » (dans le même dossier).
  2. Remplis le Wi-Fi, puis la marque de ta clim (étape B du README).
  config.h n'est jamais envoyé sur GitHub : ton mot de passe reste chez toi.
*/
#pragma once

// --- Wi-Fi (avec internet : l'heure exacte, indispensable pour le mode nuit) ---
#define WIFI_NOM "nom-de-ton-wifi"
#define WIFI_MOT_DE_PASSE "mot-de-passe-du-wifi"
#define RESEAU_BOITIER_NOM "Coupure-Clim"
#define RESEAU_BOITIER_MOT_DE_PASSE "change-moi-2026"
#define DECALAGE_UTC_S (3 * 3600)        // Djibouti : UTC+3

// --- Simulation : true = pas de capteurs ni de clim ; les MINUTES deviennent des SECONDES
//     pour voir les règles agir vite. Mets false quand tout est branché. ---
#define MODE_SIMULATION true

// --- Ta clim : le nom du protocole infrarouge, affiché par le Moniteur série quand tu appuies
//     sur la télécommande devant le récepteur (ex. "GREE", "MIDEA", "LG", "SAMSUNG_AC", "DAIKIN"). ---
#define PROTOCOLE_CLIM "GREE"
#define PUISSANCE_CLIM_W 1200            // mesurée avec le compteur d'énergie, ou étiquette de la clim

// --- Les règles ---
#define DELAI_SANS_PRESENCE_MIN 15       // pièce vide depuis 15 min → éteindre
#define NUIT_DEBUT_H 22                  // de 22 h…
#define NUIT_FIN_H 7                     // … à 7 h : jamais d'extinction automatique
#define CONSIGNE_MIN 24                  // mode éco : pas plus froid que 24 °C (0 = désactivé)
#define CONSIGNE_ALLUMAGE 25             // température quand on allume depuis le téléphone
#define SEUIL_SURCHAUFFE_C 32.0          // pièce encore à 32 °C…
#define DELAI_SURCHAUFFE_MIN 45          // … après 45 min de clim → alerte

// --- Branchements ---
#define BROCHE_IR_LED 4                  // LED infrarouge (via un transistor, voir schéma)
#define BROCHE_IR_RECEPTEUR 15           // récepteur infrarouge (lit la vraie télécommande)
#define BROCHE_PRESENCE 27               // sortie « OUT » du radar LD2410 (HAUT = quelqu'un)
#define BROCHE_DHT 26                    // capteur température / humidité DHT22

// --- Accès au tableau de bord : OBLIGATOIRE. Le programme REFUSE de compiler avec les valeurs
//     d'exemple ci-dessous (elles sont publiques sur GitHub). 10 caractères minimum. ---
#define UTILISATEUR_TABLEAU "hamdi"
#define MOT_DE_PASSE_TABLEAU "change-moi"
