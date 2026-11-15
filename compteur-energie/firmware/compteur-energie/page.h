/*
  LE TABLEAU DE BORD (page web servie par l'ESP32 lui-même)
  Le téléphone ouvre l'adresse de l'ESP32 ; cette page demande les mesures (/mesures)
  chaque seconde et les affiche. Pas d'internet nécessaire : tout reste dans la maison.
  Le tarif (FDJ par kWh) est saisi sur la page et gardé dans le téléphone.
*/
#pragma once

const char PAGE_TABLEAU[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Compteur d'énergie</title>
<style>
  :root { color-scheme: dark; --fond:#0b1220; --carte:#111a2e; --trait:#22304d; --texte:#e2e8f0; --doux:#94a3b8; --accent:#facc15; }
  * { box-sizing: border-box; }
  body { margin:0; padding:16px; background:var(--fond); color:var(--texte); font-family:system-ui,"Segoe UI",sans-serif; line-height:1.4; }
  main { max-width:560px; margin:0 auto; display:grid; gap:12px; }
  h1 { margin:0; font-size:1.15rem; }
  .etat { margin:2px 0 0; color:var(--doux); font-size:.85rem; }
  .carte { padding:14px; border:1px solid var(--trait); border-radius:12px; background:var(--carte); }
  .grand { margin:0; font-size:3rem; font-weight:700; font-variant-numeric:tabular-nums; color:var(--accent); line-height:1.1; }
  .grand small { font-size:1.1rem; color:var(--doux); font-weight:500; }
  .note { margin:4px 0 0; color:var(--doux); font-size:.8rem; }
  dl { display:grid; grid-template-columns:1fr auto; gap:6px 12px; margin:0; }
  dt { color:var(--doux); }
  dd { margin:0; font-weight:600; text-align:right; font-variant-numeric:tabular-nums; }
  label { display:flex; justify-content:space-between; align-items:center; gap:12px; margin-bottom:10px; }
  input { width:7em; padding:8px; border:1px solid var(--trait); border-radius:8px; background:var(--fond); color:var(--texte); font:inherit; text-align:right; }
  canvas { width:100%; height:140px; display:block; }
  .alerte { color:#fca5a5; }
</style>
</head>
<body>
<main>
  <header>
    <h1>Compteur d'énergie</h1>
    <p class="etat" id="etat" role="status">Connexion…</p>
  </header>

  <section class="carte" aria-label="Puissance actuelle">
    <p class="grand"><span id="puissance">–</span> <small>W</small></p>
    <p class="note">Puissance estimée (courant mesuré × <span id="tension">220</span> V)</p>
  </section>

  <section class="carte" aria-label="Mesures">
    <dl>
      <dt>Courant</dt><dd id="courant">–</dd>
      <dt>Énergie depuis la mise en marche</dt><dd id="energie">–</dd>
      <dt>Durée de mesure</dt><dd id="duree">–</dd>
    </dl>
  </section>

  <section class="carte" aria-label="Coût">
    <label>Tarif de ta facture (FDJ par kWh)
      <input id="tarif" type="number" min="0" step="0.1" inputmode="decimal" placeholder="ex. sur ta facture">
    </label>
    <dl>
      <dt>Coût depuis la mise en marche</dt><dd id="cout">–</dd>
      <dt>Au rythme moyen : par jour</dt><dd id="jour">–</dd>
      <dt>Au rythme moyen : par mois (30 j)</dt><dd id="mois">–</dd>
    </dl>
  </section>

  <section class="carte" aria-label="Courbe des 2 dernières minutes">
    <canvas id="courbe" width="520" height="140" role="img" aria-label="Puissance des 2 dernières minutes"></canvas>
    <p class="note">Puissance des 2 dernières minutes</p>
  </section>
</main>

<script>
  const $ = (id) => document.getElementById(id);
  const nombre = (v, d) => v.toLocaleString("fr-FR", { minimumFractionDigits: d, maximumFractionDigits: d });
  const historique = [];
  let derniere = null;

  // Le tarif est gardé dans le téléphone (localStorage) pour ne pas le retaper à chaque fois.
  try { $("tarif").value = localStorage.getItem("tarif") || ""; } catch (e) {}
  $("tarif").addEventListener("input", () => {
    try { localStorage.setItem("tarif", $("tarif").value); } catch (e) {}
    afficherCout();
  });

  function afficherCout() {
    const tarif = parseFloat($("tarif").value);
    if (!derniere || !(tarif > 0)) {
      ["cout", "jour", "mois"].forEach((id) => ($(id).textContent = "indique le tarif"));
      return;
    }
    // Moyenne depuis la mise en marche (une clim s'allume et s'éteint : la puissance du moment varie trop).
    // Pendant la première minute, on n'a pas assez de recul : on prend la puissance du moment.
    const parJourKWh = derniere.depuis >= 60 ? (derniere.energie / derniere.depuis) * 86400 : (derniere.puissance * 24) / 1000;
    $("cout").textContent = nombre(derniere.energie * tarif, 0) + " FDJ";
    $("jour").textContent = nombre(parJourKWh * tarif, 0) + " FDJ";
    $("mois").textContent = nombre(parJourKWh * 30 * tarif, 0) + " FDJ";
  }

  function dessiner() {
    const c = $("courbe"), ctx = c.getContext("2d");
    ctx.clearRect(0, 0, c.width, c.height);
    if (historique.length < 2) return;
    const max = Math.max(100, ...historique) * 1.15;
    ctx.strokeStyle = "#22304d";
    ctx.beginPath(); ctx.moveTo(0, c.height - 0.5); ctx.lineTo(c.width, c.height - 0.5); ctx.stroke();
    ctx.strokeStyle = "#facc15"; ctx.lineWidth = 2; ctx.beginPath();
    historique.forEach((p, i) => {
      const x = (i / 119) * c.width, y = c.height - (p / max) * c.height;
      i ? ctx.lineTo(x, y) : ctx.moveTo(x, y);
    });
    ctx.stroke();
    ctx.fillStyle = "#94a3b8"; ctx.font = "12px system-ui";
    ctx.fillText(nombre(max, 0) + " W", 4, 14);
  }

  function duree(s) {
    const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60);
    return h ? `${h} h ${m} min` : `${m} min ${Math.floor(s % 60)} s`;
  }

  async function lire() {
    try {
      const r = await fetch("/mesures", { cache: "no-store" });
      derniere = await r.json();
      $("puissance").textContent = nombre(derniere.puissance, 0);
      $("tension").textContent = nombre(derniere.tension, 0);
      $("courant").textContent = nombre(derniere.courant, 2) + " A";
      $("energie").textContent = nombre(derniere.energie, 3) + " kWh";
      $("duree").textContent = duree(derniere.depuis);
      $("etat").textContent = derniere.simulation ? "Mode simulation (pas de pince branchée)" : "Mesure en direct";
      $("etat").className = "etat";
      historique.push(derniere.puissance);
      if (historique.length > 120) historique.shift();
      afficherCout();
      dessiner();
    } catch (e) {
      $("etat").textContent = "Connexion perdue avec le compteur…";
      $("etat").className = "etat alerte";
    }
  }
  lire();
  setInterval(lire, 1000);
</script>
</body>
</html>
)rawliteral";
