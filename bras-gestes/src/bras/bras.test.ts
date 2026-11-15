import { describe, expect, it } from "vitest";
import { convertir, lisser, mainVersAngles, type Point } from "./angles";
import { aChange, ligneAngles } from "./protocole";
import { ANGLES_REPOS, LIMITES } from "./reglages";

/** Une main simple, centrée en (cx, cy), de taille t, pouce et index écartés de e (en tailles de main). */
function main(cx: number, cy: number, t: number, e: number): Point[] {
  const p: Point[] = Array.from({ length: 21 }, () => ({ x: cx, y: cy }));
  p[0] = { x: cx, y: cy + t / 2 }; // poignet
  p[9] = { x: cx, y: cy - t / 2 }; // base du majeur
  p[5] = { x: cx - t / 4, y: cy - t / 2 };
  // Bases de l'annulaire et de l'auriculaire placées pour que la paume reste centrée sur cx.
  p[13] = { x: cx + t / 8, y: cy - t / 2 };
  p[17] = { x: cx + t / 8, y: cy - t / 2 + t * 0.1 };
  p[4] = { x: cx - (e * t) / 2, y: cy - t };
  p[8] = { x: cx + (e * t) / 2, y: cy - t };
  return p;
}

describe("de la main aux angles", () => {
  it("convertit et borne", () => {
    expect(convertir(0.5, 0, 1, 0, 180)).toBe(90);
    expect(convertir(2, 0, 1, 0, 180)).toBe(180);
    expect(convertir(-1, 0, 1, 0, 180)).toBe(0);
    expect(convertir(0.85, 0.85, 0.2, 40, 150)).toBe(40); // plage inversée (le haut de l'image = y petit)
  });

  it("main au centre : base au milieu", () => {
    const a = mainVersAngles(main(0.5, 0.5, 0.2, 0.6));
    expect(a.base).toBeCloseTo((LIMITES.base.min + LIMITES.base.max) / 2, 0);
  });

  it("main à droite de l'utilisateur (gauche de l'image) : base vers le max", () => {
    expect(mainVersAngles(main(0.1, 0.5, 0.2, 0.6)).base).toBe(LIMITES.base.max);
    expect(mainVersAngles(main(0.9, 0.5, 0.2, 0.6)).base).toBe(LIMITES.base.min);
  });

  it("main haute : épaule levée ; main basse : épaule baissée", () => {
    expect(mainVersAngles(main(0.5, 0.1, 0.2, 0.6)).epaule).toBe(LIMITES.epaule.max);
    expect(mainVersAngles(main(0.5, 0.95, 0.2, 0.6)).epaule).toBe(LIMITES.epaule.min);
  });

  it("main proche (grande) : coude tendu", () => {
    expect(mainVersAngles(main(0.5, 0.5, 0.35, 0.6)).coude).toBe(LIMITES.coude.max);
    expect(mainVersAngles(main(0.5, 0.5, 0.06, 0.6)).coude).toBe(LIMITES.coude.min);
  });

  it("pincer ferme la pince, écarter l'ouvre", () => {
    expect(mainVersAngles(main(0.5, 0.5, 0.2, 0.1)).pince).toBe(LIMITES.pince.min);
    expect(mainVersAngles(main(0.5, 0.5, 0.2, 1.5)).pince).toBe(LIMITES.pince.max);
  });
});

describe("mouvement doux", () => {
  it("ne dépasse jamais la vitesse maximale", () => {
    const vise = { ...ANGLES_REPOS, base: 170 };
    const apres = lisser(ANGLES_REPOS, vise, 0.1); // 0,1 s à 120 °/s → 12° au plus
    expect(apres.base - ANGLES_REPOS.base).toBeLessThanOrEqual(12.0001);
    expect(apres.base).toBeGreaterThan(ANGLES_REPOS.base);
  });

  it("finit par atteindre la cible, sans dépasser les limites", () => {
    let a = ANGLES_REPOS;
    for (let i = 0; i < 300; i++) a = lisser(a, { ...ANGLES_REPOS, base: 999 }, 1 / 30);
    expect(a.base).toBe(LIMITES.base.max);
  });
});

describe("message pour l'ESP32", () => {
  it("écrit une ligne d'angles entiers, bornés", () => {
    expect(ligneAngles({ base: 90.4, epaule: 45.6, coude: 120, pince: 60 })).toBe("A 90 46 120 60\n");
    expect(ligneAngles({ base: 999, epaule: -5, coude: 120, pince: 60 })).toBe(
      `A ${LIMITES.base.max} ${LIMITES.epaule.min} 120 60\n`,
    );
  });

  it("n'envoie que si un angle a bougé d'au moins 1°", () => {
    expect(aChange(null, ANGLES_REPOS)).toBe(true);
    expect(aChange(ANGLES_REPOS, { ...ANGLES_REPOS, base: 90.3 })).toBe(false);
    expect(aChange(ANGLES_REPOS, { ...ANGLES_REPOS, base: 91.2 })).toBe(true);
  });
});
