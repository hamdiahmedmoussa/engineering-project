/*
  LE LIEN USB AVEC L'ESP32 (Web Serial)
  Chrome et Edge sur ordinateur savent parler à une carte branchée en USB, directement depuis la page.
  On envoie des lignes de texte (voir protocole.ts) et on affiche ce que l'ESP32 répond.
*/
import { useCallback, useEffect, useRef, useState } from "react";

export type StatutLien = "indisponible" | "deconnecte" | "connecte";

export function useLienUsb() {
  const [statut, setStatut] = useState<StatutLien>("serial" in navigator ? "deconnecte" : "indisponible");
  const [message, setMessage] = useState("");
  const port = useRef<SerialPort | null>(null);
  const ecrivain = useRef<WritableStreamDefaultWriter<Uint8Array> | null>(null);
  const occupe = useRef(false);
  const encodeur = useRef(new TextEncoder());

  const deconnecter = useCallback(async () => {
    try {
      ecrivain.current?.releaseLock();
      await port.current?.close();
    } catch {
      // déjà fermé (câble débranché) : rien à faire
    }
    ecrivain.current = null;
    port.current = null;
    setStatut("serial" in navigator ? "deconnecte" : "indisponible");
  }, []);

  const connecter = useCallback(async () => {
    try {
      const p = await navigator.serial.requestPort(); // Chrome affiche la liste des cartes branchées
      await p.open({ baudRate: 115200 });
      port.current = p;
      ecrivain.current = p.writable!.getWriter();
      setStatut("connecte");
      setMessage("");

      // Lecture des réponses de l'ESP32, ligne par ligne (en arrière-plan).
      (async () => {
        const lecteur = p.readable!.pipeThrough(new TextDecoderStream() as unknown as ReadableWritablePair<string, Uint8Array>).getReader();
        let reste = "";
        try {
          for (;;) {
            const { value, done } = await lecteur.read();
            if (done) break;
            reste += value;
            const lignes = reste.split("\n");
            reste = lignes.pop() ?? "";
            const derniere = lignes.map((l) => l.trim()).filter(Boolean).pop();
            if (derniere) setMessage(derniere);
          }
        } catch {
          // câble débranché
        }
        await deconnecter();
      })();
    } catch (e) {
      // L'utilisateur a fermé la liste sans choisir : ce n'est pas une erreur.
      if (!(e instanceof DOMException && e.name === "NotFoundError")) setMessage("Impossible d'ouvrir la carte. Ferme le Moniteur série de l'Arduino IDE, puis réessaie.");
    }
  }, [deconnecter]);

  /** Envoie une ligne. Si l'envoi précédent n'est pas fini, on saute celle-ci (la suivante arrive dans 33 ms). */
  const envoyer = useCallback(async (ligne: string) => {
    if (!ecrivain.current || occupe.current) return;
    occupe.current = true;
    try {
      await ecrivain.current.write(encodeur.current.encode(ligne));
    } catch {
      await deconnecter();
    } finally {
      occupe.current = false;
    }
  }, [deconnecter]);

  useEffect(() => () => void deconnecter(), [deconnecter]);
  return { statut, message, connecter, deconnecter, envoyer };
}
