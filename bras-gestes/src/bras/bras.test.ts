import { describe, expect, it } from "vitest";
import { convertir, lisser, mainVersAngles, pliage, rotationPoignet, type Point } from "./angles";
import { aChange, ligneAngles } from "./protocole";
import { ANGLES_REPOS, DOIGTS, LIMITES, type Doigt } from "./reglages";

/** Une main « image » (2D) centrée en (cx, cy), de taille t : pour la position du bras. */
function mainImage(cx: number, cy: number, t: number): Point[] {
  const p: Point[] = Array.from({ length: 21 }, () => ({ x: cx, y: cy }));
  p[0] = { x: cx, y: cy + t / 2 };
  p[9] = { x: cx, y: cy - t / 2 };
  p[5] = { x: cx - t / 4, y: cy - t / 2 };
  p[13] = { x: cx + t / 8, y: cy - t / 2 };
  p[17] = { x: cx + t / 8, y: cy - t / 2 + t * 0.1 };
  return p;
}

/**
 * Une main « monde » (3D, en mètres) : chaque articulation de chaque doigt plie de « plis[doigt] » degrés.
 * Le poignet est à l'origine, les doigts montent (y négatif) et plient vers la paume (z positif).
 */
function mainMonde(plis: Partial<Record<Doigt, number>>, rotationZ = 0): Point[] {
  const p: Point[] = Array.from({ length: 21 }, () => ({ x: 0, y: 0, z: 0 }));
  const chaine = (indices: number[], depart: Point, dir0: number, longueurs: number[], pli: number) => {
    let { x, y, z = 0 } = depart;
    let angle = dir0; // angle dans le plan y-z : 0 = vers le haut
    p[indices[0]] = { x, y, z };
    longueurs.forEach((l, k) => {
      y -= Math.cos((angle * Math.PI) / 180) * l;
      z += Math.sin((angle * Math.PI) / 180) * l;
      p[indices[k + 1]] = { x, y, z };
      angle += pli;
    });
  };
  const bases: [Doigt, number[], number][] = [
    ["index", [5, 6, 7, 8], -0.03], ["majeur", [9, 10, 11, 12], -0.01],
    ["annulaire", [13, 14, 15, 16], 0.01], ["auriculaire", [17, 18, 19, 20], 0.03],
  ];
  for (const [d, idx, x] of bases) {
    const pli = plis[d] ?? 0;
    // Segment poignet → base du doigt, puis pliage à la base, au milieu et au bout (3 articulations).
    p[idx[0]] = { x, y: -0.08, z: 0 };
    chaine(idx, p[idx[0]], pli, [0.04, 0.025, 0.02], pli);
  }
  chaine([1, 2, 3, 4], { x: -0.04, y: -0.02, z: 0 }, 0, [0.03, 0.03, 0.025], plis.pouce ?? 0);
  p[17] = { ...p[17], z: rotationZ };
  return p;
}

describe("pliage des doigts", () => {
  it("doigt tendu : petit angle (l'éventail naturel des doigts), chaque articulation qui plie ajoute son angle", () => {
    const tendu = pliage(mainMonde({}), "index");
    expect(tendu).toBeLessThan(25);
    // 3 articulations × 50° : environ 150° de plus (la 1re articulation mélange éventail et pliage, en 3D).
    const plie = pliage(mainMonde({ index: 50 }), "index");
    expect(plie).toBeGreaterThan(130);
    expect(plie).toBeLessThan(170);
    expect(pliage(mainMonde({ pouce: 30 }), "pouce")).toBeCloseTo(60, 5); // 2 articulations × 30°
  });

  it("main ouverte : doigts au minimum ; poing : au maximum", () => {
    const ouverte = mainVersAngles(mainImage(0.5, 0.5, 0.2), mainMonde({}));
    const poing = mainVersAngles(mainImage(0.5, 0.5, 0.2), mainMonde({ pouce: 45, index: 70, majeur: 70, annulaire: 70, auriculaire: 70 }));
    for (const d of DOIGTS) {
      expect(ouverte[d]).toBe(LIMITES[d].min);
      expect(poing[d]).toBe(LIMITES[d].max);
    }
  });

  it("chaque doigt est indépendant (index seul plié)", () => {
    const a = mainVersAngles(mainImage(0.5, 0.5, 0.2), mainMonde({ index: 70 }));
    expect(a.index).toBe(LIMITES.index.max);
    expect(a.majeur).toBe(LIMITES.majeur.min);
  });
});

describe("poignet", () => {
  it("paume face à la caméra : 0°, main tournée : angle positif", () => {
    expect(rotationPoignet(mainMonde({}))).toBeCloseTo(0, 5);
    expect(rotationPoignet(mainMonde({}, 0.06))).toBeGreaterThan(30);
  });
});

describe("position du bras", () => {
  it("convertit et borne", () => {
    expect(convertir(0.5, 0, 1, 0, 180)).toBe(90);
    expect(convertir(2, 0, 1, 0, 180)).toBe(180);
    expect(convertir(0.85, 0.85, 0.2, 40, 150)).toBe(40);
  });

  it("main au centre : base au milieu ; à droite de l'utilisateur : base au max", () => {
    const m = mainMonde({});
    expect(mainVersAngles(mainImage(0.5, 0.5, 0.2), m).base).toBeCloseTo((LIMITES.base.min + LIMITES.base.max) / 2, 0);
    expect(mainVersAngles(mainImage(0.1, 0.5, 0.2), m).base).toBe(LIMITES.base.max);
  });

  it("main haute : épaule levée ; main proche : coude tendu", () => {
    const m = mainMonde({});
    expect(mainVersAngles(mainImage(0.5, 0.1, 0.2), m).epaule).toBe(LIMITES.epaule.max);
    expect(mainVersAngles(mainImage(0.5, 0.5, 0.35), m).coude).toBe(LIMITES.coude.max);
  });
});

describe("mouvement doux", () => {
  it("ne dépasse jamais la vitesse maximale, et atteint la cible sans sortir des limites", () => {
    const apres = lisser(ANGLES_REPOS, { ...ANGLES_REPOS, base: 170 }, 0.1); // 0,1 s à 150 °/s → 15° au plus
    expect(apres.base - ANGLES_REPOS.base).toBeLessThanOrEqual(15.0001);
    let a = ANGLES_REPOS;
    for (let i = 0; i < 300; i++) a = lisser(a, { ...ANGLES_REPOS, index: 999 }, 1 / 30);
    expect(a.index).toBe(LIMITES.index.max);
  });
});

describe("message pour l'ESP32", () => {
  it("9 angles entiers, bornés, dans le bon ordre", () => {
    expect(ligneAngles({ ...ANGLES_REPOS, base: 90.4, index: 999 })).toBe(
      `A 90 90 90 90 20 ${LIMITES.index.max} 20 20 20\n`,
    );
  });

  it("n'envoie que si un angle a bougé d'au moins 1°", () => {
    expect(aChange(null, ANGLES_REPOS)).toBe(true);
    expect(aChange(ANGLES_REPOS, { ...ANGLES_REPOS, majeur: 20.3 })).toBe(false);
    expect(aChange(ANGLES_REPOS, { ...ANGLES_REPOS, majeur: 21.2 })).toBe(true);
  });
});
