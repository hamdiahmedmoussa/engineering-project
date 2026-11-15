# Coupure intelligente de climatiseur

Éteint la clim quand la pièce est vide, l'empêche de tourner trop froid, et prévient quand elle
n'arrive plus à refroidir. Le tout se pilote depuis le téléphone, en phrases simples :
« Personne depuis 6 min. La clim s'éteindra dans 9 min si personne ne revient. »

**Aucun fil 220 V n'est touché.** Le boîtier parle à la clim **en infrarouge**, comme sa télécommande.
Couper l'alimentation d'une clim avec un relais serait dangereux (courant d'appel du compresseur,
risque d'abîmer la clim) et ne permettrait pas de régler la température.

## 1. Le matériel

| Pièce | Rôle | Prix indicatif |
|---|---|---|
| ESP32 DevKit | Le cerveau | 5 – 10 $ |
| LED infrarouge 940 nm + transistor 2N2222 + résistances 100 Ω et 1 kΩ | Envoie les ordres à la clim | 1 $ |
| Récepteur infrarouge **VS1838B** (ou TSOP38238) | Lit la vraie télécommande | 1 $ |
| Radar de présence **LD2410C** | Détecte une personne, **même immobile** (qui dort, qui lit) | 3 – 6 $ |
| Capteur **DHT22** (température et humidité) | La température de la pièce | 3 – 5 $ |
| Plaque d'essai, fils, alimentation USB 5 V | Montage | 3 $ |

**Pourquoi un radar et pas un détecteur PIR classique ?** Le PIR ne voit que les **mouvements** :
il croirait la pièce vide pendant que quelqu'un dort, et couperait la clim. Le LD2410 détecte la
respiration. On ajoute en plus un **mode nuit** (22 h – 7 h) où la clim ne s'éteint jamais seule.

## 2. Le schéma

```
  LED infrarouge (portée de plusieurs mètres grâce au transistor) :
    GPIO4 ──[1 kΩ]── base du 2N2222
    5 V ──[100 Ω]── (+) LED IR (−) ── collecteur du 2N2222
    émetteur du 2N2222 ── GND

  Récepteur VS1838B (face bombée devant toi) :  OUT → GPIO15   GND → GND   VCC → 3,3 V

  Radar LD2410C :   OUT → GPIO27   GND → GND   VCC → 5 V

  DHT22 :           DATA → GPIO26 (+ résistance 10 kΩ vers 3,3 V)   GND → GND   VCC → 3,3 V
```

Place la LED **face à la clim**, à moins de 5 m, et le radar **face au lit ou au canapé**.

## 3. Les étapes

### Étape A : sans rien brancher (simulation)

1. Arduino IDE → *Gérer les bibliothèques* : installe **IRremoteESP8266** et **DHT sensor library** (Adafruit).
2. Ouvre `firmware/coupure-clim/coupure-clim.ino`, **copie `config.exemple.h` en `config.h`**, mets ton Wi-Fi.
3. Téléverse et ouvre l'adresse affichée par le *Moniteur série*.

En simulation, **une minute dure une seconde**. Tu vois en 3 minutes tout le scénario : quelqu'un rallume
la clim à 20 °C, le mode éco la remonte à 24 °C, la personne part, la clim s'éteint 15 « minutes » plus tard.

### Étape B : trouver la marque de ta clim

1. Branche **seulement le récepteur infrarouge**, mets `MODE_SIMULATION false`, téléverse.
2. Ouvre le *Moniteur série*, pointe la télécommande vers le récepteur et appuie sur un bouton.
3. Il affiche par exemple `Signal infrarouge reçu, protocole : GREE`.
4. Recopie ce nom dans `PROTOCOLE_CLIM` (dans `config.h`) et téléverse à nouveau.

Si le protocole affiché est `UNKNOWN`, ta clim n'est pas reconnue par la bibliothèque : essaie de nouveau
plus près du récepteur. Si ça ne change rien, note la marque et le modèle, il existe d'autres méthodes.

### Étape C : tout brancher et tester

1. Fais le montage complet.
2. Sur le téléphone, appuie sur **« Allumer à 25 °C »** : la clim doit biper et démarrer.
3. Utilise la vraie télécommande : le tableau de bord doit afficher le nouvel état
   (« Télécommande : réglée sur 22 °C »). Le boîtier **sait toujours** ce que fait la clim.
4. Sors de la pièce 15 minutes (en journée) : la clim s'éteint et le journal l'indique.

### Étape D : régler les règles

Tout est dans `config.h` : délai sans présence, heures de nuit, consigne minimale (mode éco),
seuil d'alerte. Mets `PUISSANCE_CLIM_W` à la valeur **mesurée avec ton compteur d'énergie** :
le tableau de bord en déduit les kWh économisés.

## 4. Les règles (fichier `regles.h`, testé)

1. Clim éteinte → rien à faire.
2. Personne depuis 15 min → **éteindre**, sauf la nuit.
3. Réglée plus froid que 24 °C → **remonter à 24 °C**, une seule fois par allumage.
   Si quelqu'un la rebaisse ensuite avec la télécommande, on respecte son choix.
4. Encore 32 °C après 45 min de clim → **alerte** : filtre encrassé (très fréquent avec la poussière),
   fenêtre ouverte, ou manque de gaz.

```bash
g++ -std=c++17 -o test tests/test_regles.cpp && ./test
```

## 5. Organisation du code

| Fichier | Rôle |
|---|---|
| `firmware/coupure-clim/coupure-clim.ino` | Programme principal : infrarouge, capteurs, journal, serveur web |
| `firmware/coupure-clim/regles.h` | Les règles de décision |
| `firmware/coupure-clim/page.h` | Le tableau de bord |
| `firmware/coupure-clim/config.exemple.h` | Modèle des réglages |
| `tests/test_regles.cpp` | Tests des règles, sur ordinateur |

## 6. Bon à savoir

- La page et les commandes sont protégées par mot de passe, et les commandes refusent les autres sites (voir `SECURITE.md`).
- Si la compilation échoue dans les fichiers d'IRremoteESP8266, installe la **dernière version** de la
  bibliothèque. Si le problème continue, essaie la carte **esp32 version 2.0.17** dans le gestionnaire de cartes.
- Ce projet utilise le même ESP32 que le compteur d'énergie : tu peux même les réunir plus tard dans un
  seul boîtier qui mesure la consommation **et** pilote la clim.

## Sécurité

Avant de téléverser, choisis dans `config.h` un **mot de passe pour le tableau de bord** (`MOT_DE_PASSE_TABLEAU`)
et un **mot de passe pour le réseau du boîtier** (`RESEAU_BOITIER_MOT_DE_PASSE`) : avec ceux de l'exemple,
le programme refuse de compiler. Le navigateur demandera ensuite l'identifiant et le mot de passe.
Détails : [`SECURITE.md`](../SECURITE.md).
