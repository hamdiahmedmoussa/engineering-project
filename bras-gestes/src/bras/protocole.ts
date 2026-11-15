/*
  LE MESSAGE ENVOYÉ AU BRAS PAR LE CÂBLE USB
  Une ligne de texte par commande, facile à lire dans le Moniteur série :
    "A 90 45 120 60\n"  → angles de la base, de l'épaule, du coude et de la pince (degrés entiers)
    "R\n"               → retour en position de repos
  L'ESP32 vérifie chaque ligne et refuse tout ce qui est hors limites (voir limites.h).
*/
import { borner } from "./angles";
import type { Angles } from "./reglages";

export function ligneAngles(a: Angles): string {
  const v = (x: keyof Angles) => Math.round(borner(x, a[x]));
  return `A ${v("base")} ${v("epaule")} ${v("coude")} ${v("pince")}\n`;
}

export const LIGNE_REPOS = "R\n";

/** Faut-il renvoyer ? Seulement si un angle a bougé d'au moins 1° : on ne sature pas le câble. */
export function aChange(avant: Angles | null, apres: Angles): boolean {
  if (!avant) return true;
  return (Object.keys(apres) as (keyof Angles)[]).some((k) => Math.abs(Math.round(apres[k]) - Math.round(avant[k])) >= 1);
}
