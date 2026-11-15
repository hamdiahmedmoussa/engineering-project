/*
  LE BRAS DESSINÉ À L'ÉCRAN (vue de côté + vue de dessus pour la base)
  Sert à tester sans le vrai bras, et à vérifier que le bras réel fait bien la même chose.
  Convention du dessin : épaule 90° = bras vertical ; coude 180° = bras tendu.
*/
import { LIMITES, type Angles } from "./reglages";

const rad = (d: number) => (d * Math.PI) / 180;

export function dessinerBras(canvas: HTMLCanvasElement, a: Angles) {
  const ctx = canvas.getContext("2d")!;
  const s = getComputedStyle(document.documentElement);
  const c = (n: string) => s.getPropertyValue(n).trim();
  const w = canvas.width, h = canvas.height;
  ctx.clearRect(0, 0, w, h);
  ctx.lineCap = "round";
  ctx.lineJoin = "round";

  // --- Vue de côté : socle, épaule, coude, pince ---
  const sol = h - 30;
  const x0 = w * 0.38, y0 = sol - 26;
  ctx.fillStyle = c("--trait");
  ctx.beginPath(); ctx.roundRect(x0 - 46, sol - 12, 92, 18, 9); ctx.fill();
  ctx.fillStyle = c("--doux");
  ctx.beginPath(); ctx.roundRect(x0 - 18, y0, 36, sol - y0, 6); ctx.fill();

  const l1 = h * 0.32, l2 = h * 0.28;
  const angle1 = rad(180 - a.epaule); // dans le canvas, y descend : on retourne l'angle
  const x1 = x0 + Math.cos(angle1) * l1, y1 = y0 - Math.sin(angle1) * l1;
  const angle2 = angle1 - rad(180 - a.coude);
  const x2 = x1 + Math.cos(angle2) * l2, y2 = y1 - Math.sin(angle2) * l2;

  ctx.strokeStyle = c("--accent"); ctx.lineWidth = 16;
  ctx.beginPath(); ctx.moveTo(x0, y0); ctx.lineTo(x1, y1); ctx.stroke();
  ctx.strokeStyle = c("--soleil"); ctx.lineWidth = 12;
  ctx.beginPath(); ctx.moveTo(x1, y1); ctx.lineTo(x2, y2); ctx.stroke();

  // Articulations
  ctx.fillStyle = c("--carte");
  for (const [x, y] of [[x0, y0], [x1, y1]]) { ctx.beginPath(); ctx.arc(x, y, 6, 0, Math.PI * 2); ctx.fill(); }

  // Pince : deux mâchoires qui s'écartent
  const ouverture = (a.pince - LIMITES.pince.min) / (LIMITES.pince.max - LIMITES.pince.min);
  const ecart = rad(8 + ouverture * 30);
  ctx.strokeStyle = c("--texte"); ctx.lineWidth = 5;
  for (const sens of [-1, 1]) {
    const ang = angle2 + sens * ecart;
    ctx.beginPath(); ctx.moveTo(x2, y2); ctx.lineTo(x2 + Math.cos(ang) * 26, y2 - Math.sin(ang) * 26); ctx.stroke();
  }

  // --- Vue de dessus : la rotation de la base ---
  const cx = w * 0.84, cy = h * 0.3, r = Math.min(w, h) * 0.13;
  ctx.strokeStyle = c("--trait"); ctx.lineWidth = 2;
  ctx.beginPath(); ctx.arc(cx, cy, r, Math.PI, 0); ctx.stroke();
  const ab = rad(180 - a.base);
  ctx.strokeStyle = c("--accent"); ctx.lineWidth = 6;
  ctx.beginPath(); ctx.moveTo(cx, cy); ctx.lineTo(cx + Math.cos(ab) * r, cy - Math.sin(ab) * r); ctx.stroke();
  ctx.fillStyle = c("--doux"); ctx.font = "12px system-ui"; ctx.textAlign = "center";
  ctx.fillText("vue de dessus", cx, cy + 20);
}
