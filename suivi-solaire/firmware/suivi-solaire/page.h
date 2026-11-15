/*
  LE TABLEAU DE BORD SOLAIRE (page web servie par l'ESP32)
  Même style que le compteur d'énergie : sable, terre cuite, des phrases simples.
  /mesures est lu chaque seconde, /courbe (une valeur par minute) chaque minute.
  Le nom du panneau et la date du dernier nettoyage restent dans le téléphone.
*/
#pragma once

const char PAGE_SOLAIRE[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="theme-color" content="#f6efe4">
<title>Suivi solaire</title>
<style>
  :root {
    --fond: #f6efe4; --carte: #fffaf3; --texte: #3a2e26; --doux: #8a7769; --trait: #eadfce;
    --accent: #d9693a; --accent-pale: #f6dccb;
    --soleil: #e6a23c; --soleil-pale: #fbe7c4;   /* la production */
    --mer: #2f7d78;                               /* ce qui va bien */
    --ombre: 0 1px 2px rgba(90,60,30,.06), 0 8px 24px rgba(90,60,30,.07);
    --arrondi: 22px; color-scheme: light;
  }
  @media (prefers-color-scheme: dark) {
    :root {
      --fond: #1f1a17; --carte: #2a2420; --texte: #f1e7db; --doux: #b3a293; --trait: #3b332d;
      --accent: #f08a5b; --accent-pale: #4a3022; --soleil: #f0b55a; --soleil-pale: #4a3a1e; --mer: #6cc3bb;
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
  .nom {
    width: 100%; margin: 2px 0 0; padding: 0; border: 0; border-bottom: 2px dashed transparent; background: none;
    color: var(--texte); font-family: inherit; font-size: 1.6rem; font-weight: 700; line-height: 1.2;
  }
  .nom:hover, .nom:focus { border-bottom-color: var(--soleil-pale); outline: none; }
  .direct { display: flex; align-items: center; gap: 8px; margin: 6px 0 0; color: var(--doux); font-size: .85rem; }
  .direct::before { content: ""; width: 8px; height: 8px; border-radius: 50%; background: var(--mer); }
  .direct.perdu { color: var(--accent); }
  .direct.perdu::before { background: var(--accent); }

  .carte { padding: 18px 20px; border-radius: var(--arrondi); background: var(--carte); box-shadow: var(--ombre); }
  .titre-carte { margin: 0 0 6px; color: var(--doux); font-size: .9rem; }
  .grand { margin: 0; font-size: 3.4rem; font-weight: 700; line-height: 1; font-variant-numeric: tabular-nums; letter-spacing: -.02em; }
  .grand small, .moyen small { margin-left: 4px; font-size: 1.2rem; font-weight: 600; color: var(--doux); }
  .moyen { margin: 0; font-size: 2rem; font-weight: 700; font-variant-numeric: tabular-nums; }
  .phrase { margin: 8px 0 0; }
  .detail { margin: 6px 0 0; color: var(--doux); font-size: .88rem; }
  .chaud { color: var(--accent); }
  .bien { color: var(--mer); }

  /* Jauge douce : la production par rapport à la puissance annoncée */
  .jauge { height: 10px; margin-top: 14px; border-radius: 99px; background: var(--soleil-pale); overflow: hidden; }
  .jauge span { display: block; height: 100%; width: 0; border-radius: 99px; background: var(--soleil); transition: width .6s ease; }

  canvas { width: 100%; height: 160px; display: block; }
  .legende { display: flex; gap: 16px; margin: 8px 0 0; color: var(--doux); font-size: .8rem; }
  .legende span::before { content: ""; display: inline-block; width: 10px; height: 10px; margin-right: 6px; border-radius: 3px; vertical-align: -1px; }
  .l-prod::before { background: var(--soleil); }
  .l-temp::before { height: 0; border-top: 2px dashed var(--accent); border-radius: 0; vertical-align: 3px; }
  .l-net::before { background: var(--mer); }

  button {
    margin-top: 12px; padding: 12px 18px; border: 0; border-radius: 16px; background: var(--mer); color: #fff;
    font: inherit; font-weight: 700; cursor: pointer;
  }
  button:focus-visible { outline: 3px solid var(--soleil); outline-offset: 2px; }
  .pied { margin: 4px 6px 0; color: var(--doux); font-size: .8rem; text-align: center; }
</style>
</head>
<body>
<main>
  <header>
    <p class="bonjour">Ce que produit</p>
    <input id="nom" class="nom" aria-label="Nom du panneau" value="mon panneau solaire" maxlength="30">
    <p class="direct" id="etat" role="status">Connexion au boîtier…</p>
  </header>

  <section class="carte" aria-labelledby="t-maintenant">
    <h2 class="titre-carte" id="t-maintenant">En ce moment</h2>
    <p class="grand"><span id="puissance">–</span><small>W</small></p>
    <p class="phrase" id="part"></p>
    <div class="jauge" aria-hidden="true"><span id="jauge"></span></div>
    <p class="detail" id="electrique"></p>
  </section>

  <section class="carte" aria-labelledby="t-chaleur">
    <h2 class="titre-carte" id="t-chaleur">La chaleur</h2>
    <p class="moyen"><span id="temperature">–</span><small>°C</small></p>
    <p class="phrase" id="chaleur"></p>
    <p class="detail" id="coef"></p>
  </section>

  <section class="carte" aria-labelledby="t-jour">
    <h2 class="titre-carte" id="t-jour">Aujourd'hui</h2>
    <p class="moyen"><span id="energie">–</span><small>Wh</small></p>
    <p class="phrase" id="telephones"></p>
  </section>

  <section class="carte" aria-labelledby="t-courbe">
    <h2 class="titre-carte" id="t-courbe">La journée</h2>
    <canvas id="courbe" width="480" height="160" role="img" aria-label="Production et température au fil de la journée"></canvas>
    <p class="legende"><span class="l-prod">Production</span><span class="l-temp">Température</span><span class="l-net">Nettoyage</span></p>
  </section>

  <section class="carte" aria-labelledby="t-poussiere">
    <h2 class="titre-carte" id="t-poussiere">La poussière</h2>
    <p class="phrase" id="nettoyage">Tu n'as pas encore noté de nettoyage.</p>
    <p class="detail">Après un nettoyage, compare la courbe à la même heure, par beau temps : la différence, c'est ce que la poussière te coûtait.</p>
    <button type="button" id="nettoye">J'ai nettoyé le panneau</button>
  </section>

  <p class="pied" id="pied">Les mesures restent dans ta maison : rien n'est envoyé sur internet.</p>
</main>

<script>
  const $ = (id) => document.getElementById(id);
  const nombre = (v, d) => v.toLocaleString("fr-FR", { minimumFractionDigits: d, maximumFractionDigits: d });
  let derniere = null, points = [];

  // Le nom et la date du dernier nettoyage restent dans le téléphone.
  const lireLocal = (cle) => { try { return localStorage.getItem(cle); } catch (e) { return null; } };
  const ecrireLocal = (cle, v) => { try { localStorage.setItem(cle, v); } catch (e) {} };
  if (lireLocal("panneau")) $("nom").value = lireLocal("panneau");
  $("nom").addEventListener("input", () => ecrireLocal("panneau", $("nom").value));

  function afficherNettoyage() {
    const quand = Number(lireLocal("nettoyage"));
    if (!quand) return;
    const jours = Math.floor((Date.now() - quand) / 86400000);
    $("nettoyage").textContent = jours === 0
      ? "Nettoyé aujourd'hui. Regarde si la courbe monte plus haut qu'hier à la même heure."
      : `Dernier nettoyage : il y a ${jours} jour${jours > 1 ? "s" : ""}.`;
  }
  $("nettoye").addEventListener("click", () => { ecrireLocal("nettoyage", String(Date.now())); afficherNettoyage(); dessiner(); });
  afficherNettoyage();

  function afficher(m) {
    $("puissance").textContent = nombre(m.puissance, 1);
    const pct = m.nominale > 0 ? (m.puissance / m.nominale) * 100 : 0;
    $("part").textContent = m.puissance < 0.2
      ? "Pas de production pour l'instant (nuit, ou panneau à l'ombre)."
      : `C'est ${nombre(pct, 0)} % des ${nombre(m.nominale, 0)} W annoncés sur l'étiquette.`;
    $("jauge").style.width = Math.min(100, pct) + "%";
    $("electrique").textContent = `${nombre(m.tension, 1)} V · ${nombre(m.courant, 2)} A`;

    if (m.temperature === null) {
      $("temperature").textContent = "–";
      $("chaleur").textContent = "La sonde de température ne répond pas.";
    } else {
      $("temperature").textContent = nombre(m.temperature, 0);
      $("chaleur").innerHTML = m.perte < 1
        ? `<span class="bien">Il est assez frais :</span> pas de perte due à la chaleur.`
        : `La chaleur lui fait perdre environ <strong class="chaud">${nombre(m.perte, 0)} %</strong>. ` +
          `À 25 °C, il donnerait <strong>${nombre(m.p25, 1)} W</strong>.`;
    }
    $("coef").textContent = `Ton panneau perd ${nombre(-m.coef, 2)} % par degré au-dessus de 25 °C.`;

    $("t-jour").textContent = m.heure ? "Aujourd'hui" : "Depuis la mise en marche";
    $("energie").textContent = nombre(m.energie, 1);
    // Une recharge de smartphone : environ 15 Wh (batterie de 4 000 mAh sous 3,85 V).
    const tel = Math.floor(m.energie / 15);
    $("telephones").textContent = tel >= 1 ? `De quoi recharger environ ${tel} téléphone${tel > 1 ? "s" : ""}.` : "";

    $("etat").textContent = m.simulation ? "Simulation : aucun panneau branché" : "En direct";
    $("etat").className = "direct";
  }

  function dessiner() {
    const c = $("courbe"), ctx = c.getContext("2d"), s = getComputedStyle(document.documentElement);
    const couleur = (n) => s.getPropertyValue(n).trim();
    ctx.clearRect(0, 0, c.width, c.height);
    const n = points.length;
    if (n < 2) {
      ctx.fillStyle = couleur("--doux"); ctx.font = "14px system-ui";
      ctx.fillText("La courbe se remplit minute après minute…", 8, c.height / 2);
      return;
    }
    const bas = c.height - 18, haut = 8;
    const x = (i) => (i / (n - 1)) * c.width;
    const maxP = Math.max(derniere ? derniere.nominale : 1, ...points.map((p) => p[0])) * 1.1;
    const y = (p) => bas - (p / maxP) * (bas - haut);

    // Production : une aire couleur soleil.
    const degrade = ctx.createLinearGradient(0, haut, 0, bas);
    degrade.addColorStop(0, couleur("--soleil")); degrade.addColorStop(1, couleur("--soleil-pale"));
    ctx.beginPath(); ctx.moveTo(0, bas);
    points.forEach((p, i) => ctx.lineTo(x(i), y(p[0])));
    ctx.lineTo(x(n - 1), bas); ctx.closePath(); ctx.fillStyle = degrade; ctx.globalAlpha = 0.85; ctx.fill(); ctx.globalAlpha = 1;

    // Température : une ligne fine (échelle de 20 à 80 °C).
    const yT = (t) => bas - ((t - 20) / 60) * (bas - haut);
    ctx.beginPath(); let commence = false;
    points.forEach((p, i) => { if (p[1] === null) return; commence ? ctx.lineTo(x(i), yT(p[1])) : ctx.moveTo(x(i), yT(p[1])); commence = true; });
    // En pointillés : on la distingue de la production même quand les deux courbes se rapprochent.
    ctx.strokeStyle = couleur("--accent"); ctx.lineWidth = 2; ctx.lineJoin = "round"; ctx.setLineDash([5, 4]); ctx.stroke(); ctx.setLineDash([]);

    // Les heures, si l'ESP32 connaît l'heure : le dernier point = maintenant, un point = une minute.
    ctx.fillStyle = couleur("--doux"); ctx.font = "11px system-ui";
    if (derniere && derniere.heure) {
      const fin = derniere.heure * 1000, debut = fin - (n - 1) * 60000;
      for (let h = Math.ceil(debut / 10800000) * 10800000; h <= fin; h += 10800000) {
        const i = (h - debut) / 60000;
        const texte = new Date(h).toLocaleTimeString("fr-FR", { hour: "2-digit", minute: "2-digit", timeZone: "Africa/Djibouti" });
        ctx.fillText(texte, Math.min(x(i), c.width - 30), c.height - 4);
      }
      // Le dernier nettoyage, s'il est dans la courbe.
      const nettoyage = Number(lireLocal("nettoyage"));
      if (nettoyage >= debut && nettoyage <= fin) {
        const xn = x((nettoyage - debut) / 60000);
        ctx.strokeStyle = couleur("--mer"); ctx.lineWidth = 2; ctx.setLineDash([4, 4]);
        ctx.beginPath(); ctx.moveTo(xn, haut); ctx.lineTo(xn, bas); ctx.stroke(); ctx.setLineDash([]);
      }
    } else {
      ctx.fillText(`${n} min de mesures`, 4, c.height - 4);
    }
  }

  async function lireMesures() {
    try {
      derniere = await (await fetch("/mesures", { cache: "no-store" })).json();
      afficher(derniere);
    } catch (e) {
      $("etat").textContent = "Le boîtier ne répond plus… vérifie qu'il est allumé";
      $("etat").className = "direct perdu";
    }
  }
  async function lireCourbe() {
    try {
      points = (await (await fetch("/courbe", { cache: "no-store" })).json()).points;
      dessiner();
    } catch (e) {}
  }
  lireMesures().then(lireCourbe);
  setInterval(lireMesures, 1000);
  setInterval(lireCourbe, 60000);
</script>
</body>
</html>
)rawliteral";
