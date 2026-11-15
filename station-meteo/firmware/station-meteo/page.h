/*
  LE TABLEAU DE BORD DE LA STATION MÉTÉO (page web servie par l'ESP32)
  Même style que les autres projets : sable, terre cuite, des phrases et des conseils.
  /mesures est lu toutes les 5 s, /courbe toutes les 5 min.
*/
#pragma once

const char PAGE_METEO[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="theme-color" content="#f6efe4">
<title>Station météo</title>
<style>
  :root {
    --fond: #f6efe4; --carte: #fffaf3; --texte: #3a2e26; --doux: #8a7769; --trait: #eadfce;
    --accent: #d9693a; --accent-pale: #f6dccb; --soleil: #e6a23c; --mer: #2f7d78; --sable: #b08d5f;
    --ombre: 0 1px 2px rgba(90,60,30,.06), 0 8px 24px rgba(90,60,30,.07);
    --arrondi: 22px; color-scheme: light;
    /* Couleurs des niveaux : du calme (bleu-vert) au danger (rouge brique) */
    --n0: #2f7d78; --n1: #8a9a3c; --n2: #e6a23c; --n3: #d9693a; --n4: #b4442c; --n5: #7d2a2a;
  }
  @media (prefers-color-scheme: dark) {
    :root {
      --fond: #1f1a17; --carte: #2a2420; --texte: #f1e7db; --doux: #b3a293; --trait: #3b332d;
      --accent: #f08a5b; --accent-pale: #4a3022; --soleil: #f0b55a; --mer: #6cc3bb; --sable: #cfae80;
      --n0: #6cc3bb; --n1: #b5c46a; --n2: #f0b55a; --n3: #f08a5b; --n4: #e0664d; --n5: #d05050;
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
  .nom:hover, .nom:focus { border-bottom-color: var(--accent-pale); outline: none; }
  .direct { display: flex; align-items: center; gap: 8px; margin: 6px 0 0; color: var(--doux); font-size: .85rem; }
  .direct::before { content: ""; width: 8px; height: 8px; border-radius: 50%; background: var(--mer); }
  .direct.perdu { color: var(--accent); }
  .direct.perdu::before { background: var(--accent); }

  .carte { padding: 18px 20px; border-radius: var(--arrondi); background: var(--carte); box-shadow: var(--ombre); }
  .titre-carte { margin: 0 0 6px; color: var(--doux); font-size: .9rem; }
  .grand { margin: 0; font-size: 3.4rem; font-weight: 700; line-height: 1; font-variant-numeric: tabular-nums; letter-spacing: -.02em; }
  .grand small { margin-left: 6px; font-size: 1.3rem; font-weight: 600; color: var(--doux); }
  .phrase { margin: 8px 0 0; }
  .detail { margin: 6px 0 0; color: var(--doux); font-size: .88rem; }
  .ligne { display: flex; flex-wrap: wrap; gap: 6px 18px; margin: 10px 0 0; color: var(--doux); font-size: .92rem; }
  .ligne strong { color: var(--texte); }

  /* Le conseil : une bande douce, avec la couleur du niveau sur le côté */
  .conseil { margin: 12px 0 0; padding: 10px 12px; border-left: 4px solid var(--niveau, var(--n0)); border-radius: 6px 12px 12px 6px; background: var(--fond); }
  .conseil strong { color: var(--niveau, var(--n0)); }

  /* Échelle de l'air : 6 cases, la case actuelle est pleine */
  .echelle { display: grid; grid-template-columns: repeat(6, 1fr); gap: 4px; margin-top: 12px; }
  .echelle span { height: 8px; border-radius: 99px; background: var(--trait); }
  .echelle span.actif { background: var(--niveau); }

  canvas { width: 100%; height: 110px; display: block; }
  .graphe + .graphe { margin-top: 10px; }
  .pied { margin: 4px 6px 0; color: var(--doux); font-size: .8rem; text-align: center; }
</style>
</head>
<body>
<main>
  <header>
    <p class="bonjour">La météo de</p>
    <input id="lieu" class="nom" aria-label="Nom du lieu" value="ma terrasse" maxlength="30">
    <p class="direct" id="statut" role="status">Connexion à la station…</p>
  </header>

  <section class="carte" aria-labelledby="t-chaleur">
    <h2 class="titre-carte" id="t-chaleur">Dehors</h2>
    <p class="grand"><span id="temperature">–</span><small>°C</small></p>
    <p class="phrase" id="ressenti"></p>
    <p class="ligne"><span>Humidité <strong id="humidite">–</strong></span><span>Pression <strong id="pression">–</strong></span></p>
    <p class="conseil" id="conseil-chaleur"></p>
  </section>

  <section class="carte" aria-labelledby="t-air">
    <h2 class="titre-carte" id="t-air">L'air</h2>
    <p class="grand"><span id="pm25">–</span><small>µg/m³</small></p>
    <p class="phrase">de particules fines (PM2,5)</p>
    <div class="echelle" id="echelle" aria-hidden="true"><span></span><span></span><span></span><span></span><span></span><span></span></div>
    <p class="conseil" id="conseil-air">Première mesure de l'air dans moins d'une minute…</p>
    <p class="ligne"><span>Poussière (PM10) <strong id="pm10">–</strong></span><span>PM1 <strong id="pm1">–</strong></span></p>
    <p class="detail" id="age"></p>
  </section>

  <section class="carte" aria-labelledby="t-temps">
    <h2 class="titre-carte" id="t-temps">Le temps qui vient</h2>
    <p class="phrase" id="tendance">Il faut 3 heures de mesures pour voir la tendance de la pression.</p>
  </section>

  <section class="carte" aria-labelledby="t-courbes">
    <h2 class="titre-carte" id="t-courbes">Les dernières 24 heures</h2>
    <div class="graphe"><p class="detail">Température (°C)</p><canvas id="c-temp" width="480" height="110"></canvas></div>
    <div class="graphe"><p class="detail">Particules fines PM2,5 (µg/m³)</p><canvas id="c-air" width="480" height="110"></canvas></div>
  </section>

  <p class="pied">Seuils : chaleur ressentie selon la NOAA, particules selon l'EPA (2024). Les mesures restent dans ta maison.</p>
</main>

<script>
  const $ = (id) => document.getElementById(id);
  const nombre = (v, d = 0) => v.toLocaleString("fr-FR", { minimumFractionDigits: d, maximumFractionDigits: d });
  let derniere = null, points = [];

  try { const l = localStorage.getItem("lieu"); if (l) $("lieu").value = l; } catch (e) {}
  $("lieu").addEventListener("input", () => { try { localStorage.setItem("lieu", $("lieu").value); } catch (e) {} });

  const CHALEUR = [
    ["Rien de particulier", "profite de ta journée."],
    ["Prudence", "fatigue possible en cas d'effort prolongé au soleil."],
    ["Grande prudence", "bois souvent, évite l'effort aux heures les plus chaudes."],
    ["Danger", "coup de chaleur possible. Reste à l'ombre et bois beaucoup."],
    ["Danger extrême", "évite de sortir, surtout les enfants et les personnes âgées."],
  ];
  const AIR = [
    ["Bon", "l'air est propre."],
    ["Moyen", "acceptable, sauf pour les personnes très sensibles."],
    ["Mauvais pour les personnes sensibles", "les enfants, les personnes âgées et les asthmatiques devraient limiter l'effort dehors."],
    ["Mauvais", "tout le monde peut être gêné. Ferme les fenêtres."],
    ["Très mauvais", "reste à l'intérieur, fenêtres fermées."],
    ["Dangereux", "reste à l'intérieur ; si tu dois sortir, porte un masque FFP2."],
  ];
  const couleur = (n) => `var(--n${n})`;

  function afficher(m) {
    $("temperature").textContent = m.temperature === null ? "–" : nombre(m.temperature);
    $("humidite").textContent = m.humidite === null ? "–" : `${nombre(m.humidite)} %`;
    $("pression").textContent = m.pression === null ? "–" : `${nombre(m.pression)} hPa`;
    if (m.ressenti === null) {
      $("ressenti").textContent = "Le capteur de température ne répond pas.";
    } else {
      const ecart = Math.round(m.ressenti - m.temperature);
      $("ressenti").innerHTML = ecart >= 1
        ? `Ressenti <strong>${nombre(m.ressenti)} °C</strong> : l'humidité ajoute ${ecart} °C.`
        : `Ressenti ${nombre(m.ressenti)} °C.`;
    }
    const nc = Math.max(0, m.niveauChaleur);
    $("conseil-chaleur").style.setProperty("--niveau", couleur(nc));
    $("conseil-chaleur").innerHTML = `<strong>${CHALEUR[nc][0]}</strong> : ${CHALEUR[nc][1]}`;

    if (m.air) {
      // On garde le pire des deux niveaux : PM2,5 (fumées, pollution) et PM10 (poussière, sable).
      const na = Math.max(m.niveauPM25, m.niveauPM10);
      const cause = m.niveauPM10 > m.niveauPM25 ? " (surtout de la poussière)" : "";
      $("pm25").textContent = nombre(m.pm25);
      $("pm10").textContent = `${nombre(m.pm10)} µg/m³`;
      $("pm1").textContent = `${nombre(m.pm1)} µg/m³`;
      $("conseil-air").style.setProperty("--niveau", couleur(na));
      $("conseil-air").innerHTML = `<strong>${AIR[na][0]}${cause}</strong> : ${AIR[na][1]}`;
      $("echelle").style.setProperty("--niveau", couleur(na));
      [...$("echelle").children].forEach((s, i) => s.classList.toggle("actif", i <= na));
      const min = Math.round(m.ageAir / 60);
      $("age").textContent = min < 1 ? "Mesuré à l'instant." : `Mesuré il y a ${min} min (une mesure toutes les 5 min).`;
    }

    if (m.tendance !== null) {
      const t = m.tendance;
      $("tendance").textContent = t <= -2
        ? `La pression baisse vite (${nombre(t, 1)} hPa en 3 h) : le temps pourrait changer, avec du vent et de la poussière.`
        : t >= 2
          ? `La pression remonte (+${nombre(t, 1)} hPa en 3 h) : le temps se stabilise.`
          : "La pression est stable : pas de changement de temps en vue.";
    }

    $("statut").textContent = m.simulation ? "Simulation : aucun capteur branché" : "En direct";
    $("statut").className = "direct";
  }

  /** Une petite courbe : valeurs (avec des trous possibles), couleur, et les heures en bas. */
  function courbe(id, valeurs, nomCouleur, minimum) {
    const c = $(id), ctx = c.getContext("2d"), s = getComputedStyle(document.documentElement);
    const col = (n) => s.getPropertyValue(n).trim();
    ctx.clearRect(0, 0, c.width, c.height);
    const ok = valeurs.filter((v) => v !== null);
    if (ok.length < 2) {
      ctx.fillStyle = col("--doux"); ctx.font = "13px system-ui";
      ctx.fillText("La courbe se remplit toutes les 5 minutes…", 4, c.height / 2);
      return;
    }
    const bas = c.height - 16, haut = 14;
    const min = Math.min(minimum ?? Infinity, ...ok), max = Math.max(...ok, min + 1);
    const x = (i) => (i / Math.max(1, valeurs.length - 1)) * c.width;
    const y = (v) => bas - ((v - min) / (max - min)) * (bas - haut);
    ctx.strokeStyle = col(nomCouleur); ctx.lineWidth = 2.5; ctx.lineJoin = "round";
    ctx.beginPath(); let commence = false;
    valeurs.forEach((v, i) => { if (v === null) { commence = false; return; } commence ? ctx.lineTo(x(i), y(v)) : ctx.moveTo(x(i), y(v)); commence = true; });
    ctx.stroke();
    ctx.fillStyle = col("--doux"); ctx.font = "11px system-ui";
    ctx.fillText(`${nombre(max)}`, 2, 11);
    ctx.fillText(`${nombre(min)}`, 2, bas - 2);
    // Les heures, si la station connaît l'heure : un point toutes les 5 min, le dernier = maintenant.
    if (derniere && derniere.heure) {
      const fin = derniere.heure * 1000, debut = fin - (valeurs.length - 1) * 300000;
      for (let h = Math.ceil(debut / 21600000) * 21600000; h <= fin; h += 21600000) {
        const texte = new Date(h).toLocaleTimeString("fr-FR", { hour: "2-digit", minute: "2-digit", timeZone: "Africa/Djibouti" });
        ctx.fillText(texte, Math.min(x((h - debut) / 300000), c.width - 30), c.height - 2);
      }
    }
  }

  function dessiner() {
    courbe("c-temp", points.map((p) => p[0]), "--accent");
    courbe("c-air", points.map((p) => p[2]), "--sable", 0);
  }

  async function lireMesures() {
    try {
      derniere = await (await fetch("/mesures", { cache: "no-store" })).json();
      afficher(derniere);
    } catch (e) {
      $("statut").textContent = "La station ne répond plus… vérifie qu'elle est allumée";
      $("statut").className = "direct perdu";
    }
  }
  async function lireCourbe() {
    try { points = (await (await fetch("/courbe", { cache: "no-store" })).json()).points; dessiner(); } catch (e) {}
  }
  lireMesures().then(lireCourbe);
  setInterval(lireMesures, 5000);
  setInterval(lireCourbe, 300000);
</script>
</body>
</html>
)rawliteral";
