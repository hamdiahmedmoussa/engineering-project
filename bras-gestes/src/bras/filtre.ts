/*
  LE FILTRE « ONE EURO » : fluide ET précis
  Problème : la détection de la main tremble un peu à chaque image. Si on lisse fort, le bras ne
  tremble plus mais il est en retard et rate les petits mouvements. Si on lisse peu, il tremble.

  Le filtre One Euro (Casiez, Roussel et Vogel, 2012), utilisé par la plupart des systèmes de suivi
  de la main, règle le lissage TOUT SEUL selon la vitesse :
    - main presque immobile → lissage fort : les tremblements disparaissent ;
    - main qui bouge vite   → lissage faible : aucun retard.

  En plus : un garde-fou contre les ERREURS de détection. Un saut énorme qui ne dure qu'une image
  (MediaPipe qui se trompe un instant) est ignoré ; un vrai mouvement rapide est confirmé par l'image
  suivante et passe.
*/
import { ARTICULATIONS, type Angles } from "./reglages";

/** Réglages du filtre (les valeurs du papier original, adaptées à des angles en degrés). */
export const FILTRE = {
  /** Fréquence de coupure au repos (Hz) : plus bas = plus lisse quand la main est immobile. */
  coupureMin: 1.2,
  /** Réactivité à la vitesse : plus haut = moins de retard quand la main bouge vite. */
  beta: 0.03,
  /** Lissage de l'estimation de vitesse (Hz). */
  coupureVitesse: 1.0,
  /** Un saut plus grand que ça entre deux images (degrés) est suspect… */
  sautSuspect: 45,
  /** … et n'est accepté que si l'image suivante le confirme (à cette distance près, en degrés). */
  confirmation: 15,
};

/** Coefficient de lissage pour une fréquence de coupure et un intervalle de temps donnés. */
const alpha = (coupure: number, dt: number) => 1 / (1 + 1 / (2 * Math.PI * coupure * dt));

class FiltreUnEuro {
  private x: number | null = null;
  private dx = 0;

  filtrer(valeur: number, dt: number): number {
    if (this.x === null || dt <= 0) {
      this.x = valeur;
      return valeur;
    }
    const vitesse = (valeur - this.x) / dt;
    this.dx += alpha(FILTRE.coupureVitesse, dt) * (vitesse - this.dx);
    const coupure = FILTRE.coupureMin + FILTRE.beta * Math.abs(this.dx);
    this.x += alpha(coupure, dt) * (valeur - this.x);
    return this.x;
  }

  get valeur(): number | null {
    return this.x;
  }
}

export class FiltreAngles {
  private filtres = Object.fromEntries(ARTICULATIONS.map((a) => [a, new FiltreUnEuro()])) as Record<string, FiltreUnEuro>;
  private enAttente: Partial<Angles> = {};
  private tPrecedent: number | null = null;

  /** À appeler à chaque nouvelle détection de la main. t en secondes. */
  filtrer(bruts: Angles, t: number): Angles {
    const dt = this.tPrecedent === null ? 0 : Math.min(0.2, t - this.tPrecedent);
    this.tPrecedent = t;
    const sortie = {} as Angles;
    for (const a of ARTICULATIONS) {
      const f = this.filtres[a];
      let v = bruts[a];
      const avant = f.valeur;
      if (avant !== null && Math.abs(v - avant) > FILTRE.sautSuspect) {
        const suspect = this.enAttente[a];
        if (suspect === undefined || Math.abs(v - suspect) > FILTRE.confirmation) {
          this.enAttente[a] = v; // premier saut : on le met de côté et on garde l'ancienne valeur
          v = avant;
        }
      }
      if (v !== avant) delete this.enAttente[a];
      sortie[a] = f.filtrer(v, dt);
    }
    return sortie;
  }

  /** Quand la main disparaît puis revient : on repart de zéro (pas de « saut » vers l'ancienne position). */
  reinitialiser(): void {
    this.filtres = Object.fromEntries(ARTICULATIONS.map((a) => [a, new FiltreUnEuro()])) as Record<string, FiltreUnEuro>;
    this.enAttente = {};
    this.tPrecedent = null;
  }
}
