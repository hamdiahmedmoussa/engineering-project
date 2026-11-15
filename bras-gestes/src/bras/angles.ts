/*
  DE LA MAIN AUX 9 ANGLES
  MediaPipe donne deux séries de 21 points :
    - « main »  : dans l'image (x, y de 0 à 1)  → la POSITION de la main pilote le bras ;
    - « monde » : en 3D, en mètres, centrée sur la main → la FORME de la main pilote les doigts et le poignet.
  Numéros des points : 0 poignet, 1-4 pouce, 5-8 index, 9-12 majeur, 13-16 annulaire, 17-20 auriculaire.
*/
import { DOIGTS, LIMITES, VITESSE_MAX, type Angles, type Articulation, type Doigt } from "./reglages";

export interface Point {
  x: number;
  y: number;
  z?: number;
}

const distance = (a: Point, b: Point) => Math.hypot(a.x - b.x, a.y - b.y);

/** Ramène v de [de1, de2] vers [vers1, vers2], sans sortir de [vers1, vers2]. */
export function convertir(v: number, de1: number, de2: number, vers1: number, vers2: number): number {
  const t = Math.min(1, Math.max(0, (v - de1) / (de2 - de1)));
  return vers1 + t * (vers2 - vers1);
}

/** Garde un angle entre les limites de son servo (sécurité). */
export function borner(articulation: Articulation, angle: number): number {
  const l = LIMITES[articulation];
  return Math.min(l.max, Math.max(l.min, angle));
}

/** Une valeur de 0 à 1 devient un angle du servo (en tenant compte d'un servo monté à l'envers). */
function versServo(a: Articulation, t: number): number {
  const l = LIMITES[a];
  const u = Math.min(1, Math.max(0, t));
  return l.inverse ? l.max - u * (l.max - l.min) : l.min + u * (l.max - l.min);
}

/** Angle (en degrés) entre deux segments 3D : 0° = alignés (doigt droit). */
function angleEntre(a: Point, b: Point, c: Point, d: Point): number {
  const u = [b.x - a.x, b.y - a.y, (b.z ?? 0) - (a.z ?? 0)];
  const v = [d.x - c.x, d.y - c.y, (d.z ?? 0) - (c.z ?? 0)];
  const produit = u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
  const normes = Math.hypot(...u) * Math.hypot(...v);
  return normes === 0 ? 0 : (Math.acos(Math.min(1, Math.max(-1, produit / normes))) * 180) / Math.PI;
}

/** Les points de chaque doigt, de la base vers le bout (le poignet sert de « segment 0 » pour les 4 doigts). */
const POINTS: Record<Doigt, number[]> = {
  pouce: [1, 2, 3, 4],
  index: [0, 5, 6, 7, 8],
  majeur: [0, 9, 10, 11, 12],
  annulaire: [0, 13, 14, 15, 16],
  auriculaire: [0, 17, 18, 19, 20],
};

/**
 * Pliage d'un doigt, en degrés : la somme des angles de ses articulations.
 * Doigt tendu ≈ 0°, poing fermé ≈ 200° (pour le pouce, environ 90°).
 */
export function pliage(monde: Point[], doigt: Doigt): number {
  const p = POINTS[doigt].map((i) => monde[i]);
  let total = 0;
  for (let k = 0; k + 2 < p.length; k++) total += angleEntre(p[k], p[k + 1], p[k + 1], p[k + 2]);
  return total;
}

/**
 * Rotation du poignet (comme tourner une poignée de porte), en degrés : 0 = paume face à la caméra.
 * On regarde la ligne « base de l'index → base de l'auriculaire » : quand on tourne la main, elle s'incline en profondeur.
 */
export function rotationPoignet(monde: Point[]): number {
  const dx = monde[17].x - monde[5].x;
  const dz = (monde[17].z ?? 0) - (monde[5].z ?? 0);
  return (Math.atan2(dz, Math.abs(dx)) * 180) / Math.PI;
}

/** Plages de mesure (réglables si ta caméra est plus proche ou plus loin, ou selon ta main). */
export const PLAGES = {
  x: [0.15, 0.85], // la main va de 15 % à 85 % de la largeur de l'image
  y: [0.85, 0.2], // de bas (85 %) en haut (20 %) de l'image
  taille: [0.1, 0.28], // distance poignet → base du majeur : main loin → main proche
  pliageDoigt: [25, 190], // degrés : tendu → fermé (25 et non 0 : les doigts s'écartent naturellement en éventail)
  pliagePouce: [15, 80],
  poignet: [-60, 60], // degrés de rotation
};

export function mainVersAngles(main: Point[], monde: Point[]): Angles {
  const paume = [0, 5, 9, 13, 17].map((i) => main[i]);
  const cx = paume.reduce((s, p) => s + p.x, 0) / paume.length;
  const cy = paume.reduce((s, p) => s + p.y, 0) / paume.length;
  const L = LIMITES;

  const angles = {
    // La caméra voit en miroir : main vers la droite de l'utilisateur = x plus petit dans l'image.
    base: convertir(1 - cx, PLAGES.x[0], PLAGES.x[1], L.base.min, L.base.max),
    epaule: convertir(cy, PLAGES.y[0], PLAGES.y[1], L.epaule.min, L.epaule.max),
    coude: convertir(distance(main[0], main[9]), PLAGES.taille[0], PLAGES.taille[1], L.coude.min, L.coude.max),
    poignet: versServo("poignet", convertir(rotationPoignet(monde), PLAGES.poignet[0], PLAGES.poignet[1], 0, 1)),
  } as Angles;

  for (const d of DOIGTS) {
    const [de, a] = d === "pouce" ? PLAGES.pliagePouce : PLAGES.pliageDoigt;
    angles[d] = versServo(d, convertir(pliage(monde, d), de, a, 0, 1));
  }
  return angles;
}

/**
 * Avance doucement des angles actuels vers les angles visés :
 * lissage (pour effacer les tremblements de la détection) + vitesse maximale (pour les servos).
 */
export function lisser(actuels: Angles, vises: Angles, dt: number): Angles {
  const suivant = { ...actuels };
  const k = 1 - Math.exp(-8 * dt);
  const pasMax = VITESSE_MAX * dt;
  for (const a of Object.keys(vises) as Articulation[]) {
    const ecart = (vises[a] - actuels[a]) * k;
    suivant[a] = borner(a, actuels[a] + Math.max(-pasMax, Math.min(pasMax, ecart)));
  }
  return suivant;
}
