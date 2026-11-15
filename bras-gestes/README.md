# Bras robot piloté par la main

La webcam filme ta main, **MediaPipe** repère ses 21 points (comme pour le drone), et un bras robot
à 4 servomoteurs suit tes mouvements :

| Ta main | Le bras |
|---|---|
| Gauche / droite | La **base** tourne |
| Haut / bas | L'**épaule** monte ou descend |
| Proche / loin de la caméra | Le **coude** se tend ou se plie |
| Pouce et index qui se pincent | La **pince** se ferme |

La page web (Chrome ou Edge sur ordinateur) envoie les angles à un **ESP32 par le câble USB** (Web Serial) :
pas de Wi-Fi à régler, presque aucun délai. Sans bras branché, un **dessin du bras** permet de tout tester.

## 1. Le matériel

| Pièce | Rôle | Prix indicatif |
|---|---|---|
| Bras en kit à **4 servos** (type « MeArm », acrylique ou bois) | La mécanique | 15 – 30 $ |
| 4 servos **MG90S** (engrenages métal) ou SG90 (souvent fournis avec le kit) | Les articulations | 8 – 15 $ |
| ESP32 DevKit (celui des autres projets) | Reçoit les angles, commande les servos | 5 – 10 $ |
| **Alimentation 5 V, 3 A** (bloc secteur avec fiche jack, ou chargeur USB puissant) | Alimente les servos | 5 – 8 $ |
| Condensateur **1000 µF 10 V** | Absorbe les pics de courant au démarrage des servos | < 1 $ |
| Plaque d'essai, fils, bornier | Montage | 3 $ |

## 2. ⚠️ Les règles qui évitent de tout griller

- **Les servos ne s'alimentent JAMAIS par la broche 5 V de l'ESP32.** Quatre servos peuvent demander
  plus de 2 A d'un coup : l'ESP32 redémarrerait sans arrêt (ou son régulateur chaufferait).
  Utilise l'**alimentation 5 V séparée**.
- **Masse commune obligatoire** : relie le GND de l'alimentation au GND de l'ESP32. Sans ça,
  les servos reçoivent des ordres incompréhensibles et tremblent.
- **Calibre AVANT de visser les bras sur les servos** (étape C). Un servo qui force contre une butée
  chauffe et casse ses engrenages.
- Garde les doigts loin de la pince pendant les essais.

## 3. Le schéma

```
  Alimentation 5 V (+) ──┬── fil rouge des 4 servos
                         ├── (+) condensateur 1000 µF
  Alimentation 5 V (−) ──┼── fil marron/noir des 4 servos
                         ├── (−) condensateur
                         └── GND de l'ESP32          (masse commune)

  Fils de signal (orange ou jaune) :
    base → GPIO13    épaule → GPIO12    coude → GPIO14    pince → GPIO27

  ESP32 ── câble USB ── ordinateur (Chrome ou Edge)
```

## 4. Les étapes

### Étape A : la page seule, sans rien acheter

```bash
cd bras-gestes
npm install      # copie aussi le moteur MediaPipe dans public/mediapipe
npm run dev      # http://localhost:5173 dans Chrome ou Edge
npm test         # 11 tests : angles, mouvement doux, message, limites identiques
```

Clique **« Activer la caméra »** et bouge la main : le dessin du bras la suit.

### Étape B : l'ESP32 et les servos (sans le bras monté)

1. Arduino IDE → *Gérer les bibliothèques* : installe **ESP32Servo**.
2. Ouvre `firmware/bras-gestes/bras-gestes.ino` et téléverse.
3. Branche les servos **sans leurs bras en plastique**, comme le schéma.
4. Ouvre le *Moniteur série* (115200 bauds, fin de ligne « Nouvelle ligne ») et tape :
   `A 90 90 90 60` puis `A 120 90 90 60` : le servo de la base doit tourner doucement.
   Essaie aussi `A 200 90 90 60` : l'ESP32 doit répondre **« Commande refusée »**.

### Étape C : calibrer

1. Avec la commande `R` (repos), tous les servos vont à 90° (et la pince à 60°).
2. **Visse maintenant les bras du kit** dans cette position, comme le montre la notice du kit
   (bras « au milieu »).
3. Pour chaque articulation, essaie des angles de plus en plus grands avec `A …` dans le Moniteur série.
   Note l'angle **juste avant** que la mécanique touche une butée : c'est ta limite.
4. Écris ces limites à **deux endroits** : `firmware/bras-gestes/limites.h` **et** `src/bras/reglages.ts`.
   Le test `npm test` vérifie que les deux sont identiques.
5. Téléverse à nouveau.

### Étape D : piloter avec la main

1. **Ferme le Moniteur série** de l'Arduino IDE (un seul programme peut utiliser le câble USB à la fois).
2. Sur la page, clique **« Connecter le bras (USB) »** et choisis la carte dans la liste.
3. Active la caméra : le vrai bras suit ta main.

**Sécurité :** <kbd>Espace</kbd> met en pause (le bras s'immobilise). Si ta main sort de l'image,
le bras se fige là où il est. S'il ne reçoit plus rien pendant 3 s, il reste immobile.

## 5. Comment c'est organisé

| Fichier | Rôle |
|---|---|
| `src/bras/angles.ts` | Main → 4 angles ; mouvement doux (lissage + vitesse maximale de 120 °/s) |
| `src/bras/reglages.ts` | Limites de chaque servo et position de repos |
| `src/bras/protocole.ts` | Le message envoyé par USB : `A base épaule coude pince` |
| `src/bras/useLienUsb.ts` | La connexion USB (Web Serial) |
| `src/bras/dessinerBras.ts` | Le dessin du bras à l'écran |
| `src/useCamera.ts` | Webcam + MediaPipe |
| `firmware/bras-gestes/protocole.h` | L'ESP32 vérifie chaque ligne et **refuse** tout angle hors limites |
| `firmware/bras-gestes/bras-gestes.ino` | Déplace les servos doucement (3° toutes les 15 ms au plus) |

Tests de l'ESP32, sur ordinateur :

```bash
g++ -std=c++17 -o test firmware/tests/test_protocole.cpp && ./test
```

## 6. Pour aller plus loin

- **Mémoriser une trajectoire** : enregistrer les angles pendant que tu bouges, puis la rejouer seule.
- **Un vrai geste pour attraper** : utiliser la reconnaissance du drone (poing fermé = saisir).
- **Sans fil** : remplacer l'USB par le Wi-Fi (WebSocket) pour piloter depuis un téléphone.
