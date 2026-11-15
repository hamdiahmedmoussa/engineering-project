# Laboratoire de sécurité industrielle (Modbus)

Un petit **poste électrique simulé** sur ton ordinateur, pour comprendre comment on attaque et comment on
défend les systèmes qui font tourner l'électricité, l'eau, les ports et les usines.
C'est le croisement exact de l'**électricité** et de la **cybersécurité** : la sécurité « OT »
(*Operational Technology*).

```
   Supervision (page web)                Pare-feu Modbus                 Automate du poste
   http://127.0.0.1:8080   ──Modbus──►   port 5020          ──Modbus──►  port 5021
   depuis 127.0.0.2                      lit chaque message               3 départs : Hôpital,
                                         et bloque l'anormal              Quartier, Zone portuaire
                                         journal-pare-feu.jsonl
                     ▲
   Démonstration d'attaque (--labo) ─────┘ ou directement sur l'automate
```

**Tout est en Python, sans aucune bibliothèque à installer** (Python 3.10 ou plus récent).
Le protocole Modbus est écrit à la main dans `modbus.py`, pour le comprendre octet par octet.

## ⚠️ La règle d'or

Ce laboratoire ne tourne **que sur ton ordinateur**. La démonstration d'attaque refuse toute cible qui
n'est pas ton ordinateur ou ton réseau local, et exige l'option `--labo`.
Viser un équipement réel (automate, poste, usine, université) sans **autorisation écrite** est un délit,
et sur un système électrique, cela peut mettre des vies en danger.

## 1. Pourquoi Modbus est dangereux

Modbus date de 1979. À l'époque, les automates étaient reliés par un câble fermé, dans l'usine.
Il n'a donc **aucun mot de passe** : quiconque peut parler à l'automate peut **lire et écrire**.
Aujourd'hui, beaucoup de ces automates sont reliés à des réseaux, parfois à internet.

L'attaque de la démonstration est celle des vraies attaques contre des réseaux électriques :
on ne coupe pas forcément le courant directement. On **sabote la protection** (seuils à 1 A),
et c'est l'automate lui-même qui coupe tout, en croyant protéger le réseau.

## 2. Lancer le laboratoire

Ouvre **3 terminaux** dans le dossier `labo-ot` :

```bash
python automate.py       # 1. le poste électrique
python pare_feu.py       # 2. le pare-feu, devant l'automate
python supervision.py    # 3. la salle de contrôle → ouvre http://127.0.0.1:8080
```

Sur la page : ouvre et referme un disjoncteur, monte un seuil. Tout passe par le pare-feu
et apparaît dans son journal.

## 3. Les expériences

**Expérience 1 · L'attaque sans protection** (4e terminal) :

```bash
python demonstration_attaque.py --labo                # vise l'automate en direct (port 5021)
```

La supervision affiche **« Coupure : tout le poste est sans courant »**, et le pare-feu n'a **rien vu**.
Leçon : dans un vrai poste, l'automate ne doit être joignable **que** par le pare-feu (réseau séparé).
Pour réparer : bouton **« Rétablir les seuils normaux »**, puis referme les 3 disjoncteurs.

**Expérience 2 · La même attaque à travers le pare-feu** :

```bash
python demonstration_attaque.py --labo --port 5020
```

La lecture passe (l'attaquant voit l'état du poste), mais le sabotage et la coupure sont **bloqués**,
et le journal affiche des alertes en rouge.

**Expérience 3 · L'attaquant usurpe l'adresse de la supervision** :

```bash
python demonstration_attaque.py --labo --port 5020 --usurper
```

La règle « seule la supervision écrit » est contournée… mais les autres tiennent : seuils hors de la plage
raisonnable, 3 disjoncteurs ouverts d'un coup. C'est la **défense en profondeur**.

## 4. Les règles du pare-feu (`regles.py`)

| Règle | Ce qu'elle bloque |
|---|---|
| Seule la supervision (127.0.0.2) peut écrire | Un appareil inconnu qui donne des ordres |
| Seules les fonctions utiles au poste | Diagnostic, identification, fonctions exotiques |
| Seuils de protection dans une plage raisonnable, par départ | Le sabotage de la protection |
| Jamais plus d'un disjoncteur ouvert par commande | La coupure générale en un seul message |
| Au plus 6 écritures par minute | Un script qui mitraille les commandes |
| Trames mal formées refusées | Les messages abîmés ou fabriqués pour faire planter l'automate |

## 5. La version ESP32 : un vrai automate

Un ESP32 remplace `automate.py` : c'est un **vrai équipement Modbus TCP** (port 502), avec 3 **LED**
comme disjoncteurs (allumée = départ alimenté). Il crée **son propre réseau Wi-Fi « Labo-OT »**,
pour que cet automate volontairement vulnérable ne soit **jamais** sur le Wi-Fi de la maison.

**Matériel** : un ESP32, 3 LED, 3 résistances de 220 Ω (environ 7 dollars). Uniquement de la basse tension.

```
  GPIO25 ──[220 Ω]──▶|── GND    Hôpital
  GPIO26 ──[220 Ω]──▶|── GND    Quartier résidentiel
  GPIO27 ──[220 Ω]──▶|── GND    Zone portuaire          (patte longue de la LED côté résistance)
```

1. Dans `firmware/automate-esp32/`, copie `config.exemple.h` en `config.h` et **change le mot de passe**
   (sinon le programme refuse de compiler). Téléverse `automate-esp32.ino`.
2. Connecte ton PC au Wi-Fi **Labo-OT**. L'automate est à l'adresse **192.168.4.1**.
3. Sur le PC :

```bash
python pare_feu.py --automate 192.168.4.1:502
python supervision.py
python demonstration_attaque.py --labo --hote 192.168.4.1 --port 502   # attaque directe : les LED s'éteignent
python demonstration_attaque.py --labo --port 5020                     # via le pare-feu : bloquée
```

Le *Moniteur série* (115200 bauds) affiche chaque disjoncteur ouvert et chaque déclenchement de la protection.

## 6. Tests

```bash
python -m unittest discover -s tests -t .
```

13 tests : le protocole (aller-retour de chaque fonction, trames coupées par TCP, trames abîmées),
chaque règle du pare-feu, et un **scénario complet** qui lance l'automate et le pare-feu pour de vrai,
puis joue l'attaque directe (réussie), l'attaque via le pare-feu (bloquée), l'usurpation (bloquée)
et la supervision (autorisée et journalisée).

Tests de l'automate ESP32 (sur ordinateur) :

```bash
g++ -std=c++17 -o test firmware/tests/test_poste.cpp && ./test
```

**Sous Windows**, si la supervision n'arrive pas à se connecter depuis 127.0.0.2, lance-la avec un autre
ordinateur du réseau local, et mets son adresse dans `ecrivains` (`regles.py`).

## 7. Organisation du code

| Fichier | Rôle |
|---|---|
| `modbus.py` | Le protocole Modbus TCP : lire, vérifier et fabriquer les trames |
| `automate.py` | Le poste simulé : mémoire Modbus, courants, protection qui déclenche |
| `regles.py` | Les règles du pare-feu (logique pure, testée) |
| `pare_feu.py` | Le pare-feu : relaie, bloque et journalise |
| `client.py` | Un client Modbus minimal |
| `supervision.py` + `supervision.html` | La salle de contrôle |
| `demonstration_attaque.py` | L'attaque, limitée à ton laboratoire |
| `firmware/automate-esp32/poste.h` | L'automate ESP32 : réponses Modbus (même mémoire que `automate.py`), testé sur ordinateur |
| `firmware/automate-esp32/automate-esp32.ino` | Réseau « Labo-OT », serveur Modbus (4 clients), LED, protection |

## 8. Pour aller plus loin

- **Le pare-feu sur un Raspberry Pi à deux interfaces** (Wi-Fi d'un côté, Ethernet de l'autre) : l'automate
  n'est alors physiquement joignable QUE par le pare-feu, comme dans un vrai poste.
- **Détecter sans bloquer** : un système de détection d'intrusion qui apprend le trafic « normal »
  (qui écrit quoi, à quelle fréquence) et alerte sur ce qui sort de l'ordinaire.
- **Les normes** : IEC 62443 (sécurité des systèmes industriels) et le modèle de Purdue (zones et conduits).
- **Modbus sécurisé** : la variante « Modbus/TCP Security » ajoute TLS et des certificats.
