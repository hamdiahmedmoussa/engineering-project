/*
  L'ÉCRAN PRINCIPAL
  À chaque détection de la main (environ 30 fois par seconde) :
    les 9 angles sont calculés, puis nettoyés par le filtre One Euro (tremblements, erreurs de détection).
  À chaque image d'animation (environ 60 fois par seconde) :
    1. la cible : les angles filtrés, ou la position figée (pause, main perdue, repos) ;
    2. les angles actuels rejoignent la cible sans dépasser la vitesse maximale des servos ;
    3. on dessine le bras, et on envoie les angles à l'ESP32 environ 50 fois par seconde.
*/
import { useEffect, useRef, useState } from "react";
import { limiterVitesse, mainVersAngles } from "./bras/angles";
import { Bras3D } from "./bras/Bras3D";
import { FiltreAngles } from "./bras/filtre";
import { aChange, ligneAngles } from "./bras/protocole";
import { ANGLES_REPOS, DOIGTS, NOMS, type Angles, type Articulation } from "./bras/reglages";
import { useLienUsb } from "./bras/useLienUsb";
import { useCamera } from "./useCamera";

/** Main absente plus longtemps que ça → le bras se fige là où il est. */
const DELAI_MAIN_PERDUE_MS = 300;

type Mode = "suivi" | "pause" | "repos";

export default function App() {
  const lien = useLienUsb();
  const derniereMain = useRef<{ angles: Angles; t: number } | null>(null);
  const filtre = useRef(new FiltreAngles());
  const camera = useCamera((main, monde) => {
    const t = performance.now();
    if (!main || !monde) return;
    // La main revient après une absence : on repart de zéro (sinon le filtre « glisserait » depuis l'ancienne position).
    if (!derniereMain.current || t - derniereMain.current.t > DELAI_MAIN_PERDUE_MS) filtre.current.reinitialiser();
    derniereMain.current = { angles: filtre.current.filtrer(mainVersAngles(main, monde), t / 1000), t };
  });

  const [mode, setMode] = useState<Mode>("suivi");
  const modeRef = useRef<Mode>("suivi");
  modeRef.current = mode;
  const [affichage, setAffichage] = useState<{ angles: Angles; mainVue: boolean; ips: number }>({ angles: ANGLES_REPOS, mainVue: false, ips: 0 });
  const imagesParSeconde = camera.imagesParSeconde;
  /** Les angles actuels, lus ~60 fois par seconde par la scène 3D (sans redessiner React). */
  const anglesAffiches = useRef<Angles>(ANGLES_REPOS);
  // Couleur du fond de la page (claire ou sombre) : la scène 3D s'y fond.
  const [fond] = useState(() => getComputedStyle(document.documentElement).getPropertyValue("--fond").trim() || "#f6efe4");
  const envoyer = useRef(lien.envoyer);
  envoyer.current = lien.envoyer;

  useEffect(() => {
    let actuels: Angles = ANGLES_REPOS;
    let dernierEnvoi: Angles | null = null;
    let tEnvoi = 0, tAffichage = 0, avant = performance.now(), image = 0;

    const boucle = (maintenant: number) => {
      const dt = Math.min(0.05, (maintenant - avant) / 1000);
      avant = maintenant;
      const m = derniereMain.current;
      const mainVue = !!m && maintenant - m.t < DELAI_MAIN_PERDUE_MS;

      // La cible : la main en mode suivi ; le repos ; sinon on reste où l'on est.
      const cible =
        modeRef.current === "repos" ? ANGLES_REPOS
        : modeRef.current === "suivi" && mainVue ? m!.angles
        : actuels;
      actuels = limiterVitesse(actuels, cible, dt);

      anglesAffiches.current = actuels;
      if (maintenant - tEnvoi > 20 && aChange(dernierEnvoi, actuels)) {
        tEnvoi = maintenant;
        dernierEnvoi = actuels;
        void envoyer.current(ligneAngles(actuels));
      }
      if (maintenant - tAffichage > 50) {
        tAffichage = maintenant;
        setAffichage({ angles: actuels, mainVue, ips: imagesParSeconde.current });
      }
      image = requestAnimationFrame(boucle);
    };
    image = requestAnimationFrame(boucle);
    return () => cancelAnimationFrame(image);
  }, [imagesParSeconde]);

  // Espace : pause / reprise (geste de sécurité au clavier).
  useEffect(() => {
    const touche = (e: KeyboardEvent) => {
      if (e.code !== "Space" || (e.target as HTMLElement).tagName === "BUTTON") return;
      e.preventDefault();
      setMode((m) => (m === "suivi" ? "pause" : "suivi"));
    };
    window.addEventListener("keydown", touche);
    return () => window.removeEventListener("keydown", touche);
  }, []);

  const phrase =
    mode === "pause" ? "En pause : le bras reste immobile. Espace pour reprendre."
    : mode === "repos" ? "Retour en position de repos."
    : camera.statut !== "active" ? "Active la caméra pour piloter le bras avec ta main."
    : affichage.mainVue
      ? affichage.ips && affichage.ips < 15
        ? `Le bras suit ta main, mais le suivi est lent (${affichage.ips} images/s) : ferme les autres programmes, ajoute de la lumière.`
        : `Le bras suit ta main (${affichage.ips} images/s).`
    : "Main hors de l'image : le bras s'est figé là où il était.";

  return (
    <main className="page">
      <header className="entete">
        <p className="bonjour">Projet d'ingénierie</p>
        <h1>Le bras et la main robotiques qui suivent ta main</h1>
        <p className={`direct ${mode !== "suivi" || !affichage.mainVue ? "attente" : ""}`} role="status">{phrase}</p>
      </header>

      <div className="grille">
        <section className="carte camera" aria-label="Caméra">
          <div className="camera__cadre">
            <video ref={camera.video} playsInline muted />
            <canvas ref={camera.dessin} />
            {camera.statut !== "active" && (
              <div className="camera__voile">
                <button type="button" className="bouton" onClick={camera.demarrer} disabled={camera.statut === "chargement"}>
                  {camera.statut === "chargement" ? "Chargement…" : "Activer la caméra"}
                </button>
                {camera.erreur && <p className="erreur">{camera.erreur}</p>}
                <p className="note">L'image reste sur ton ordinateur : rien n'est envoyé sur internet.</p>
              </div>
            )}
          </div>
          <ul className="aide">
            <li><strong>Gauche / droite</strong> : la base tourne</li>
            <li><strong>Haut / bas</strong> : l'épaule monte ou descend</li>
            <li><strong>Main près de la caméra</strong> : le bras se tend</li>
            <li><strong>Plier un doigt</strong> : la main robot plie le même</li>
            <li><strong>Tourner la main</strong> (comme une poignée) : le poignet tourne</li>
          </ul>
        </section>

        <section className="carte" aria-label="Le bras">
          <div className="bras" role="img" aria-label="Le bras et la main robotiques en 3D, qui copient ta main">
            <Bras3D angles={anglesAffiches} fond={fond} />
          </div>
          <p className="aide-3d">Fais glisser pour tourner autour du robot, molette pour zoomer.</p>
          <h2 className="sous-titre">Le bras</h2>
          <dl className="angles">
            {(["base", "epaule", "coude", "poignet"] as Articulation[]).map((a) => (
              <div key={a}>
                <dt>{NOMS[a]}</dt>
                <dd>{Math.round(affichage.angles[a])}°</dd>
              </div>
            ))}
          </dl>
          <h2 className="sous-titre">Les doigts</h2>
          <dl className="angles angles--doigts">
            {DOIGTS.map((a) => (
              <div key={a}>
                <dt>{NOMS[a]}</dt>
                <dd>{Math.round(affichage.angles[a])}°</dd>
              </div>
            ))}
          </dl>
          <div className="boutons">
            <button type="button" className="bouton bouton--doux" onClick={() => setMode(mode === "suivi" ? "pause" : "suivi")}>
              {mode === "suivi" ? "Pause" : "Reprendre"} <kbd>Espace</kbd>
            </button>
            <button type="button" className="bouton bouton--doux" onClick={() => setMode("repos")}>Position de repos</button>
          </div>
        </section>

        <section className="carte usb" aria-label="Le vrai bras">
          <h2>Le vrai bras</h2>
          {lien.statut === "indisponible" ? (
            <p>Pour piloter le vrai bras par USB, ouvre cette page dans <strong>Chrome</strong> ou <strong>Edge</strong> sur ordinateur.</p>
          ) : lien.statut === "connecte" ? (
            <>
              <p className="ok">Bras connecté : il reçoit les angles en direct.</p>
              {lien.message && <p className="note">L'ESP32 répond : « {lien.message} »</p>}
              <button type="button" className="bouton bouton--doux" onClick={lien.deconnecter}>Déconnecter</button>
            </>
          ) : (
            <>
              <p>Sans bras branché, le dessin te permet déjà de tout tester.</p>
              {lien.message && <p className="erreur">{lien.message}</p>}
              <button type="button" className="bouton" onClick={lien.connecter}>Connecter le bras (USB)</button>
            </>
          )}
        </section>
      </div>
    </main>
  );
}
