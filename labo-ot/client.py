"""UN CLIENT MODBUS TCP MINIMAL (utilisé par la supervision et par la démonstration)."""
from __future__ import annotations

import socket

from modbus import (ECRIRE_BOBINE, ECRIRE_BOBINES, ECRIRE_REGISTRE, ECRIRE_REGISTRES, LIRE_BOBINES, LIRE_ENTREES,
                    LIRE_REGISTRES, decoder_reponse, decoder_requete, encoder_requete, extraire_trame)


class ClientModbus:
    def __init__(self, hote: str, port: int, adresse_source: str | None = None) -> None:
        # adresse_source : l'adresse depuis laquelle on se connecte (la supervision utilise 127.0.0.2).
        self.prise = socket.create_connection((hote, port), timeout=3,
                                              source_address=(adresse_source, 0) if adresse_source else None)
        self.transaction = 0

    def _demander(self, fonction: int, adresse: int, quantite: int = 1, valeurs: list[int] | None = None) -> list[int]:
        self.transaction = (self.transaction + 1) & 0xFFFF
        trame = encoder_requete(self.transaction, 1, fonction, adresse, quantite, valeurs)
        self.prise.sendall(trame)
        tampon, reponse = b"", None
        while reponse is None:
            morceau = self.prise.recv(260)
            if not morceau:
                raise ConnectionError("connexion fermée")
            tampon += morceau
            reponse, _ = extraire_trame(tampon)
        return decoder_reponse(reponse, decoder_requete(trame))

    def lire_bobines(self, adresse: int, n: int) -> list[int]:
        return self._demander(LIRE_BOBINES, adresse, n)

    def lire_registres(self, adresse: int, n: int) -> list[int]:
        return self._demander(LIRE_REGISTRES, adresse, n)

    def lire_mesures(self, adresse: int, n: int) -> list[int]:
        return self._demander(LIRE_ENTREES, adresse, n)

    def ecrire_bobine(self, adresse: int, valeur: int) -> None:
        self._demander(ECRIRE_BOBINE, adresse, 1, [valeur])

    def ecrire_bobines(self, adresse: int, valeurs: list[int]) -> None:
        self._demander(ECRIRE_BOBINES, adresse, len(valeurs), valeurs)

    def ecrire_registre(self, adresse: int, valeur: int) -> None:
        self._demander(ECRIRE_REGISTRE, adresse, 1, [valeur])

    def ecrire_registres(self, adresse: int, valeurs: list[int]) -> None:
        self._demander(ECRIRE_REGISTRES, adresse, len(valeurs), valeurs)

    def fermer(self) -> None:
        self.prise.close()
