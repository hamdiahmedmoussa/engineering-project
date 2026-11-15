"""Le protocole : trames bien lues, trames abîmées refusées."""
import struct
import unittest

from modbus import (ECRIRE_BOBINE, ECRIRE_BOBINES, ECRIRE_REGISTRES, LIRE_REGISTRES, ErreurModbus,
                    decoder_requete, encoder_requete, extraire_trame)


class TestModbus(unittest.TestCase):
    def test_aller_retour_pour_chaque_fonction(self):
        cas = [(LIRE_REGISTRES, 0, 3, None, []), (ECRIRE_BOBINE, 2, 1, [1], [1]),
               (ECRIRE_BOBINES, 0, 3, [1, 0, 1], [1, 0, 1]), (ECRIRE_REGISTRES, 1, 2, [2500, 3000], [2500, 3000])]
        for fonction, adresse, quantite, valeurs, attendu in cas:
            req = decoder_requete(encoder_requete(7, 1, fonction, adresse, quantite, valeurs))
            self.assertEqual((req.transaction, req.fonction, req.adresse, req.quantite, req.valeurs),
                             (7, fonction, adresse, quantite, attendu))

    def test_tcp_peut_couper_ou_coller_les_trames(self):
        a, b = encoder_requete(1, 1, LIRE_REGISTRES, 0, 3), encoder_requete(2, 1, LIRE_REGISTRES, 0, 1)
        self.assertEqual(extraire_trame(a[:5]), (None, a[:5]))  # trame incomplète : on attend la suite
        trame, reste = extraire_trame(a + b)
        self.assertEqual((trame, reste), (a, b))

    def test_trames_abimees_refusees(self):
        bonne = encoder_requete(1, 1, ECRIRE_BOBINE, 0, 1, [1])
        mauvais_protocole = bonne[:2] + b"\x00\x07" + bonne[4:]
        mauvaise_valeur = bonne[:10] + b"\x12\x34"          # une bobine ne vaut que 0xFF00 ou 0x0000
        fonction_inconnue = struct.pack(">HHHBB", 1, 0, 2, 1, 0x2B)  # 43 : lecture d'identification
        trop_court = bonne[:-1][:4] + struct.pack(">H", 5) + bonne[6:-1]
        for trame in (mauvais_protocole, mauvaise_valeur, fonction_inconnue, trop_court):
            with self.assertRaises(ErreurModbus):
                decoder_requete(trame)
        with self.assertRaises(ErreurModbus):
            extraire_trame(b"\x00\x01\x00\x00\x09\x99\x01")  # longueur annoncée absurde


if __name__ == "__main__":
    unittest.main()
