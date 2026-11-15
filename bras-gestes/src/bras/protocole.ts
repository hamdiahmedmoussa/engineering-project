/*
  LE MESSAGE ENVOYÉ AU BRAS PAR LE CÂBLE USB
  Une ligne de texte par commande, facile à lire dans le Moniteur série :
    "A 90 90 90 90 20 20 20 20 20\n" → 9 angles entiers, dans l'ordre :
       base, épaule, coude, poignet, pouce, index, majeur, annulaire, auriculaire
    "R\n" → retour en position de repos
  L'ESP32 vérifie chaque ligne et refuse tout ce qui est hors limites (voir limites.h).
*/
import { borner } from "./angles";
import { ARTICULATIONS, type Angles } from "./reglages";

export function ligneAngles(a: Angles): string {
  return "A " + ARTICULATIONS.map((x) => Math.round(borner(x, a[x]))).join(" ") + "\n";
}

export const LIGNE_REPOS = "R\n";

/** Faut-il renvoyer ? Seulement si un angle a bougé d'au moins 1° : on ne sature pas le câble. */
export function aChange(avant: Angles | null, apres: Angles): boolean {
  if (!avant) return true;
  return ARTICULATIONS.some((k) => Math.abs(Math.round(apres[k]) - Math.round(avant[k])) >= 1);
}
