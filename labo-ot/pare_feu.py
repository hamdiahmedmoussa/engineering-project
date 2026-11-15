"""
LE PARE-FEU MODBUS : il se place ENTRE la supervision et l'automate.

    supervision ──► pare-feu (port 5020) ──► automate (port 5021)

Chaque message est lu et jugé par regles.py :
  - autorisé : transmis à l'automate, et la réponse revient ;
  - refusé : l'automate ne le voit JAMAIS ; le pare-feu répond « fonction illégale » et lance une alerte.
Tout est écrit dans journal-pare-feu.jsonl (une ligne JSON par événement), lu par la supervision.

Dans un vrai poste, l'automate n'est joignable QUE par le pare-feu (câble ou réseau séparé).
Ici, tout est sur le même PC : la démonstration montre l'attaque directe ET l'attaque via le pare-feu.

Lancer :  python pare_feu.py                               (automate simulé, sur ce PC)
          python pare_feu.py --automate 192.168.4.1:502      (automate ESP32, réseau « Labo-OT »)
"""
from __future__ import annotations

import argparse
import asyncio
import json
import struct
import time
from pathlib import Path

from modbus import FONCTION_ILLEGALE, ErreurModbus, decoder_requete, extraire_trame, reponse_exception
from regles import Gardien

PORT = 5020
AUTOMATE = ("127.0.0.1", 5021)
JOURNAL = Path(__file__).with_name("journal-pare-feu.jsonl")


def journaliser(fichier: Path, ip: str, quoi: str, autorise: bool, raison: str) -> None:
    ligne = {"heure": time.strftime("%H:%M:%S"), "ip": ip, "requete": quoi, "autorise": autorise, "raison": raison}
    with fichier.open("a", encoding="utf-8") as f:
        f.write(json.dumps(ligne, ensure_ascii=False) + "\n")
    if not autorise:
        print(f"[pare-feu] ALERTE {ligne['heure']}  {ip}  {quoi}  →  BLOQUÉ : {raison}")


async def servir(gardien: Gardien, port: int = PORT, automate: tuple[str, int] = AUTOMATE,
                 journal: Path = JOURNAL) -> asyncio.Server:
    async def client(lecteur: asyncio.StreamReader, ecrivain: asyncio.StreamWriter) -> None:
        ip = ecrivain.get_extra_info("peername")[0]
        try:
            vers_lecteur, vers_ecrivain = await asyncio.open_connection(*automate)
        except OSError:
            print("[pare-feu] Automate injoignable : lance d'abord « python automate.py ».")
            ecrivain.close()
            return
        tampon = b""
        try:
            while donnees := await lecteur.read(260):
                tampon += donnees
                while True:
                    trame, tampon = extraire_trame(tampon)
                    if trame is None:
                        break
                    try:
                        req = decoder_requete(trame)
                    except ErreurModbus as e:
                        # Trame mal formée ou fonction inconnue : on ne la transmet pas.
                        t, u, f = struct.unpack(">HxxxxBB", trame[:8])
                        journaliser(journal, ip, f"trame invalide (fonction {f})", False, str(e))
                        ecrivain.write(reponse_exception(t, u, f, FONCTION_ILLEGALE))
                        continue
                    decision = gardien.examiner(req, ip, time.monotonic())
                    if req.est_ecriture or not decision.autorise:  # on ne journalise pas chaque lecture
                        journaliser(journal, ip, str(req), decision.autorise, decision.raison)
                    if not decision.autorise:
                        ecrivain.write(reponse_exception(req.transaction, req.unite, req.fonction, FONCTION_ILLEGALE))
                        continue
                    vers_ecrivain.write(trame)
                    await vers_ecrivain.drain()
                    # On lit la réponse de l'automate jusqu'à avoir une trame complète, puis on la renvoie.
                    reponse, trame_reponse = b"", None
                    while trame_reponse is None:
                        morceau = await vers_lecteur.read(260)
                        if not morceau:
                            raise ConnectionError
                        reponse += morceau
                        trame_reponse, _ = extraire_trame(reponse)
                    ecrivain.write(trame_reponse)
                await ecrivain.drain()
        except (ErreurModbus, ConnectionError):
            pass
        finally:
            ecrivain.close()
            vers_ecrivain.close()

    return await asyncio.start_server(client, "127.0.0.1", port)


async def principal(automate: tuple[str, int]) -> None:
    serveur = await servir(Gardien(), automate=automate)
    print(f"[pare-feu] En garde sur le port {PORT}, devant l'automate {automate[0]}:{automate[1]}. Journal : {JOURNAL.name}")
    async with serveur:
        await serveur.serve_forever()


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--automate", default=f"{AUTOMATE[0]}:{AUTOMATE[1]}", help="adresse:port de l'automate à protéger")
    hote, _, port = p.parse_args().automate.rpartition(":")
    try:
        asyncio.run(principal((hote, int(port))))
    except KeyboardInterrupt:
        pass
