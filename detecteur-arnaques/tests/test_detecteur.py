"""Tests du détecteur : texte, règles, classifieur, verdict."""
import unittest

from detecteur import Classifieur, analyser, charger, mots, normaliser, signaux

MODELE = Classifieur().apprendre(charger())


def regles(texte):
    return {r.nom for r in signaux(texte)}


class TestTexte(unittest.TestCase):
    def test_normaliser(self):
        self.assertEqual(normaliser("  Félicitations !\n Vous AVEZ gagné "), "felicitations ! vous avez gagne")

    def test_mots_jetons_et_mots_vides(self):
        m = mots("Envoyez 5 000 FDJ au 77 12 34 56 ici : http://bit.ly/abc")
        self.assertIn("_montant_", m)
        self.assertIn("_numero_", m)
        self.assertIn("_lien_", m)
        self.assertNotIn("au", m)


class TestRegles(unittest.TestCase):
    def test_demande_de_code(self):
        self.assertIn("code", regles("Communiquez votre code PIN à ce numéro"))
        self.assertIn("code", regles("Connectez-vous avec votre mot de passe"))

    def test_un_vrai_code_n_est_pas_une_demande(self):
        self.assertNotIn("code", regles("Votre code est 4821. Ne le communiquez à personne."))
        self.assertNotIn("urgence", regles("Votre code expire dans 5 minutes."))

    def test_autres_ruses(self):
        self.assertIn("erreur", regles("Je t'ai envoyé 10 000 FDJ par erreur"))
        self.assertIn("frais", regles("Payez 3 000 FDJ de frais de livraison"))
        self.assertIn("lien", regles("Voir www.exemple.xyz"))
        self.assertIn("agent", regles("Ici l'agent Waafi"))
        self.assertEqual(regles("On se voit demain au marché ?"), set())


class TestClassifieur(unittest.TestCase):
    def test_apprend_sur_un_petit_exemple(self):
        m = Classifieur().apprendre([("arnaque", "gagne loterie gratuit"), ("normal", "rendez-vous demain marche")])
        self.assertGreater(m.probabilite_arnaque("loterie gratuit"), 0.5)
        self.assertLess(m.probabilite_arnaque("demain marche"), 0.5)


class TestVerdict(unittest.TestCase):
    def test_messages_jamais_vus(self):
        cas = {
            "Bonjour, agent Waafi, votre compte sera suspendu. Envoyez le code reçu par SMS.": "arnaque",
            "J'ai transféré 10 000 FDJ par erreur sur ton numéro, renvoie-les stp": "arnaque",
            "Félicitations, vous avez gagné un bonus, cliquez sur www.bonus-djib.top": "arnaque",
            "Salut, tu viens ce soir chez Ali ? On regarde le match.": "rien",
            "Votre code de vérification est 482913. Ne le partagez avec personne.": "rien",
        }
        for texte, attendu in cas.items():
            self.assertEqual(analyser(texte, MODELE).niveau, attendu, texte)

    def test_l_ia_seule_ne_suffit_pas_a_accuser(self):
        # Mots d'arnaque mais aucune ruse reconnue par les règles : au plus « méfiance ».
        v = analyser("compte sms fdj envoyez numero", MODELE)
        self.assertNotEqual(v.niveau, "arnaque")

    def test_raisons_lisibles(self):
        v = analyser("Donnez-moi le code reçu, c'est urgent", MODELE)
        self.assertTrue(any("code" in r for r in v.raisons))

    def test_entrees_invalides(self):
        for texte in ("", "   ", "a" * 2001):
            with self.assertRaises(ValueError):
                analyser(texte, MODELE)


if __name__ == "__main__":
    unittest.main()
