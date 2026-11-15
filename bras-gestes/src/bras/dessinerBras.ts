/*
  LE BRAS ET LA MAIN DESSINÉS À L'ÉCRAN
  À gauche : le bras vu de côté (épaule, coude), avec la base vue de dessus en petit.
  À droite : la main vue de face, avec ses 5 doigts qui se plient et le poignet qui tourne.
  Sert à tester sans matériel, et à vérifier que le vrai bras fait bien la même chose.
*/
import { DOIGTS, LIMITES, type Angles, type Doigt } from "./reglages";

const rad = (d: number) => (d * Math.PI) / 180;

/** Pliage d'un doigt de 0 (ouvert) à 1 (fermé), d'après l'angle de son servo. */
function fermeture(a: Angles, d: Doigt): number {
  const l = LIMITES[d];
  const t = (a[d] - l.min) / (l.max - l.min);
  return l.inverse ? 1 - t : t;
}

export function dessinerBras(canvas: HTMLCanvasElement, a: Angles) {
  const ctx = canvas.getContext("2d")!;
  const s = getComputedStyle(document.documentElement);
  const c = (n: string) => s.getPropertyValue(n).trim();
  const w = canvas.width, h = canvas.height;
  ctx.clearRect(0, 0, w, h);
  ctx.lineCap = "round";
  ctx.lineJoin = "round";

  // ---------- Le bras, vu de côté ----------
  const sol = h - 24;
  const x0 = w * 0.2, y0 = sol - 24;
  ctx.fillStyle = c("--trait");
  ctx.beginPath(); ctx.roundRect(x0 - 40, sol - 10, 80, 16, 8); ctx.fill();
  ctx.fillStyle = c("--doux");
  ctx.beginPath(); ctx.roundRect(x0 - 15, y0, 30, sol - y0, 6); ctx.fill();

  const l1 = h * 0.3, l2 = h * 0.26;
  const angle1 = rad(180 - a.epaule);
  const x1 = x0 + Math.cos(angle1) * l1, y1 = y0 - Math.sin(angle1) * l1;
  const angle2 = angle1 - rad(180 - a.coude);
  const x2 = x1 + Math.cos(angle2) * l2, y2 = y1 - Math.sin(angle2) * l2;
  ctx.strokeStyle = c("--accent"); ctx.lineWidth = 14;
  ctx.beginPath(); ctx.moveTo(x0, y0); ctx.lineTo(x1, y1); ctx.stroke();
  ctx.strokeStyle = c("--soleil"); ctx.lineWidth = 11;
  ctx.beginPath(); ctx.moveTo(x1, y1); ctx.lineTo(x2, y2); ctx.stroke();
  ctx.fillStyle = c("--carte");
  for (const [x, y] of [[x0, y0], [x1, y1], [x2, y2]]) { ctx.beginPath(); ctx.arc(x, y, 5, 0, Math.PI * 2); ctx.fill(); }

  // Base, vue de dessus (petit cadran)
  const bx = x0, by = 34, br = 22;
  ctx.strokeStyle = c("--trait"); ctx.lineWidth = 2;
  ctx.beginPath(); ctx.arc(bx, by, br, Math.PI, 0); ctx.stroke();
  ctx.strokeStyle = c("--accent"); ctx.lineWidth = 5;
  const ab = rad(180 - a.base);
  ctx.beginPath(); ctx.moveTo(bx, by); ctx.lineTo(bx + Math.cos(ab) * br, by - Math.sin(ab) * br); ctx.stroke();
  ctx.fillStyle = c("--doux"); ctx.font = "11px system-ui"; ctx.textAlign = "center";
  ctx.fillText("base (vue de dessus)", bx, by + 16);

  // ---------- La main, vue de face ----------
  const mx = w * 0.7, my = h * 0.78;
  ctx.save();
  ctx.translate(mx, my);
  // Le poignet tourne : on « écrase » la main en largeur, comme une main qui pivote.
  const pivot = Math.cos(rad(a.poignet - LIMITES.poignet.repos));
  ctx.scale(Math.max(0.25, Math.abs(pivot)), 1);

  // Paume
  ctx.fillStyle = c("--accent-pale");
  ctx.strokeStyle = c("--accent"); ctx.lineWidth = 3;
  ctx.beginPath(); ctx.roundRect(-46, -70, 92, 80, 18); ctx.fill(); ctx.stroke();

  // Doigts : 3 phalanges qui se replient vers la paume (le dessin les raccourcit et les courbe)
  const bases: Record<Doigt, [number, number, number]> = {
    pouce: [-46, -30, 0.8], index: [-33, -70, 1], majeur: [-11, -70, 1.1], annulaire: [11, -70, 1], auriculaire: [32, -70, 0.8],
  };
  for (const d of DOIGTS) {
    const [x, y, taille] = bases[d];
    const f = fermeture(a, d);
    let px = x, py = y;
    let dir = d === "pouce" ? rad(-150) : rad(-90);  // le pouce part sur le côté
    const longueurs = [26, 20, 16].map((l) => l * taille);
    ctx.strokeStyle = c("--soleil"); ctx.lineWidth = 12;
    ctx.beginPath(); ctx.moveTo(px, py);
    for (const l of longueurs) {
      // Vue de face, un doigt qui se plie vient vers nous : il paraît plus court.
      // Le pouce, lui, se replie en travers de la paume : il tourne.
      if (d === "pouce") dir += f * rad(55);
      const raccourci = 1 - f * 0.55;
      px += Math.cos(dir) * l * raccourci;
      py += Math.sin(dir) * l * raccourci;
      ctx.lineTo(px, py);
    }
    ctx.stroke();
  }
  ctx.restore();
  ctx.fillStyle = c("--doux"); ctx.font = "11px system-ui"; ctx.textAlign = "center";
  ctx.fillText("main (vue de face)", mx, h - 6);
}
