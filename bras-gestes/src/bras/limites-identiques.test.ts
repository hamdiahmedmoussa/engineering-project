// Les limites sont écrites deux fois : dans la page (reglages.ts) et dans l'ESP32 (limites.h).
// Ce test vérifie qu'elles sont identiques : si tu en changes une, il te rappelle l'autre.
import { readFileSync } from "node:fs";
import { expect, it } from "vitest";
import { ARTICULATIONS, LIMITES } from "./reglages";

const h = readFileSync(new URL("../../firmware/bras-gestes/limites.h", import.meta.url), "utf8");
const tableau = (nom: string) =>
  h.match(new RegExp(`${nom}\\[NB_SERVOS\\] = \\{([^}]*)\\}`))![1].split(",").map((v) => Number(v.trim()));

it("les limites de la page et de l'ESP32 sont les mêmes", () => {
  expect(tableau("LIMITE_MIN")).toEqual(ARTICULATIONS.map((a) => LIMITES[a].min));
  expect(tableau("LIMITE_MAX")).toEqual(ARTICULATIONS.map((a) => LIMITES[a].max));
  expect(tableau("REPOS")).toEqual(ARTICULATIONS.map((a) => LIMITES[a].repos));
});
