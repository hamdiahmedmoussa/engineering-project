"""
LA SUPERVISION (la « salle de contrôle »), sous forme de page web

Elle lit l'automate chaque seconde À TRAVERS LE PARE-FEU, depuis l'adresse 127.0.0.2
(l'adresse « autorisée à écrire » dans regles.py), et permet d'ouvrir / fermer les disjoncteurs
et de régler les seuils. Elle affiche aussi le journal du pare-feu.

    python supervision.py              → http://127.0.0.1:8080 (via le pare-feu, port 5020)
    python supervision.py --direct     → sans pare-feu (port 5021), pour comparer
"""
from __future__ import annotations

import argparse
import json
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

from client import ClientModbus
from modbus import ErreurModbus
from pare_feu import JOURNAL

DEPARTS = ["Hôpital", "Quartier résidentiel", "Zone portuaire"]
PAGE = Path(__file__).with_name("supervision.html").read_bytes()
ADRESSE_SUPERVISION = "127.0.0.2"
SEUILS_NORMAUX = [2000, 2800, 3500]  # dixièmes d'ampère : 200 A, 280 A, 350 A (réglages d'origine du poste)


class Liaison:
    """Une seule connexion Modbus, partagée par toutes les requêtes de la page (protégée par un verrou)."""

    def __init__(self, port: int) -> None:
        self.port = port
        self.client: ClientModbus | None = None
        self.verrou = threading.Lock()

    def executer(self, action):
        with self.verrou:
            try:
                if self.client is None:
                    self.client = ClientModbus("127.0.0.1", self.port, ADRESSE_SUPERVISION)
                return action(self.client)
            except (OSError, ConnectionError):
                self.client = None  # on se reconnectera à la prochaine demande
                raise


def creer_gestionnaire(liaison: Liaison):
    class Gestionnaire(BaseHTTPRequestHandler):
        def log_message(self, *args) -> None:  # pas de ligne dans la console à chaque requête
            pass

        def repondre(self, code: int, corps: bytes, type_: str = "application/json") -> None:
            self.send_response(code)
            self.send_header("Content-Type", type_)
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(corps)

        def json(self, objet, code: int = 200) -> None:
            self.repondre(code, json.dumps(objet, ensure_ascii=False).encode())

        def do_GET(self) -> None:
            if self.path == "/":
                return self.repondre(200, PAGE, "text/html; charset=utf-8")
            if self.path == "/etat":
                try:
                    b, s, m = liaison.executer(lambda c: (c.lire_bobines(0, 3), c.lire_registres(0, 3), c.lire_mesures(0, 5)))
                except (OSError, ConnectionError, ErreurModbus):
                    return self.json({"erreur": "Poste injoignable : lance automate.py (et pare_feu.py)."})
                return self.json({
                    "departs": [{"nom": DEPARTS[i], "ferme": bool(b[i]), "courant": m[i] / 10, "seuil": s[i] / 10} for i in range(3)],
                    "tension": m[3] / 10, "frequence": m[4] / 100, "viaPareFeu": liaison.port == 5020,
                })
            if self.path == "/journal":
                lignes = JOURNAL.read_text(encoding="utf-8").splitlines()[-12:] if JOURNAL.exists() else []
                return self.json([json.loads(l) for l in reversed(lignes)])
            self.repondre(404, b"{}")

        def do_POST(self) -> None:
            # Même protection que les boîtiers ESP32 : seule notre page envoie cet en-tête (contre le CSRF).
            if self.path != "/commande" or self.headers.get("X-Tableau") != "1":
                return self.repondre(403, b"{}")
            try:
                d = json.loads(self.rfile.read(int(self.headers.get("Content-Length", 0))))
                if d.get("action") == "retablir":  # après un sabotage : les 3 seuils d'origine, en une commande
                    liaison.executer(lambda c: c.ecrire_registres(0, SEUILS_NORMAUX))
                    return self.json({"ok": True})
                i = int(d["depart"])
                if not 0 <= i < 3:
                    raise ValueError
                if d["action"] in ("ouvrir", "fermer"):
                    liaison.executer(lambda c: c.ecrire_bobine(i, 1 if d["action"] == "fermer" else 0))
                elif d["action"] == "seuil":
                    liaison.executer(lambda c: c.ecrire_registre(i, int(round(float(d["valeur"]) * 10))))
                else:
                    raise ValueError
                self.json({"ok": True})
            except ErreurModbus:
                self.json({"ok": False, "message": "Commande refusée par le pare-feu (voir le journal)."})
            except (OSError, ConnectionError):
                self.json({"ok": False, "message": "Poste injoignable."})
            except (ValueError, KeyError, json.JSONDecodeError):
                self.repondre(400, b"{}")

    return Gestionnaire


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--direct", action="store_true", help="parler à l'automate sans passer par le pare-feu")
    a = p.parse_args()
    liaison = Liaison(5021 if a.direct else 5020)
    serveur = ThreadingHTTPServer(("127.0.0.1", 8080), creer_gestionnaire(liaison))
    print(f"Supervision : http://127.0.0.1:8080  ({'SANS pare-feu' if a.direct else 'via le pare-feu'}). Ctrl+C pour arrêter.")
    try:
        serveur.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
