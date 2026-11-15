/*
  LA CAMÉRA ET LA DÉTECTION DE LA MAIN (même principe que le drone)
  À chaque image, MediaPipe donne les 21 points de la main (dans l'image ET en 3D), ou rien ;
  on les dessine, puis on les transmet à « surMain ».
  Tout se passe dans le navigateur : l'image de la caméra n'est envoyée nulle part.
*/
import { DrawingUtils, FilesetResolver, HandLandmarker } from "@mediapipe/tasks-vision";
import { useCallback, useEffect, useRef, useState } from "react";
import type { Point } from "./bras/angles";

export type StatutCamera = "arret" | "chargement" | "active" | "erreur";

async function creerDetecteur(): Promise<HandLandmarker> {
  const moteur = await FilesetResolver.forVisionTasks("/mediapipe");
  const options = (delegate: "GPU" | "CPU") => ({
    baseOptions: { modelAssetPath: "/modeles/hand_landmarker.task", delegate },
    runningMode: "VIDEO" as const,
    numHands: 1,
  });
  try {
    return await HandLandmarker.createFromOptions(moteur, options("GPU"));
  } catch {
    return await HandLandmarker.createFromOptions(moteur, options("CPU"));
  }
}

/** surMain reçoit les points dans l'image (position) et en 3D (forme de la main), ou null sans main. */
export function useCamera(surMain: (main: Point[] | null, monde: Point[] | null) => void) {
  const video = useRef<HTMLVideoElement>(null);
  const dessin = useRef<HTMLCanvasElement>(null);
  const [statut, setStatut] = useState<StatutCamera>("arret");
  const [erreur, setErreur] = useState("");
  const rappel = useRef(surMain);
  rappel.current = surMain;
  const arret = useRef<() => void>(() => {});

  const demarrer = useCallback(async () => {
    setStatut("chargement");
    setErreur("");
    try {
      const flux = await navigator.mediaDevices.getUserMedia({
        video: { facingMode: "user", width: 640, height: 480 },
        audio: false,
      });
      const v = video.current!;
      v.srcObject = flux;
      await v.play();
      const detecteur = await creerDetecteur();
      const canvas = dessin.current!;
      canvas.width = v.videoWidth;
      canvas.height = v.videoHeight;
      const ctx = canvas.getContext("2d")!;
      const outils = new DrawingUtils(ctx);
      let derniereImage = -1;
      let image = 0;

      const boucle = () => {
        if (v.currentTime !== derniereImage) {
          derniereImage = v.currentTime;
          const resultat = detecteur.detectForVideo(v, performance.now());
          const main = resultat.landmarks[0] ?? null;
          ctx.clearRect(0, 0, canvas.width, canvas.height);
          if (main) {
            outils.drawConnectors(main, HandLandmarker.HAND_CONNECTIONS, { color: "#e6a23c", lineWidth: 3 });
            outils.drawLandmarks(main, { color: "#fffaf3", radius: 3 });
          }
          rappel.current(main, resultat.worldLandmarks[0] ?? null);
        }
        image = requestAnimationFrame(boucle);
      };
      image = requestAnimationFrame(boucle);
      setStatut("active");
      arret.current = () => {
        cancelAnimationFrame(image);
        detecteur.close();
        flux.getTracks().forEach((t) => t.stop());
      };
    } catch (e) {
      setStatut("erreur");
      const nom = e instanceof DOMException ? e.name : "";
      setErreur(
        nom === "NotAllowedError"
          ? "Accès à la caméra refusé. Autorise la caméra dans la barre d'adresse, puis réessaie."
          : nom === "NotFoundError"
            ? "Aucune caméra trouvée sur cet appareil."
            : "Impossible de démarrer la détection de la main. Recharge la page et réessaie.",
      );
    }
  }, []);

  useEffect(() => () => arret.current(), []);
  return { video, dessin, statut, erreur, demarrer };
}
