"""
L'AUTOMATE D'UN PETIT POSTE ÉLECTRIQUE (simulé), qui parle Modbus TCP

Il alimente 3 départs. Sa mémoire Modbus :
    bobines 0-2          disjoncteurs des départs (1 = fermé, le départ est alimenté)
    registres 0-2        seuils de protection, en dixièmes d'ampère (2500 = 250,0 A)
    mesures 0-2          courant de chaque départ (dixièmes d'ampère)
    mesures 3 et 4       tension (dixièmes de volt) et fréquence (centièmes de Hz)

La PROTECTION : si le courant d'un départ dépasse son seuil, l'automate ouvre son disjoncteur
(comme un vrai poste, pour éviter qu'un câble surchargé ne prenne feu).

Comme un vrai automate Modbus, il obéit à QUICONQUE lui parle : c'est la faille du laboratoire.

Lancer :  python automate.py            (écoute sur le port 5021)
"""
from __future__ import annotations

import asyncio
import math
import random
import struct
import time

from modbus import (ADRESSE_ILLEGALE, ECRIRE_BOBINE, ECRIRE_BOBINES, ECRIRE_REGISTRE, ECRIRE_REGISTRES,
                    LIRE_BOBINES, LIRE_ENTREES, LIRE_REGISTRES, ErreurModbus, Requete, decoder_requete,
                    encoder_reponse, extraire_trame, reponse_exception)

DEPARTS = ["Hôpital", "Quartier résidentiel", "Zone portuaire"]
COURANT_NORMAL = [1200, 1800, 2400]  # dixièmes d'ampère : 120 A, 180 A, 240 A en moyenne
PORT = 5021


class Memoire:
    """La mémoire Modbus de l'automate, et ce qu'il fait quand on la lit ou l'écrit."""

    def __init__(self) -> None:
        self.bobines = [1, 1, 1]
        self.registres = [2000, 2800, 3500]  # seuils : 200 A, 280 A, 350 A
        self.mesures = [0, 0, 0, 2300, 5000]  # 3 courants, tension 230,0 V, fréquence 50,00 Hz
        self.evenements: list[str] = []

    def traiter(self, req: Requete) -> bytes:
        """Exécute une requête et renvoie la réponse (ou une exception si l'adresse n'existe pas)."""
        try:
            return encoder_reponse(req, self._executer(req))
        except ErreurModbus as e:
            return reponse_exception(req.transaction, req.unite, req.fonction, e.code)

    def _zone(self, liste: list[int], req: Requete) -> slice:
        if req.adresse + req.quantite > len(liste):
            raise ErreurModbus("adresse inexistante", ADRESSE_ILLEGALE)
        return slice(req.adresse, req.adresse + req.quantite)

    def _executer(self, req: Requete) -> bytes:
        f = req.fonction
        if f == LIRE_BOBINES:
            bits = self.bobines[self._zone(self.bobines, req)]
            octets = bytearray((len(bits) + 7) // 8)
            for i, b in enumerate(bits):
                octets[i // 8] |= b << (i % 8)
            return bytes([len(octets)]) + bytes(octets)
        if f in (LIRE_REGISTRES, LIRE_ENTREES):
            liste = self.registres if f == LIRE_REGISTRES else self.mesures
            valeurs = liste[self._zone(liste, req)]
            return bytes([2 * len(valeurs)]) + struct.pack(f">{len(valeurs)}H", *valeurs)
        if f in (ECRIRE_BOBINE, ECRIRE_BOBINES):
            self.bobines[self._zone(self.bobines, req)] = req.valeurs
            for i, v in enumerate(req.valeurs):
                self.noter(f"Disjoncteur « {DEPARTS[req.adresse + i]} » {'fermé' if v else 'OUVERT'} par Modbus")
        elif f in (ECRIRE_REGISTRE, ECRIRE_REGISTRES):
            self.registres[self._zone(self.registres, req)] = req.valeurs
            for i, v in enumerate(req.valeurs):
                self.noter(f"Seuil de protection « {DEPARTS[req.adresse + i]} » réglé à {v / 10:.1f} A")
        # Réponse d'une écriture : écho de la requête (norme Modbus)
        if f == ECRIRE_BOBINE:
            return struct.pack(">HH", req.adresse, 0xFF00 if req.valeurs[0] else 0)
        if f == ECRIRE_REGISTRE:
            return struct.pack(">HH", req.adresse, req.valeurs[0])
        return struct.pack(">HH", req.adresse, req.quantite)

    def simuler(self, t: float) -> None:
        """Fait vivre le réseau : courants qui varient, et protection qui déclenche si un seuil est dépassé."""
        for i, normal in enumerate(COURANT_NORMAL):
            if not self.bobines[i]:
                self.mesures[i] = 0
                continue
            courant = normal * (0.85 + 0.15 * math.sin(t / 20 + i)) + random.uniform(-30, 30)
            self.mesures[i] = max(0, int(courant))
            if self.mesures[i] > self.registres[i]:
                self.bobines[i] = 0
                self.noter(f"PROTECTION : « {DEPARTS[i]} » coupé ({self.mesures[i] / 10:.0f} A > seuil {self.registres[i] / 10:.0f} A)")
        self.mesures[3] = 2300 + random.randint(-20, 20)
        self.mesures[4] = 5000 + random.randint(-3, 3)

    def noter(self, texte: str) -> None:
        ligne = f"{time.strftime('%H:%M:%S')}  {texte}"
        self.evenements.append(ligne)
        del self.evenements[:-20]
        print("[automate]", ligne)


async def servir(memoire: Memoire, hote: str = "127.0.0.1", port: int = PORT) -> asyncio.Server:
    async def client(lecteur: asyncio.StreamReader, ecrivain: asyncio.StreamWriter) -> None:
        tampon = b""
        try:
            while donnees := await lecteur.read(260):
                tampon += donnees
                while True:
                    trame, tampon = extraire_trame(tampon)
                    if trame is None:
                        break
                    try:
                        ecrivain.write(memoire.traiter(decoder_requete(trame)))
                    except ErreurModbus as e:
                        t, u, f = struct.unpack(">HxxxxBB", trame[:8])
                        ecrivain.write(reponse_exception(t, u, f, e.code))
                await ecrivain.drain()
        except (ErreurModbus, ConnectionError):
            pass  # trame illisible ou client parti : on ferme la connexion
        finally:
            ecrivain.close()

    return await asyncio.start_server(client, hote, port)


async def principal() -> None:
    memoire = Memoire()
    serveur = await servir(memoire)
    print(f"[automate] Poste électrique en marche, Modbus sur le port {PORT}. Ctrl+C pour arrêter.")
    debut = time.monotonic()
    async with serveur:
        while True:
            memoire.simuler(time.monotonic() - debut)
            await asyncio.sleep(0.5)


if __name__ == "__main__":
    try:
        asyncio.run(principal())
    except KeyboardInterrupt:
        pass
