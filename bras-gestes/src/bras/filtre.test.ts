import { describe, expect, it } from "vitest";
import { FiltreAngles } from "./filtre";
import { ANGLES_REPOS, type Angles } from "./reglages";

const avec = (base: number): Angles => ({ ...ANGLES_REPOS, base });
const IMAGE = 1 / 30; // 30 détections par seconde

describe("filtre One Euro", () => {
  it("main immobile qui tremble de ±2° : le tremblement est fortement réduit", () => {
    const f = new FiltreAngles();
    const sorties: number[] = [];
    for (let i = 0; i < 90; i++) sorties.push(f.filtrer(avec(90 + (i % 2 ? 2 : -2)), i * IMAGE).base);
    const fin = sorties.slice(60);
    expect(Math.max(...fin) - Math.min(...fin)).toBeLessThan(1); // 4° d'écart en entrée → moins de 1°
  });

  it("mouvement rapide : le filtre suit avec peu de retard", () => {
    const f = new FiltreAngles();
    let sortie = 0;
    // La main tourne de 90° à 150° en une seconde (60 °/s), par pas de 2°.
    for (let i = 0; i <= 30; i++) sortie = f.filtrer(avec(90 + 2 * i), i * IMAGE).base;
    expect(150 - sortie).toBeLessThan(8); // moins de 8° de retard à la fin
  });

  it("un petit mouvement de 3° est bien transmis (pas effacé)", () => {
    const f = new FiltreAngles();
    let sortie = 0;
    for (let i = 0; i < 30; i++) f.filtrer(avec(90), i * IMAGE);
    for (let i = 30; i < 90; i++) sortie = f.filtrer(avec(93), i * IMAGE).base;
    expect(sortie).toBeGreaterThan(92.5);
  });

  it("erreur de détection d'UNE image (saut de 70°) : ignorée", () => {
    const f = new FiltreAngles();
    for (let i = 0; i < 30; i++) f.filtrer(avec(90), i * IMAGE);
    const pendantErreur = f.filtrer(avec(160), 30 * IMAGE).base;
    const apres = f.filtrer(avec(90), 31 * IMAGE).base;
    expect(pendantErreur).toBeCloseTo(90, 0);
    expect(apres).toBeCloseTo(90, 0);
  });

  it("vrai mouvement brusque (confirmé par l'image suivante) : accepté", () => {
    const f = new FiltreAngles();
    for (let i = 0; i < 30; i++) f.filtrer(avec(90), i * IMAGE);
    f.filtrer(avec(150), 30 * IMAGE);
    let sortie = 0;
    for (let i = 31; i < 45; i++) sortie = f.filtrer(avec(150), i * IMAGE).base;
    expect(sortie).toBeGreaterThan(140);
  });
});
