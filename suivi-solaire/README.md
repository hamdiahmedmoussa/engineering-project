# Suivi d'un panneau solaire

Mesure ce que produit **vraiment** un panneau solaire, et surtout **ce que lui coûtent la chaleur
et la poussière**, deux problèmes majeurs à Djibouti. Tout s'affiche sur le téléphone, en phrases simples :
« La chaleur lui fait perdre environ 13 %. À 25 °C, il donnerait 15,9 W. »

ESP32 + INA219 (tension et courant) + sonde DS18B20 (température du panneau).
Même style de tableau de bord que le compteur d'énergie.

## 1. Le matériel

| Pièce | Rôle | Prix indicatif |
|---|---|---|
| ESP32 DevKit (celui du compteur convient) | Mesure, calcule, sert la page web | 5 – 10 $ |
| **Panneau solaire « 12 V », 10 à 20 W** | Ce qu'on étudie | 15 – 30 $ |
| Module **INA219** | Mesure tension et courant du panneau | 2 – 4 $ |
| Sonde **DS18B20 étanche** + résistance 4,7 kΩ | Température du dos du panneau | 2 – 4 $ |
| **Résistance de puissance** (voir le calcul plus bas), 25 ou 50 W, boîtier aluminium | La « charge » : le panneau doit débiter dans quelque chose pour produire | 3 – 6 $ |
| Ruban aluminium ou pâte thermique | Coller la sonde au dos du panneau | 1 $ |
| Plaque d'essai, fils, bornier à vis | Montage | 3 $ |

**⚠️ Prends un panneau « 12 V » (tension à vide, « Voc », inférieure à 23 V).** L'INA219 supporte
26 V au maximum. Un panneau « 24 V » (Voc autour de 40 V) le détruirait.

### Choisir la résistance de charge

Sur l'étiquette du panneau : **Vmp** (tension au point de puissance maximale) et **Imp** (courant).
La résistance idéale vaut **R = Vmp ÷ Imp**.

| Panneau | Vmp | Imp | R conseillée | Puissance de la résistance |
|---|---|---|---|---|
| 10 W | ≈ 18 V | ≈ 0,55 A | 33 Ω | 25 W |
| 20 W | ≈ 18 V | ≈ 1,1 A | 16 Ω (ou 15 Ω) | 50 W |

Prends toujours une résistance **2 fois plus puissante** que le panneau : elle chauffe beaucoup.

**Plus tard (version 2)** : remplacer la résistance par un régulateur de charge et une batterie 12 V.
L'énergie servira à quelque chose (lumière, ventilateur, téléphone) au lieu de chauffer.

## 2. ⚠️ Sécurité

- **La résistance de charge devient brûlante** (plus de 80 °C). Visse-la sur un support métallique,
  loin de tout ce qui brûle, et ne la touche pas pendant le fonctionnement.
- Un panneau au soleil produit du courant **dès qu'il est éclairé**. Câble-le **face contre le sol**
  ou couvert d'un carton, puis découvre-le à la fin.
- Respecte le sens **+ / −** du panneau.
- Ici, tout reste en basse tension continue (moins de 23 V) : pas de 220 V dans ce projet.

## 3. Le schéma

```
  Panneau +  ─────────── VIN+  [ INA219 ]  VIN− ─────── Résistance de charge ───┐
                                                                                  │
  Panneau −  ─────────────────────────────────────────────────────────────────────┘
       │
       └──── GND de l'INA219 ──── GND de l'ESP32     (masse commune obligatoire)

  INA219 :  VCC → 3,3 V    SDA → GPIO21    SCL → GPIO22

  DS18B20 : rouge → 3,3 V    noir → GND    jaune → GPIO4
            + résistance 4,7 kΩ entre jaune et 3,3 V
            La sonde est collée au CENTRE du dos du panneau (ruban aluminium).
```

L'INA219 mesure le courant grâce à une toute petite résistance interne (0,1 Ω) placée sur le fil +,
et la tension de ce même fil par rapport à la masse.

## 4. Les étapes

### Étape A : sans rien brancher (simulation)

1. Dans l'Arduino IDE, *Outils → Gérer les bibliothèques* : installe **Adafruit INA219**,
   **OneWire** et **DallasTemperature**. (La carte **esp32** est déjà installée si tu as fait le compteur.)
2. Ouvre `firmware/suivi-solaire/suivi-solaire.ino`.
3. **Copie `config.exemple.h` et renomme la copie `config.h`.** Mets ton Wi-Fi, laisse `MODE_SIMULATION true`.
4. Téléverse, ouvre le *Moniteur série* (115200 bauds) et l'adresse affichée sur ton téléphone.

Avec internet, l'ESP32 récupère l'heure exacte et simule le vrai soleil de la journée (rien la nuit).
Sans internet, il fait défiler une journée entière en 24 minutes.

### Étape B : avec le panneau

1. Fais le montage du schéma, **panneau couvert**.
2. Dans `config.h` : `MODE_SIMULATION false`, et la **puissance** et le **coefficient de température**
   de ton panneau (étiquette ou fiche technique : « Temperature coefficient of Pmax », souvent -0,35 à -0,45 %/°C).
3. Téléverse, puis découvre le panneau au soleil.

### Étape C : les expériences

1. **La chaleur** : note la production à 9 h (panneau encore frais) puis à 13 h (panneau brûlant).
   La page affiche la perte due à la chaleur, et ce que le panneau donnerait à 25 °C.
2. **La poussière** : laisse le panneau quelques jours dehors sans le toucher. Puis nettoie-le et
   appuie sur **« J'ai nettoyé le panneau »**. Compare la courbe avant et après, **à la même heure**,
   par **beau temps**. Utilise la valeur « à 25 °C » pour que la chaleur ne fausse pas la comparaison.
3. **L'inclinaison** : à Djibouti (latitude 11,5° nord), un panneau incliné d'environ 10 à 15 °,
   tourné vers le sud, produit le mieux sur l'année. Essaie à plat, puis incliné, puis vers l'est.

Ces mesures peuvent devenir un vrai **rapport de projet** : tableaux, courbes, conclusions chiffrées.

## 5. Les calculs

| Grandeur | Formule |
|---|---|
| Puissance | P = U × I |
| Énergie du jour | somme de P × durée (en Wh), remise à zéro à minuit si l'heure est connue |
| Perte due à la chaleur | coefficient × (T − 25 °C), par exemple 0,40 % × (65 − 25) = 16 % |
| Puissance à 25 °C | P ÷ (1 + coefficient × (T − 25)) |

La courbe garde **une moyenne par minute** pendant 24 heures (1 440 points) dans la mémoire de l'ESP32.
Elle est perdue si on débranche le boîtier.

## 6. Organisation du code

| Fichier | Rôle |
|---|---|
| `firmware/suivi-solaire/suivi-solaire.ino` | Programme principal : mesures, courbe de la journée, Wi-Fi, serveur web |
| `firmware/suivi-solaire/solaire.h` | Les calculs : énergie, perte due à la chaleur, puissance à 25 °C |
| `firmware/suivi-solaire/page.h` | Le tableau de bord |
| `firmware/suivi-solaire/config.exemple.h` | Modèle des réglages |
| `tests/test_solaire.cpp` | Tests des calculs, sur ordinateur |

```bash
g++ -std=c++17 -o test tests/test_solaire.cpp && ./test
```

## Sécurité

Avant de téléverser, choisis dans `config.h` un **mot de passe pour le tableau de bord** (`MOT_DE_PASSE_TABLEAU`)
et un **mot de passe pour le réseau du boîtier** (`RESEAU_BOITIER_MOT_DE_PASSE`) : avec ceux de l'exemple,
le programme refuse de compiler. Le navigateur demandera ensuite l'identifiant et le mot de passe.
Détails : [`SECURITE.md`](../SECURITE.md).
