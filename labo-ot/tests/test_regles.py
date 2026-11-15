"""Les règles du pare-feu, une par une."""
import unittest

from modbus import ECRIRE_BOBINE, ECRIRE_BOBINES, ECRIRE_REGISTRE, ECRIRE_REGISTRES, LIRE_REGISTRES, Requete
from regles import Gardien

SUPERVISION, INCONNU = "127.0.0.2", "192.168.1.66"


def req(fonction, adresse, valeurs=()):
    return Requete(1, 1, fonction, adresse, max(1, len(valeurs)), list(valeurs))


class TestRegles(unittest.TestCase):
    def setUp(self):
        self.g = Gardien()

    def test_tout_le_monde_peut_lire(self):
        self.assertTrue(self.g.examiner(req(LIRE_REGISTRES, 0), INCONNU, 0).autorise)

    def test_seule_la_supervision_peut_ecrire(self):
        self.assertFalse(self.g.examiner(req(ECRIRE_BOBINE, 0, [0]), INCONNU, 0).autorise)
        self.assertTrue(self.g.examiner(req(ECRIRE_BOBINE, 0, [0]), SUPERVISION, 0).autorise)

    def test_seuils_hors_plage_refuses_meme_par_la_supervision(self):
        d = self.g.examiner(req(ECRIRE_REGISTRE, 0, [10]), SUPERVISION, 0)  # 1 A : sabotage
        self.assertFalse(d.autorise)
        self.assertIn("hors de la plage", d.raison)
        self.assertTrue(self.g.examiner(req(ECRIRE_REGISTRE, 0, [2200]), SUPERVISION, 1).autorise)
        self.assertFalse(self.g.examiner(req(ECRIRE_REGISTRE, 9, [2200]), SUPERVISION, 2).autorise)

    def test_jamais_plusieurs_disjoncteurs_ouverts_d_un_coup(self):
        self.assertFalse(self.g.examiner(req(ECRIRE_BOBINES, 0, [0, 0, 0]), SUPERVISION, 0).autorise)
        self.assertTrue(self.g.examiner(req(ECRIRE_BOBINES, 0, [0, 1, 1]), SUPERVISION, 1).autorise)
        self.assertFalse(self.g.examiner(req(ECRIRE_BOBINE, 5, [1]), SUPERVISION, 2).autorise)

    def test_trop_d_ecritures_par_minute(self):
        for t in range(6):
            self.assertTrue(self.g.examiner(req(ECRIRE_BOBINE, 0, [1]), SUPERVISION, t).autorise)
        self.assertFalse(self.g.examiner(req(ECRIRE_BOBINE, 0, [1]), SUPERVISION, 6).autorise)
        self.assertTrue(self.g.examiner(req(ECRIRE_BOBINE, 0, [1]), SUPERVISION, 70).autorise)  # une minute après

    def test_fonction_interdite(self):
        self.assertFalse(self.g.examiner(req(8, 0), SUPERVISION, 0).autorise)  # 8 : diagnostic


if __name__ == "__main__":
    unittest.main()
