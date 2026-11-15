"""Le scénario complet : automate + pare-feu lancés pour de vrai, attaque puis supervision."""
import asyncio
import json
import tempfile
import threading
import unittest
from pathlib import Path

import automate
import pare_feu
from client import ClientModbus
from modbus import ErreurModbus
from regles import Gardien


class TestScenario(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.journal = Path(tempfile.mkdtemp()) / "journal.jsonl"
        cls.memoire = automate.Memoire()
        cls.boucle = asyncio.new_event_loop()
        prets = threading.Event()

        async def demarrer():
            cls.srv_automate = await automate.servir(cls.memoire, port=0)
            port_automate = cls.srv_automate.sockets[0].getsockname()[1]
            cls.srv_pare_feu = await pare_feu.servir(Gardien(), port=0, automate=("127.0.0.1", port_automate), journal=cls.journal)
            cls.ports = (port_automate, cls.srv_pare_feu.sockets[0].getsockname()[1])
            prets.set()

        threading.Thread(target=lambda: (cls.boucle.run_until_complete(demarrer()), cls.boucle.run_forever()), daemon=True).start()
        prets.wait(5)

    @classmethod
    def tearDownClass(cls):
        async def arreter():  # fermer proprement serveurs et connexions avant d'arrêter la boucle
            for serveur in (cls.srv_pare_feu, cls.srv_automate):
                serveur.close()
            for tache in asyncio.all_tasks():
                if tache is not asyncio.current_task():
                    tache.cancel()
            await asyncio.sleep(0.05)

        asyncio.run_coroutine_threadsafe(arreter(), cls.boucle).result(5)
        cls.boucle.call_soon_threadsafe(cls.boucle.stop)

    def test_1_sans_pare_feu_l_attaque_reussit(self):
        attaquant = ClientModbus("127.0.0.1", self.ports[0])
        attaquant.ecrire_registres(0, [10, 10, 10])
        self.assertEqual(self.memoire.registres, [10, 10, 10])
        self.memoire.registres = [2000, 2800, 3500]  # on répare pour la suite
        attaquant.fermer()

    def test_2_a_travers_le_pare_feu_l_attaque_echoue(self):
        attaquant = ClientModbus("127.0.0.1", self.ports[1])
        self.assertEqual(len(attaquant.lire_bobines(0, 3)), 3)  # lire reste possible
        with self.assertRaises(ErreurModbus):
            attaquant.ecrire_registres(0, [10, 10, 10])
        with self.assertRaises(ErreurModbus):
            attaquant.ecrire_bobines(0, [0, 0, 0])
        self.assertEqual(self.memoire.registres, [2000, 2800, 3500])  # l'automate n'a rien reçu
        attaquant.fermer()

    def test_3_meme_en_usurpant_l_adresse_les_autres_regles_tiennent(self):
        usurpateur = ClientModbus("127.0.0.1", self.ports[1], "127.0.0.2")
        with self.assertRaises(ErreurModbus):
            usurpateur.ecrire_registres(0, [10, 10, 10])  # seuils hors plage
        with self.assertRaises(ErreurModbus):
            usurpateur.ecrire_bobines(0, [0, 0, 0])  # 3 ouvertures d'un coup
        usurpateur.fermer()

    def test_4_la_supervision_fonctionne_et_tout_est_journalise(self):
        supervision = ClientModbus("127.0.0.1", self.ports[1], "127.0.0.2")
        supervision.ecrire_registre(1, 3000)
        self.assertEqual(self.memoire.registres[1], 3000)
        supervision.fermer()
        lignes = [json.loads(l) for l in self.journal.read_text(encoding="utf-8").splitlines()]
        self.assertTrue(any(not l["autorise"] for l in lignes))
        self.assertTrue(any(l["autorise"] and l["ip"] == "127.0.0.2" for l in lignes))


if __name__ == "__main__":
    unittest.main()
