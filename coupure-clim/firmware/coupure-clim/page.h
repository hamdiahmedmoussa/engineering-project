/*
  LE TABLEAU DE BORD DE LA CLIM (page web servie par l'ESP32)
  Même style que le compteur et le suivi solaire : sable, terre cuite, des phrases.
  /etat est lu chaque seconde ; les boutons envoient /commande (POST).
*/
#pragma once

const char PAGE_CLIM[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="theme-color" content="#f6efe4">
<title>Coupure clim</title>
<style>
  :root {
    --fond: #f6efe4; --carte: #fffaf3; --texte: #3a2e26; --doux: #8a7769; --trait: #eadfce;
    --accent: #d9693a; --accent-pale: #f6dccb;
    --froid: #3f86b8; --froid-pale: #dbeaf5;     /* la clim qui tourne */
    --mer: #2f7d78; --mer-pale: #d5ece9;
    --ombre: 0 1px 2px rgba(90,60,30,.06), 0 8px 24px rgba(90,60,30,.07);
    --arrondi: 22px; color-scheme: light;
  }
  @media (prefers-color-scheme: dark) {
    :root {
      --fond: #1f1a17; --carte: #2a2420; --texte: #f1e7db; --doux: #b3a293; --trait: #3b332d;
      --accent: #f08a5b; --accent-pale: #4a3022; --froid: #7fb6de; --froid-pale: #1f3442;
      --mer: #6cc3bb; --mer-pale: #1d3634;
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
  .nom:hover, .nom:focus { border-bottom-color: var(--froid-pale); outline: none; }
  .direct { display: flex; align-items: center; gap: 8px; margin: 6px 0 0; color: var(--doux); font-size: .85rem; }
  .direct::before { content: ""; width: 8px; height: 8px; border-radius: 50%; background: var(--mer); }
  .direct.perdu { color: var(--accent); }
  .direct.perdu::before { background: var(--accent); }

  .carte { padding: 18px 20px; border-radius: var(--arrondi); background: var(--carte); box-shadow: var(--ombre); }
  .titre-carte { margin: 0 0 6px; color: var(--doux); font-size: .9rem; }
  .etat-clim { margin: 0; font-size: 1.9rem; font-weight: 700; line-height: 1.15; }
  .etat-clim.on { color: var(--froid); }
  .phrase { margin: 8px 0 0; }
  .detail { margin: 6px 0 0; color: var(--doux); font-size: .88rem; }
  .alerte { border: 2px solid var(--accent); background: var(--accent-pale); }
  .alerte .titre-carte, .alerte strong { color: var(--accent); }
  [hidden] { display: none !important; }

  /* Le compte à rebours avant extinction : une barre douce qui se vide. */
  .rebours { height: 8px; margin-top: 12px; border-radius: 99px; background: var(--mer-pale); overflow: hidden; }
  .rebours span { display: block; height: 100%; border-radius: 99px; background: var(--mer); transition: width .8s linear; }

  .boutons { display: flex; gap: 10px; }
  .boutons button { flex: 1; }
  button {
    min-height: 52px; padding: 12px; border: 0; border-radius: 16px; font: inherit; font-weight: 700; cursor: pointer;
  }
  .b-eteindre { background: var(--accent-pale); color: var(--accent); }
  .b-allumer { background: var(--froid); color: #fff; }
  button:disabled { opacity: .5; cursor: default; }
  button:focus-visible { outline: 3px solid var(--texte); outline-offset: 2px; }

  .grand { margin: 0; font-size: 2rem; font-weight: 700; font-variant-numeric: tabular-nums; }
  .grand small { margin-left: 4px; font-size: 1.1rem; font-weight: 600; color: var(--doux); }

  ol { margin: 0; padding: 0; list-style: none; display: grid; gap: 10px; }
  li { display: grid; grid-template-columns: 4.2em 1fr; gap: 8px; font-size: .92rem; }
  li time { color: var(--doux); font-variant-numeric: tabular-nums; }
  .pied { margin: 4px 6px 0; color: var(--doux); font-size: .8rem; text-align: center; }
</style>
</head>
<body>
<main>
  <header>
    <p class="bonjour">La clim de</p>
    <input id="piece" class="nom" aria-label="Nom de la pièce" value="ma chambre" maxlength="30">
    <p class="direct" id="statut" role="status">Connexion au boîtier…</p>
  </header>

  <section class="carte alerte" id="carte-alerte" hidden aria-labelledby="t-alerte">
    <h2 class="titre-carte" id="t-alerte">Quelque chose ne va pas</h2>
    <p class="phrase" id="alerte"></p>
  </section>

  <section class="carte" aria-labelledby="t-clim">
    <h2 class="titre-carte" id="t-clim">En ce moment</h2>
    <p class="etat-clim" id="etat">–</p>
    <p class="phrase" id="piece-temp"></p>
    <div class="boutons" style="margin-top:16px">
      <button type="button" class="b-eteindre" id="b-eteindre">Éteindre</button>
      <button type="button" class="b-allumer" id="b-allumer">Allumer à 25 °C</button>
    </div>
  </section>

  <section class="carte" aria-labelledby="t-presence">
    <h2 class="titre-carte" id="t-presence">Dans la pièce</h2>
    <p class="phrase" id="presence">–</p>
    <div class="rebours" id="rebours" hidden aria-hidden="true"><span id="rebours-barre"></span></div>
    <p class="detail" id="regles"></p>
  </section>

  <section class="carte" aria-labelledby="t-evite">
    <h2 class="titre-carte" id="t-evite" >Clim évitée aujourd'hui</h2>
    <p class="grand" id="evite">–</p>
    <p class="phrase" id="evite-phrase"></p>
  </section>

  <section class="carte" aria-labelledby="t-journal">
    <h2 class="titre-carte" id="t-journal">Ce qui s'est passé</h2>
    <ol id="journal"></ol>
  </section>

  <p class="pied">Le boîtier parle à la clim en infrarouge, comme sa télécommande : aucun fil électrique touché.</p>
</main>

<script>
  const $ = (id) => document.getElementById(id);
  const nombre = (v, d) => v.toLocaleString("fr-FR", { minimumFractionDigits: d, maximumFractionDigits: d });
  let derniere = null;

  try { const p = localStorage.getItem("piece"); if (p) $("piece").value = p; } catch (e) {}
  $("piece").addEventListener("input", () => { try { localStorage.setItem("piece", $("piece").value); } catch (e) {} });

  async function commande(action) {
    $("b-eteindre").disabled = $("b-allumer").disabled = true;
    try { await fetch("/commande?action=" + action, { method: "POST", headers: { "X-Tableau": "1" } }); } catch (e) {}
    await lire();
    $("b-eteindre").disabled = $("b-allumer").disabled = false;
  }
  $("b-eteindre").addEventListener("click", () => commande("eteindre"));
  $("b-allumer").addEventListener("click", () => commande("allumer"));

  function duree(min) {
    const h = Math.floor(min / 60), m = Math.round(min % 60);
    return h ? `${h} h ${String(m).padStart(2, "0")}` : `${m} min`;
  }

  function heure(e, m) {
    if (e.h) return new Date(e.h * 1000).toLocaleTimeString("fr-FR", { hour: "2-digit", minute: "2-digit", timeZone: "Africa/Djibouti" });
    const il = Math.max(0, m.depuis - e.d);
    return il < 60 ? "à l'instant" : `−${Math.round(il / 60)} min`;
  }

  function afficher(m) {
    $("etat").textContent = m.allumee ? `Allumée, réglée sur ${m.consigne} °C` : "Éteinte";
    $("etat").className = "etat-clim" + (m.allumee ? " on" : "");
    $("piece-temp").textContent = m.temperature === null
      ? "Le capteur de température ne répond pas."
      : `Il fait ${nombre(m.temperature, 0)} °C dans la pièce` + (m.humidite === null ? "." : `, ${nombre(m.humidite, 0)} % d'humidité.`);
    $("b-eteindre").hidden = !m.allumee;
    $("b-allumer").hidden = m.allumee;
    $("b-allumer").textContent = `Allumer à ${m.allumage} °C`;

    // Présence et compte à rebours
    const reste = m.delai - m.sansPresence;
    if (m.presence) $("presence").textContent = "Quelqu'un est là.";
    else if (!m.allumee) $("presence").textContent = `Personne depuis ${duree(m.sansPresence)}.`;
    else if (m.nuit) $("presence").textContent = `Personne depuis ${duree(m.sansPresence)}, mais c'est la nuit : la clim reste allumée.`;
    else $("presence").textContent = `Personne depuis ${duree(m.sansPresence)}. La clim s'éteindra dans ${duree(Math.max(0, reste))} si personne ne revient.`;
    const enAttente = m.allumee && !m.presence && !m.nuit;
    $("rebours").hidden = !enAttente;
    $("rebours-barre").style.width = enAttente ? Math.max(0, (reste / m.delai) * 100) + "%" : "100%";
    $("regles").textContent =
      `Elle s'éteint seule après ${m.delai} min sans personne, jamais entre ${m.nuitDebut} h et ${m.nuitFin} h.` +
      (m.eco ? ` Réglée plus froid que ${m.eco} °C, elle remonte à ${m.eco} °C.` : "");

    // Alerte
    $("carte-alerte").hidden = !m.alerte;
    if (m.alerte) $("alerte").innerHTML =
      `La pièce est encore à <strong>${nombre(m.temperature, 0)} °C</strong> malgré la clim. ` +
      "Vérifie le filtre (souvent encrassé par la poussière), la fenêtre, ou fais contrôler la clim.";

    // Temps évité
    const min = m.evite / 60;
    $("t-evite").textContent = m.heure ? "Clim évitée aujourd'hui" : "Clim évitée depuis la mise en marche";
    $("evite").innerHTML = min < 1 ? "0 <small>min</small>" : duree(min).replace(/(\d+) min$/, "$1 <small>min</small>").replace(/ h /, " <small>h</small> ");
    const kwh = (m.puissanceClim * min) / 60 / 1000;
    $("evite-phrase").textContent = min < 1
      ? "Le boîtier n'a encore rien éteint."
      : `Environ ${nombre(kwh, 1)} kWh économisés (clim de ${nombre(m.puissanceClim, 0)} W).`;

    // Journal (les textes viennent du boîtier : insérés comme texte, jamais comme HTML)
    $("journal").replaceChildren(...m.journal.map((e) => {
      const li = document.createElement("li"), t = document.createElement("time"), s = document.createElement("span");
      t.textContent = heure(e, m); s.textContent = e.t; li.append(t, s); return li;
    }));

    $("statut").textContent = m.simulation ? "Simulation : 1 minute dure 1 seconde" : "En direct";
    $("statut").className = "direct";
  }

  async function lire() {
    try {
      derniere = await (await fetch("/etat", { cache: "no-store" })).json();
      afficher(derniere);
    } catch (e) {
      $("statut").textContent = "Le boîtier ne répond plus… vérifie qu'il est allumé";
      $("statut").className = "direct perdu";
    }
  }
  lire();
  setInterval(lire, 1000);
</script>
</body>
</html>
)rawliteral";
