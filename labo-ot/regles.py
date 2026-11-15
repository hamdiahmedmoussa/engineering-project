"""
LES RÈGLES DU PARE-FEU MODBUS (logique pure : testée dans tests/)

Un pare-feu réseau classique ne regarde que les adresses et les ports. Celui-ci LIT chaque message
Modbus (on parle d'« inspection approfondie ») et vérifie qu'il a du sens pour CE poste électrique.
Plusieurs règles indépendantes : c'est la « défense en profondeur ». Si un attaquant en contourne une
(par exemple en usurpant l'adresse de la supervision), les suivantes l'arrêtent encore.
"""
from __future__ import annotations

from collections import deque
from dataclasses import dataclass

from modbus import ECRIRE_BOBINE, ECRIRE_BOBINES, ECRIRE_REGISTRE, ECRIRE_REGISTRES, LIRE_BOBINES, LIRE_ENTREES, LIRE_REGISTRES, Requete

REGLES = {
    # 1. Seule la supervision a le droit d'écrire. Tout le monde peut lire.
    "ecrivains": {"127.0.0.2"},
    # 2. Seules les fonctions utiles à ce poste (les fonctions de diagnostic, de mise à jour… sont bloquées).
    "fonctions": {LIRE_BOBINES, LIRE_REGISTRES, LIRE_ENTREES, ECRIRE_BOBINE, ECRIRE_REGISTRE, ECRIRE_BOBINES, ECRIRE_REGISTRES},
    # 3. Ce qui peut être écrit : les 3 disjoncteurs, et les 3 seuils dans une plage RAISONNABLE.
    "bobines": range(0, 3),
    "seuils": {0: (1500, 3000), 1: (2200, 3800), 2: (2800, 4500)},  # dixièmes d'ampère, par départ
    # 4. Jamais plus d'un disjoncteur ouvert en une seule commande (une coupure générale = un geste humain par départ).
    "ouvertures_max_par_commande": 1,
    # 5. Pas plus de 6 écritures par minute : un humain clique, un script mitraille.
    "ecritures_max_par_minute": 6,
}


@dataclass
class Decision:
    autorise: bool
    raison: str


class Gardien:
    """Applique les règles, et se souvient des dernières écritures (pour la règle 5)."""

    def __init__(self, regles: dict = REGLES) -> None:
        self.regles = regles
        self.ecritures: deque[float] = deque()

    def examiner(self, req: Requete, ip: str, maintenant: float) -> Decision:
        r = self.regles
        if req.fonction not in r["fonctions"]:
            return Decision(False, f"fonction {req.fonction} interdite sur ce poste")
        if not req.est_ecriture:
            return Decision(True, "lecture")

        if ip not in r["ecrivains"]:
            return Decision(False, f"écriture depuis {ip}, qui n'est pas la supervision")

        if req.fonction in (ECRIRE_BOBINE, ECRIRE_BOBINES):
            if any(a not in r["bobines"] for a in range(req.adresse, req.adresse + req.quantite)):
                return Decision(False, "bobine hors de la liste autorisée")
            if req.valeurs.count(0) > r["ouvertures_max_par_commande"]:
                return Decision(False, f"{req.valeurs.count(0)} disjoncteurs ouverts d'un coup")
        else:  # registres : les seuils de protection
            for i, v in enumerate(req.valeurs):
                adresse = req.adresse + i
                if adresse not in r["seuils"]:
                    return Decision(False, f"registre {adresse} non modifiable")
                mini, maxi = r["seuils"][adresse]
                if not mini <= v <= maxi:
                    return Decision(False, f"seuil {v / 10:.1f} A hors de la plage {mini / 10:.0f}-{maxi / 10:.0f} A")

        while self.ecritures and maintenant - self.ecritures[0] > 60:
            self.ecritures.popleft()
        if len(self.ecritures) >= r["ecritures_max_par_minute"]:
            return Decision(False, "trop d'écritures en une minute")
        self.ecritures.append(maintenant)
        return Decision(True, "écriture conforme")
