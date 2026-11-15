"""
LA PAGE DU DÉTECTEUR : colle un message, il te dit si c'est probablement une arnaque, et pourquoi.

    python serveur.py   → http://127.0.0.1:8090

Le message reste sur ton ordinateur : rien n'est envoyé sur internet, rien n'est enregistré.
"""
from __future__ import annotations

import json
from dataclasses import asdict
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

from detecteur import Classifieur, analyser, charger

PAGE = Path(__file__).with_name("page.html").read_bytes()
MODELE = Classifieur().apprendre(charger())


class Gestionnaire(BaseHTTPRequestHandler):
    def log_message(self, *args) -> None:  # on n'écrit pas les messages analysés dans la console
        pass

    def repondre(self, code: int, corps: bytes, type_: str = "application/json") -> None:
        self.send_response(code)
        self.send_header("Content-Type", type_)
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(corps)

    def do_GET(self) -> None:
        if self.path == "/":
            self.repondre(200, PAGE, "text/html; charset=utf-8")
        else:
            self.repondre(404, b"{}")

    def do_POST(self) -> None:
        # Seule notre page envoie cet en-tête (contre les requêtes envoyées par un autre site).
        if self.path != "/analyser" or self.headers.get("X-Tableau") != "1":
            return self.repondre(403, b"{}")
        try:
            taille = int(self.headers.get("Content-Length", 0))
            if taille > 20_000:
                raise ValueError("message trop long")
            texte = json.loads(self.rfile.read(taille))["texte"]
            if not isinstance(texte, str):
                raise ValueError("texte attendu")
            verdict = analyser(texte, MODELE)
        except (ValueError, KeyError, json.JSONDecodeError) as e:
            return self.repondre(400, json.dumps({"erreur": str(e)}, ensure_ascii=False).encode())
        self.repondre(200, json.dumps(asdict(verdict), ensure_ascii=False).encode())


def main() -> None:
    serveur = ThreadingHTTPServer(("127.0.0.1", 8090), Gestionnaire)
    print("Détecteur d'arnaques : http://127.0.0.1:8090  (Ctrl+C pour arrêter)")
    try:
        serveur.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
