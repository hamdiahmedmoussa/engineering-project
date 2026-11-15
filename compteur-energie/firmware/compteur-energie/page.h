/*
  LE TABLEAU DE BORD (page web servie par l'ESP32 lui-même)
  Le téléphone ouvre l'adresse de l'ESP32 ; cette page demande les mesures (/mesures)
  chaque seconde et les affiche. Pas d'internet nécessaire : tout reste dans la maison.

  Style : couleurs de sable et de terre cuite, coins arrondis, des PHRASES plutôt
  qu'un tableau de chiffres (« Ta clim consomme 1 218 W en ce moment »).
  Le nom de l'appareil et le tarif sont saisis sur la page et gardés dans le téléphone.
*/
#pragma once

const char PAGE_TABLEAU[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="theme-color" content="#f6efe4">
<title>Compteur d'énergie</title>
<style>
  :root {
    --fond: #f6efe4;         /* sable */
    --carte: #fffaf3;
    --texte: #3a2e26;        /* brun doux, moins dur que le noir */
    --doux: #8a7769;
    --trait: #eadfce;
    --accent: #d9693a;       /* terre cuite */
    --accent-pale: #f6dccb;
    --mer: #2f7d78;          /* bleu-vert, pour ce qui est « bien » */
    --ombre: 0 1px 2px rgba(90, 60, 30, .06), 0 8px 24px rgba(90, 60, 30, .07);
    --arrondi: 22px;
    color-scheme: light;
  }
  @media (prefers-color-scheme: dark) {
    :root {
      --fond: #1f1a17; --carte: #2a2420; --texte: #f1e7db; --doux: #b3a293; --trait: #3b332d;
      --accent: #f08a5b; --accent-pale: #4a3022; --mer: #6cc3bb;
      --ombre: 0 1px 2px rgba(0,0,0,.3), 0 8px 24px rgba(0,0,0,.25); color-scheme: dark;
    }
  }
  * { box-sizing: border-box; }
  body {
    margin: 0; padding: 20px 16px 32px; background: var(--fond); color: var(--texte);
    font-family: ui-rounded, "SF Pro Rounded", "Segoe UI", system-ui, sans-serif; line-height: 1.45;
  }
  main { max-width: 520px; margin: 0 auto; display: grid; gap: 14px; }

  header { padding: 4px 6px 6px; }
  .bonjour { margin: 0; color: var(--doux); font-size: .95rem; }
  /* Le nom de l'appareil se modifie directement dans le titre. */
  .appareil {
    width: 100%; margin: 2px 0 0; padding: 0; border: 0; border-bottom: 2px dashed transparent;
    background: none; color: var(--texte); font-family: inherit; font-size: 1.6rem; font-weight: 700; line-height: 1.2;
  }
  .appareil:hover, .appareil:focus { border-bottom-color: var(--accent-pale); outline: none; }
  .direct { display: flex; align-items: center; gap: 8px; margin: 6px 0 0; color: var(--doux); font-size: .85rem; }
  .direct::before { content: ""; width: 8px; height: 8px; border-radius: 50%; background: var(--mer); }
  .direct.perdu { color: var(--accent); }
  .direct.perdu::before { background: var(--accent); }

  .carte { padding: 18px 20px; border-radius: var(--arrondi); background: var(--carte); box-shadow: var(--ombre); }
  .titre-carte { margin: 0 0 6px; color: var(--doux); font-size: .9rem; }
  .grand { margin: 0; font-size: 3.4rem; font-weight: 700; line-height: 1; font-variant-numeric: tabular-nums; letter-spacing: -.02em; }
  .grand small { margin-left: 4px; font-size: 1.2rem; font-weight: 600; color: var(--doux); }
  .phrase { margin: 8px 0 0; font-size: 1rem; }
  .phrase strong { font-weight: 700; }
  .moyen { font-size: 2rem; font-weight: 700; margin: 0; font-variant-numeric: tabular-nums; }
  .cout { color: var(--accent); }

  .tarif { display: flex; align-items: center; justify-content: space-between; gap: 12px; margin-top: 14px; padding-top: 14px; border-top: 1px solid var(--trait); color: var(--doux); font-size: .9rem; }
  .tarif input {
    width: 6.5em; padding: 10px 12px; border: 1px solid var(--trait); border-radius: 14px;
    background: var(--fond); color: var(--texte); font: inherit; font-weight: 600; text-align: right;
  }
  .tarif input:focus { outline: 2px solid var(--accent); outline-offset: 1px; }

  canvas { width: 100%; height: 130px; display: block; }
  .pied { margin: 4px 6px 0; color: var(--doux); font-size: .8rem; text-align: center; }
</style>
</head>
<body>
<main>
  <header>
    <p class="bonjour">Ce que consomme</p>
    <input id="appareil" class="appareil" aria-label="Nom de l'appareil mesuré" value="ma clim" maxlength="30">
    <p class="direct" id="etat" role="status">Connexion au compteur…</p>
  </header>

  <section class="carte" aria-labelledby="t-maintenant">
    <h2 class="titre-carte" id="t-maintenant">En ce moment</h2>
    <p class="grand"><span id="puissance">–</span><small>W</small></p>
    <p class="phrase">soit <strong id="courant">–</strong> sous <span id="tension">220</span> V</p>
  </section>

  <section class="carte" aria-labelledby="t-depuis">
    <h2 class="titre-carte" id="t-depuis">Depuis la mise en marche (<span id="duree">–</span>)</h2>
    <p class="moyen"><span id="energie">–</span> <small>kWh</small></p>
    <p class="phrase" id="cout-total"></p>
  </section>

  <section class="carte" aria-labelledby="t-rythme">
    <h2 class="titre-carte" id="t-rythme">À ce rythme</h2>
    <p class="phrase" id="estimation">Indique le prix du kWh écrit sur ta facture pour voir ce que ça coûte.</p>
    <label class="tarif">Prix du kWh (FDJ)
      <input id="tarif" type="number" min="0" step="0.1" inputmode="decimal" placeholder="facture">
    </label>
  </section>

  <section class="carte" aria-labelledby="t-courbe">
    <h2 class="titre-carte" id="t-courbe">Les 2 dernières minutes</h2>
    <canvas id="courbe" width="480" height="130" role="img" aria-label="Courbe de la puissance des 2 dernières minutes"></canvas>
  </section>

  <p class="pied">Puissance estimée : courant mesuré × 220 V. Le nom et le prix restent sur ce téléphone.</p>
</main>

<script>
  const $ = (id) => document.getElementById(id);
  const nombre = (v, d) => v.toLocaleString("fr-FR", { minimumFractionDigits: d, maximumFractionDigits: d });
  // Le nom tapé par l'utilisateur est inséré comme TEXTE, jamais comme HTML.
  const texte = (t) => t.replace(/[&<>"']/g, (c) => `&#${c.charCodeAt(0)};`);
  const historique = [];
  let derniere = null;

  // Le nom de l'appareil et le tarif sont gardés dans le téléphone (localStorage).
  function garder(id, cle) {
    try { const v = localStorage.getItem(cle); if (v !== null) $(id).value = v; } catch (e) {}
    $(id).addEventListener("input", () => {
      try { localStorage.setItem(cle, $(id).value); } catch (e) {}
      afficherCout();
    });
  }
  garder("appareil", "appareil");
  garder("tarif", "tarif");

  function afficherCout() {
    const tarif = parseFloat($("tarif").value);
    if (!derniere) return;
    if (!(tarif > 0)) {
      $("cout-total").textContent = "";
      $("estimation").textContent = "Indique le prix du kWh écrit sur ta facture pour voir ce que ça coûte.";
      return;
    }
    // Moyenne depuis la mise en marche : une clim s'allume et s'éteint, la puissance du moment varie trop.
    const parJourKWh = derniere.depuis >= 60 ? (derniere.energie / derniere.depuis) * 86400 : (derniere.puissance * 24) / 1000;
    const total = derniere.energie * tarif;
    $("cout-total").innerHTML = total < 1
      ? "soit <strong class=\"cout\">moins de 1 FDJ</strong> pour l'instant"
      : `soit environ <strong class="cout">${nombre(total, 0)} FDJ</strong>`;
    $("estimation").innerHTML =
      `${texte($("appareil").value) || "L'appareil"} coûte environ <strong class="cout">${nombre(parJourKWh * tarif, 0)} FDJ par jour</strong>, ` +
      `soit <strong class="cout">${nombre(parJourKWh * 30 * tarif, 0)} FDJ par mois</strong>.`;
  }

  function dessiner() {
    const c = $("courbe"), ctx = c.getContext("2d"), s = getComputedStyle(document.documentElement);
    ctx.clearRect(0, 0, c.width, c.height);
    if (historique.length < 2) return;
    const max = Math.max(100, ...historique) * 1.2;
    const points = historique.map((p, i) => [(i / 119) * c.width, c.height - 4 - (p / max) * (c.height - 8)]);
    // Une aire douce sous la courbe, plutôt qu'un simple trait.
    const degrade = ctx.createLinearGradient(0, 0, 0, c.height);
    degrade.addColorStop(0, s.getPropertyValue("--accent-pale"));
    degrade.addColorStop(1, s.getPropertyValue("--carte"));
    ctx.beginPath();
    points.forEach(([x, y], i) => (i ? ctx.lineTo(x, y) : ctx.moveTo(x, y)));
    ctx.lineTo(points[points.length - 1][0], c.height); ctx.lineTo(0, c.height); ctx.closePath();
    ctx.fillStyle = degrade; ctx.fill();
    ctx.beginPath();
    points.forEach(([x, y], i) => (i ? ctx.lineTo(x, y) : ctx.moveTo(x, y)));
    ctx.strokeStyle = s.getPropertyValue("--accent"); ctx.lineWidth = 2.5; ctx.lineJoin = "round"; ctx.stroke();
  }

  function duree(s) {
    const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60);
    return h ? `${h} h ${m} min` : m ? `${m} min` : `${Math.floor(s)} s`;
  }

  async function lire() {
    try {
      const r = await fetch("/mesures", { cache: "no-store" });
      derniere = await r.json();
      $("puissance").textContent = nombre(derniere.puissance, 0);
      $("tension").textContent = nombre(derniere.tension, 0);
      $("courant").textContent = nombre(derniere.courant, 1) + " A";
      $("energie").textContent = nombre(derniere.energie, 3);
      $("duree").textContent = duree(derniere.depuis);
      $("etat").textContent = derniere.simulation ? "Simulation : aucune pince branchée" : "En direct";
      $("etat").className = "direct";
      historique.push(derniere.puissance);
      if (historique.length > 120) historique.shift();
      afficherCout();
      dessiner();
    } catch (e) {
      $("etat").textContent = "Le compteur ne répond plus… vérifie qu'il est allumé";
      $("etat").className = "direct perdu";
    }
  }
  lire();
  setInterval(lire, 1000);
</script>
</body>
</html>
)rawliteral";
