/*
  LES LIMITES DES 9 SERVOS · à garder IDENTIQUES à src/bras/reglages.ts (côté page web)
  Ordre : base, épaule, coude, poignet, pouce, index, majeur, annulaire, auriculaire.
  Doigts : min = ouvert (tendu), max = fermé (plié).
  Ajuste-les pendant la calibration (README, étape C).
*/
#pragma once

const int NB_SERVOS = 9;
// Sorties de la carte PCA9685 (0 à 15) où chaque servo est branché.
const int CANAUX[NB_SERVOS] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
const int LIMITE_MIN[NB_SERVOS] = {10, 40, 40, 30, 20, 20, 20, 20, 20};
const int LIMITE_MAX[NB_SERVOS] = {170, 150, 150, 150, 150, 160, 160, 160, 160};
const int REPOS[NB_SERVOS] = {90, 90, 90, 90, 20, 20, 20, 20, 20};
