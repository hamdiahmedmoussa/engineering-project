/*
  LES LIMITES DU BRAS (à ajuster pendant la calibration, voir README)
  Chaque servomoteur a un angle minimum, un angle maximum et une position de repos.
  Ne jamais dépasser les limites : le bras forcerait contre lui-même et les servos chaufferaient.
  Les MÊMES valeurs sont écrites dans firmware/bras-gestes/limites.h : l'ESP32 vérifie aussi.
*/
export interface Limite {
  min: number;
  max: number;
  repos: number;
}

export type Articulation = "base" | "epaule" | "coude" | "pince";

export const ARTICULATIONS: Articulation[] = ["base", "epaule", "coude", "pince"];

export const NOMS: Record<Articulation, string> = {
  base: "Base (rotation)",
  epaule: "Épaule",
  coude: "Coude",
  pince: "Pince",
};

export const LIMITES: Record<Articulation, Limite> = {
  base: { min: 10, max: 170, repos: 90 },
  epaule: { min: 40, max: 150, repos: 90 },
  coude: { min: 40, max: 150, repos: 90 },
  pince: { min: 30, max: 95, repos: 60 }, // min = pince fermée, max = pince ouverte
};

/** Vitesse maximale d'un servo, en degrés par seconde : des mouvements doux, sans à-coups. */
export const VITESSE_MAX = 120;

export type Angles = Record<Articulation, number>;

export const ANGLES_REPOS: Angles = {
  base: LIMITES.base.repos,
  epaule: LIMITES.epaule.repos,
  coude: LIMITES.coude.repos,
  pince: LIMITES.pince.repos,
};
