/*
  LES LIMITES DES SERVOS · à garder IDENTIQUES à src/bras/reglages.ts (côté page web)
  Ordre : base, épaule, coude, pince. Pince : min = fermée, max = ouverte.
  Ajuste-les pendant la calibration (README, étape C).
*/
#pragma once

const int NB_SERVOS = 4;
const int BROCHES[NB_SERVOS] = {13, 12, 14, 27};   // fils de signal (orange ou jaune) des servos
const int LIMITE_MIN[NB_SERVOS] = {10, 40, 40, 30};
const int LIMITE_MAX[NB_SERVOS] = {170, 150, 150, 95};
const int REPOS[NB_SERVOS] = {90, 90, 90, 60};
