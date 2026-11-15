/*
  L'ÉCRAN PRINCIPAL
  Boucle d'animation (environ 60 fois par seconde) :
    1. la cible : les angles calculés depuis la main, ou la position figée (pause, main perdue, repos) ;
    2. les angles actuels avancent doucement vers la cible (lisser) ;
    3. on dessine le bras, et on envoie les angles à l'ESP32 environ 30 fois par seconde.
*/
import { useEffect, useRef, useState } from "react";
import { lisser, mainVersAngles, type Point } from "./bras/angles";
import { dessinerBras } from "./bras/dessinerBras";
import { aChange, ligneAngles } from "./bras/protocole";
import { ANGLES_REPOS, ARTICULATIONS, NOMS, type Angles } from "./bras/reglages";
import { useLienUsb } from "./bras/useLienUsb";
import { useCamera } from "./useCamera";

/** Main absente plus longtemps que ça → le bras se fige là où il est. */
const DELAI_MAIN_PERDUE_MS = 300;

type Mode = "suivi" | "pause" | "repos";

export default function App() {
  const lien = useLienUsb();
  const derniereMain = useRef<{ main: Point[]; t: number } | null>(null);
  const camera = useCamera((main) => {
    if (main) derniereMain.current = { main, t: performance.now() };
  });

  const [mode, setMode] = useState<Mode>("suivi");
  const modeRef = useRef<Mode>("suivi");
  modeRef.current = mode;
  const [affichage, setAffichage] = useState<{ angles: Angles; mainVue: boolean }>({ angles: ANGLES_REPOS, mainVue: false });
  const toile = useRef<HTMLCanvasElement>(null);
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
        : modeRef.current === "suivi" && mainVue ? mainVersAngles(m!.main)
        : actuels;
      actuels = lisser(actuels, cible, dt);

      if (toile.current) dessinerBras(toile.current, actuels);
      if (maintenant - tEnvoi > 33 && aChange(dernierEnvoi, actuels)) {
        tEnvoi = maintenant;
        dernierEnvoi = actuels;
        void envoyer.current(ligneAngles(actuels));
      }
      if (maintenant - tAffichage > 100) {
        tAffichage = maintenant;
        setAffichage({ angles: actuels, mainVue });
      }
      image = requestAnimationFrame(boucle);
    };
    image = requestAnimationFrame(boucle);
    return () => cancelAnimationFrame(image);
  }, []);

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
    : affichage.mainVue ? "Le bras suit ta main."
    : "Main hors de l'image : le bras s'est figé là où il était.";

  return (
    <main className="page">
      <header className="entete">
        <p className="bonjour">Projet d'ingénierie</p>
        <h1>Le bras robot qui suit ta main</h1>
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
            <li><strong>Pincer pouce et index</strong> : la pince se ferme</li>
          </ul>
        </section>

        <section className="carte" aria-label="Le bras">
          <canvas ref={toile} className="bras" width="460" height="300" role="img" aria-label="Dessin du bras robot" />
          <dl className="angles">
            {ARTICULATIONS.map((a) => (
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
