/*
  DE LA MAIN AUX ANGLES DU BRAS
  On lit 4 choses sur la main (21 points de MediaPipe, coordonnées de 0 à 1) :
    - sa position gauche / droite  → base
    - sa hauteur                    → épaule
    - sa taille apparente           → coude (main proche de la caméra = bras tendu)
    - l'écart pouce / index         → pince (pincer = fermer)
  Chaque mesure est ramenée entre les limites du servo correspondant.
*/
import { LIMITES, VITESSE_MAX, type Angles, type Articulation } from "./reglages";

export interface Point {
  x: number;
  y: number;
}

const distance = (a: Point, b: Point) => Math.hypot(a.x - b.x, a.y - b.y);

/** Ramène v de l'intervalle [de1, de2] vers [vers1, vers2], sans sortir de [vers1, vers2]. */
export function convertir(v: number, de1: number, de2: number, vers1: number, vers2: number): number {
  const t = Math.min(1, Math.max(0, (v - de1) / (de2 - de1)));
  return vers1 + t * (vers2 - vers1);
}

/** Garde un angle entre les limites de son servo (sécurité). */
export function borner(articulation: Articulation, angle: number): number {
  const l = LIMITES[articulation];
  return Math.min(l.max, Math.max(l.min, angle));
}

/** Plages de mesure de la main (réglables si ta caméra est plus proche ou plus loin). */
export const PLAGES = {
  x: [0.15, 0.85], // la main va de 15 % à 85 % de la largeur de l'image
  y: [0.85, 0.2], // de bas (85 %) en haut (20 %) de l'image
  taille: [0.1, 0.28], // distance poignet → base du majeur : main loin → main proche
  pincement: [0.25, 1.0], // écart pouce-index ÷ taille de la main : pincé → grand ouvert
};

export function mainVersAngles(main: Point[]): Angles {
  // Centre de la paume : moyenne du poignet et de la base des 4 doigts.
  const paume = [0, 5, 9, 13, 17].map((i) => main[i]);
  const cx = paume.reduce((s, p) => s + p.x, 0) / paume.length;
  const cy = paume.reduce((s, p) => s + p.y, 0) / paume.length;
  const taille = distance(main[0], main[9]);
  const pincement = distance(main[4], main[8]) / Math.max(taille, 1e-6);
  const L = LIMITES;

  return {
    // La caméra voit en miroir : main vers la droite de l'utilisateur = x plus petit dans l'image.
    base: convertir(1 - cx, PLAGES.x[0], PLAGES.x[1], L.base.min, L.base.max),
    epaule: convertir(cy, PLAGES.y[0], PLAGES.y[1], L.epaule.min, L.epaule.max),
    coude: convertir(taille, PLAGES.taille[0], PLAGES.taille[1], L.coude.min, L.coude.max),
    pince: convertir(pincement, PLAGES.pincement[0], PLAGES.pincement[1], L.pince.min, L.pince.max),
  };
}

/**
 * Avance doucement des angles actuels vers les angles visés :
 * lissage (pour effacer les tremblements de la détection) + vitesse maximale (pour les servos).
 */
export function lisser(actuels: Angles, vises: Angles, dt: number): Angles {
  const suivant = { ...actuels };
  const k = 1 - Math.exp(-8 * dt); // plus le chiffre est grand, plus le bras suit vite
  const pasMax = VITESSE_MAX * dt;
  for (const a of Object.keys(vises) as Articulation[]) {
    const ecart = (vises[a] - actuels[a]) * k;
    suivant[a] = borner(a, actuels[a] + Math.max(-pasMax, Math.min(pasMax, ecart)));
  }
  return suivant;
}
