# Compteur d'énergie connecté

Mesure en direct ce que consomme un appareil (climatiseur, frigo, chauffe-eau…) et l'affiche
sur ton téléphone : **watts, ampères, kWh, et ce que ça coûte en FDJ par jour et par mois**.

ESP32 + pince ampèremétrique **non intrusive** (on ne coupe aucun fil). Le tableau de bord est
servi par l'ESP32 lui-même : pas besoin d'internet, tout reste dans la maison.

## 1. Le matériel

| Pièce | Rôle | Prix indicatif |
|---|---|---|
| ESP32 DevKit (ESP32-WROOM-32) | Le cerveau : mesure, calcule, sert la page web | 5 – 10 $ |
| Pince **SCT-013-030** (30 A / 1 V, résistance intégrée) | Mesure le courant sans toucher aux fils | 5 – 8 $ |
| Prise jack femelle 3,5 mm (pour brancher la pince) | Relie la pince à la plaque | 1 $ |
| 2 résistances 10 kΩ | Décalent le signal à 1,65 V | < 1 $ |
| 1 condensateur 10 µF (électrolytique) | Stabilise ce décalage | < 1 $ |
| Plaque d'essai (breadboard) + fils | Montage sans soudure | 3 $ |
| Câble USB (selon ta carte : micro-USB ou USB-C) | Alimentation et téléversement | – |
| **Une rallonge électrique « dédoublée »** (voir sécurité) | Pour accrocher la pince sur un seul fil | – |

**Important : prends bien la version SCT-013-0*30*.** Elle sort une tension (1 V pour 30 A).
Les versions « 100 A » sortent un courant et demandent une résistance en plus.

## 2. ⚠️ Sécurité (à lire avant de commencer)

- **Tu ne touches jamais au 220 V.** La pince se referme *autour* d'un fil, sans le dénuder ni le couper.
- **La pince doit entourer UN SEUL fil** (la phase OU le neutre), jamais le câble entier :
  les deux courants s'annulent et tu lirais 0 A.
- Pour ça, utilise une **rallonge dont la gaine extérieure a été ouverte sur 10 cm par un électricien**
  ou un **adaptateur de mesure** vendu pour les pinces ampèremétriques. Ne fends pas toi-même
  un câble branché.
- Le climatiseur se branche sur la rallonge, la pince entoure un fil de la rallonge.
  L'ESP32, lui, n'est relié qu'à la pince et à l'USB : **aucun contact avec le secteur**.
- Fais vérifier ton premier montage par ton professeur ou un électricien.

## 3. Le schéma

```
          3,3 V (ESP32)
             │
           [10 kΩ]
             │
             ├──────────────┬──────────── fil 1 de la pince (manchon du jack)
             │              │
           [10 kΩ]       [10 µF]  (+ vers le haut)
             │              │
           GND ─────────────┘

   fil 2 de la pince (pointe du jack) ─────── GPIO34 (ESP32)
```

Les deux résistances créent un point à **1,65 V** (la moitié de 3,3 V). La pince fait osciller
la tension autour de ce point : entre environ 0,2 V et 3,1 V à 30 A, toujours dans ce que l'ESP32 sait lire.

## 4. Les étapes

### Étape A : sans la pince (mode simulation)

Tu peux tout tester avec **seulement l'ESP32** :

1. Installe l'**Arduino IDE**, puis dans *Fichier → Préférences → URL de gestionnaire de cartes* ajoute :
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. *Outils → Carte → Gestionnaire de cartes* : installe **esp32** (par Espressif).
3. Ouvre `firmware/compteur-energie/compteur-energie.ino`.
4. Dans le même dossier, **copie `config.exemple.h` et renomme la copie `config.h`**.
   Mets-y le nom et le mot de passe de ton Wi-Fi. Laisse `MODE_SIMULATION true`.
5. *Outils → Carte* : **ESP32 Dev Module**. Choisis le bon port, puis **Téléverser** (→).
6. Ouvre le *Moniteur série* (115200 bauds) : il affiche l'adresse, par exemple `http://192.168.1.42`.
7. Ouvre cette adresse sur ton téléphone (même Wi-Fi) : le tableau de bord montre une clim simulée.

Si le Wi-Fi n'est pas trouvé, le compteur crée son propre réseau **Compteur-Energie**
(mot de passe dans `config.h`) : connecte ton téléphone dessus et ouvre `http://192.168.4.1`.

### Étape B : avec la pince

1. Débranche l'USB. Fais le montage du schéma.
2. Dans `config.h`, mets `MODE_SIMULATION false`, puis téléverse à nouveau.
3. Sans rien dans la pince, le courant doit afficher **0 A**.
4. Referme la pince sur **un fil** de la rallonge, branche l'appareil : les mesures apparaissent.

### Étape C : calibrer (rendre la mesure juste)

Les pinces et l'ESP32 ont de petites erreurs. Pour les corriger :

1. Mesure le même fil avec une **pince multimètre** (à l'université ou chez un électricien).
2. Calcule : `CALIBRATION = courant du multimètre ÷ courant affiché`.
   Exemple : le multimètre lit 5,2 A, le compteur 4,9 A → `CALIBRATION 1.06`.
3. Mets cette valeur dans `config.h` et téléverse.

### Étape D : le tarif

Sur le tableau de bord, entre le **prix du kWh écrit sur ta facture d'électricité** (EDD).
Le compteur affiche alors le coût depuis la mise en marche, puis une estimation par jour et par mois,
calculée sur la **moyenne** (une clim s'allume et s'éteint sans arrêt).

## 5. Ce que le compteur ne mesure pas (encore)

- **La tension réelle.** On suppose 220 V. Si ta tension varie (210 V, 230 V…), l'erreur suit.
- **Le facteur de puissance.** On calcule une puissance *apparente* (V × A). Pour un moteur
  de clim, la puissance *réelle* est un peu plus faible (facteur de puissance de 0,8 à 0,95 environ).

**Version 2** : le module **PZEM-004T v3** mesure tension, courant, puissance réelle, facteur de puissance
et fréquence. Il se branche, lui, sur le 220 V : à faire **avec ton professeur**.

## 6. Organisation du code

| Fichier | Rôle |
|---|---|
| `firmware/compteur-energie/compteur-energie.ino` | Programme principal : mesure chaque seconde, Wi-Fi, serveur web |
| `firmware/compteur-energie/mesure.h` | Les calculs : valeur efficace (RMS), courant, énergie |
| `firmware/compteur-energie/page.h` | Le tableau de bord (page web) |
| `firmware/compteur-energie/config.exemple.h` | Modèle des réglages (Wi-Fi, calibration, simulation) |
| `tests/test_mesure.cpp` | Tests des calculs, sur ordinateur |

Tester les calculs sur ton PC (avec g++ installé) :

```bash
g++ -std=c++17 -o test tests/test_mesure.cpp && ./test
```

## 7. La valeur efficace, en deux lignes

Le courant du secteur est une sinusoïde à 50 Hz : sa moyenne vaut 0. On utilise donc la **valeur efficace** :
on élève chaque échantillon au carré, on fait la moyenne, puis la racine carrée.
On mesure 2 000 échantillons sur 200 ms, soit **10 périodes entières**, pour que le calcul soit exact.

## Sécurité

Avant de téléverser, choisis dans `config.h` un **mot de passe pour le tableau de bord** (`MOT_DE_PASSE_TABLEAU`)
et un **mot de passe pour le réseau du boîtier** (`RESEAU_BOITIER_MOT_DE_PASSE`) : avec ceux de l'exemple,
le programme refuse de compiler. Le navigateur demandera ensuite l'identifiant et le mot de passe.
Détails : [`SECURITE.md`](../SECURITE.md).
