"""
LE PROTOCOLE MODBUS TCP, OCTET PAR OCTET

Modbus date de 1979. Il est partout : usines, postes électriques, stations de pompage.
Il n'a AUCUNE authentification : n'importe quel appareil qui peut parler à l'automate
peut lire ET écrire. C'est tout le sujet de ce laboratoire.

Une trame Modbus TCP :
    en-tête MBAP (7 octets)                         | PDU (le message)
    transaction (2) | protocole = 0 (2) | longueur (2) | unité (1) | fonction (1) | données…
« longueur » = nombre d'octets qui suivent (unité + PDU).

Les fonctions utilisées ici :
    1  lire des bobines (bits : disjoncteur fermé = 1)
    3  lire des registres de maintien (réglages : seuils de protection)
    4  lire des registres d'entrée (mesures : courants, tension)
    5  écrire UNE bobine          6  écrire UN registre
    15 écrire PLUSIEURS bobines   16 écrire PLUSIEURS registres
"""
from __future__ import annotations

import struct
from dataclasses import dataclass, field

LIRE_BOBINES, LIRE_REGISTRES, LIRE_ENTREES = 1, 3, 4
ECRIRE_BOBINE, ECRIRE_REGISTRE, ECRIRE_BOBINES, ECRIRE_REGISTRES = 5, 6, 15, 16
ECRITURES = {ECRIRE_BOBINE, ECRIRE_REGISTRE, ECRIRE_BOBINES, ECRIRE_REGISTRES}
NOMS = {
    1: "lire bobines", 3: "lire registres", 4: "lire mesures",
    5: "écrire bobine", 6: "écrire registre", 15: "écrire bobines", 16: "écrire registres",
}

# Codes d'exception renvoyés par un automate (norme Modbus)
FONCTION_ILLEGALE, ADRESSE_ILLEGALE, VALEUR_ILLEGALE = 1, 2, 3


class ErreurModbus(Exception):
    """Trame invalide, ou exception renvoyée par l'automate. « code » = code d'exception Modbus."""

    def __init__(self, message: str, code: int = VALEUR_ILLEGALE):
        super().__init__(message)
        self.code = code


@dataclass
class Requete:
    transaction: int
    unite: int
    fonction: int
    adresse: int = 0
    quantite: int = 0
    valeurs: list[int] = field(default_factory=list)

    @property
    def est_ecriture(self) -> bool:
        return self.fonction in ECRITURES

    def __str__(self) -> str:
        texte = f"{NOMS.get(self.fonction, f'fonction {self.fonction}')} @{self.adresse} ×{self.quantite}"
        return texte + (f" = {self.valeurs}" if self.valeurs else "")


def extraire_trame(tampon: bytes) -> tuple[bytes | None, bytes]:
    """Découpe UNE trame complète au début du tampon (TCP peut couper ou coller les messages)."""
    if len(tampon) < 7:
        return None, tampon
    longueur = struct.unpack(">H", tampon[4:6])[0]
    if not 2 <= longueur <= 254:  # une trame Modbus TCP fait au plus 260 octets
        raise ErreurModbus("longueur de trame impossible")
    total = 6 + longueur
    if len(tampon) < total:
        return None, tampon
    return tampon[:total], tampon[total:]


def decoder_requete(trame: bytes) -> Requete:
    """Lit une requête et VÉRIFIE tout : on ne fait jamais confiance à ce qui arrive du réseau."""
    transaction, protocole, longueur, unite = struct.unpack(">HHHB", trame[:7])
    pdu = trame[7:]
    if protocole != 0:
        raise ErreurModbus("ce n'est pas du Modbus")
    if not pdu or len(pdu) != longueur - 1:
        raise ErreurModbus("taille incohérente")
    f, d = pdu[0], pdu[1:]

    if f in (LIRE_BOBINES, LIRE_REGISTRES, LIRE_ENTREES):
        if len(d) != 4:
            raise ErreurModbus("lecture mal formée")
        adresse, quantite = struct.unpack(">HH", d)
        if not 1 <= quantite <= (2000 if f == LIRE_BOBINES else 125):
            raise ErreurModbus("quantité hors norme")
        return Requete(transaction, unite, f, adresse, quantite)

    if f in (ECRIRE_BOBINE, ECRIRE_REGISTRE):
        if len(d) != 4:
            raise ErreurModbus("écriture mal formée")
        adresse, v = struct.unpack(">HH", d)
        if f == ECRIRE_BOBINE:
            if v not in (0xFF00, 0x0000):  # la norme n'autorise que ces deux valeurs
                raise ErreurModbus("valeur de bobine invalide")
            v = 1 if v else 0
        return Requete(transaction, unite, f, adresse, 1, [v])

    if f == ECRIRE_BOBINES:
        if len(d) < 5:
            raise ErreurModbus("écriture mal formée")
        adresse, quantite, nb = struct.unpack(">HHB", d[:5])
        octets = d[5:]
        if not 1 <= quantite <= 1968 or nb != (quantite + 7) // 8 or len(octets) != nb:
            raise ErreurModbus("écriture de bobines incohérente")
        return Requete(transaction, unite, f, adresse, quantite, [(octets[i // 8] >> (i % 8)) & 1 for i in range(quantite)])

    if f == ECRIRE_REGISTRES:
        if len(d) < 5:
            raise ErreurModbus("écriture mal formée")
        adresse, quantite, nb = struct.unpack(">HHB", d[:5])
        if not 1 <= quantite <= 123 or nb != 2 * quantite or len(d) != 5 + nb:
            raise ErreurModbus("écriture de registres incohérente")
        return Requete(transaction, unite, f, adresse, quantite, list(struct.unpack(f">{quantite}H", d[5:])))

    raise ErreurModbus(f"fonction {f} non prise en charge", FONCTION_ILLEGALE)


def _mbap(transaction: int, unite: int, pdu: bytes) -> bytes:
    return struct.pack(">HHHB", transaction, 0, len(pdu) + 1, unite) + pdu


def encoder_requete(transaction: int, unite: int, fonction: int, adresse: int,
                    quantite: int = 1, valeurs: list[int] | None = None) -> bytes:
    """Fabrique une requête (côté client : supervision, ou démonstration d'attaque)."""
    valeurs = valeurs or []
    if fonction in (LIRE_BOBINES, LIRE_REGISTRES, LIRE_ENTREES):
        pdu = struct.pack(">BHH", fonction, adresse, quantite)
    elif fonction == ECRIRE_BOBINE:
        pdu = struct.pack(">BHH", fonction, adresse, 0xFF00 if valeurs[0] else 0)
    elif fonction == ECRIRE_REGISTRE:
        pdu = struct.pack(">BHH", fonction, adresse, valeurs[0])
    elif fonction == ECRIRE_BOBINES:
        octets = bytearray((len(valeurs) + 7) // 8)
        for i, v in enumerate(valeurs):
            if v:
                octets[i // 8] |= 1 << (i % 8)
        pdu = struct.pack(">BHHB", fonction, adresse, len(valeurs), len(octets)) + bytes(octets)
    elif fonction == ECRIRE_REGISTRES:
        pdu = struct.pack(f">BHHB{len(valeurs)}H", fonction, adresse, len(valeurs), 2 * len(valeurs), *valeurs)
    else:
        raise ValueError(f"fonction {fonction} non prise en charge")
    return _mbap(transaction, unite, pdu)


def encoder_reponse(req: Requete, donnees: bytes) -> bytes:
    return _mbap(req.transaction, req.unite, bytes([req.fonction]) + donnees)


def reponse_exception(transaction: int, unite: int, fonction: int, code: int) -> bytes:
    """Réponse d'erreur : fonction + 0x80, puis le code d'exception."""
    return _mbap(transaction, unite, bytes([fonction | 0x80, code]))


def decoder_reponse(trame: bytes, req: Requete) -> list[int]:
    """Côté client : lit la réponse. Lève ErreurModbus si l'automate (ou le pare-feu) a refusé."""
    pdu = trame[7:]
    if pdu[0] == req.fonction | 0x80:
        raise ErreurModbus("refusé par l'équipement", pdu[1])
    if pdu[0] != req.fonction:
        raise ErreurModbus("réponse inattendue")
    if req.fonction == LIRE_BOBINES:
        return [(pdu[2 + i // 8] >> (i % 8)) & 1 for i in range(req.quantite)]
    if req.fonction in (LIRE_REGISTRES, LIRE_ENTREES):
        return list(struct.unpack(f">{req.quantite}H", pdu[2:2 + 2 * req.quantite]))
    return []
