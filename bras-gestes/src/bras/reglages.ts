/*
  LES 9 SERVOS DU BRAS ET DE LA MAIN, ET LEURS LIMITES (à ajuster pendant la calibration, voir README)
  Bras : base, épaule, coude. Main : poignet (rotation) et 5 doigts.
  Pour un doigt : min = doigt OUVERT (tendu), max = doigt FERMÉ (plié).
  Si un servo tourne dans l'autre sens sur ton montage, mets « inverse: true ».
  Les MÊMES min / max / repos sont écrits dans firmware/bras-gestes/limites.h : l'ESP32 vérifie aussi.
*/
export interface Limite {
  min: number;
  max: number;
  repos: number;
  /** Le servo est monté à l'envers : on inverse le sens (côté page seulement). */
  inverse?: boolean;
}

export const ARTICULATIONS = [
  "base", "epaule", "coude", "poignet", "pouce", "index", "majeur", "annulaire", "auriculaire",
] as const;
export type Articulation = (typeof ARTICULATIONS)[number];

export const DOIGTS = ["pouce", "index", "majeur", "annulaire", "auriculaire"] as const;
export type Doigt = (typeof DOIGTS)[number];

export const NOMS: Record<Articulation, string> = {
  base: "Base",
  epaule: "Épaule",
  coude: "Coude",
  poignet: "Poignet",
  pouce: "Pouce",
  index: "Index",
  majeur: "Majeur",
  annulaire: "Annulaire",
  auriculaire: "Auriculaire",
};

export const LIMITES: Record<Articulation, Limite> = {
  base: { min: 10, max: 170, repos: 90 },
  epaule: { min: 40, max: 150, repos: 90 },
  coude: { min: 40, max: 150, repos: 90 },
  poignet: { min: 30, max: 150, repos: 90 },
  pouce: { min: 20, max: 150, repos: 20 },
  index: { min: 20, max: 160, repos: 20 },
  majeur: { min: 20, max: 160, repos: 20 },
  annulaire: { min: 20, max: 160, repos: 20 },
  auriculaire: { min: 20, max: 160, repos: 20 },
};

/** Vitesse maximale d'un servo, en degrés par seconde : des mouvements doux, sans à-coups. */
export const VITESSE_MAX = 150;

export type Angles = Record<Articulation, number>;

export const ANGLES_REPOS = Object.fromEntries(ARTICULATIONS.map((a) => [a, LIMITES[a].repos])) as Angles;
