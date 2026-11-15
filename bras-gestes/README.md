# Bras et main robotiques pilotés par la main

La webcam filme ta main, **MediaPipe** repère ses 21 points en 3D (comme pour le drone), et un bras
robotique **avec une main à 5 doigts** copie tes gestes :

| Ta main | Le robot |
|---|---|
| Gauche / droite | La **base** tourne |
| Haut / bas | L'**épaule** monte ou descend |
| Proche / loin de la caméra | Le **coude** se tend ou se plie |
| Tourner la main (comme une poignée de porte) | Le **poignet** tourne |
| Plier un doigt | Le **même doigt** du robot se plie |

**9 servomoteurs** en tout. La page web (Chrome ou Edge sur ordinateur) envoie les 9 angles à un
**ESP32 par le câble USB** (Web Serial), qui les transmet à une carte **PCA9685**.
Sans robot branché, un **dessin du bras et de la main** permet de tout tester.

## 1. Comment un doigt robotique se plie

Chaque doigt est fait de 3 phalanges articulées. Un **fil** (fil de pêche tressé) passe à l'intérieur,
comme un **tendon**, et s'enroule sur le palonnier d'un servo placé dans l'avant-bras : le servo tourne,
le fil tire, le doigt se plie. Un **élastique** sur le dessus du doigt le ramène tendu.
Pour le doigt, on mesure sur ta main la **somme des angles de ses articulations** :
environ 20° tendu (l'éventail naturel des doigts), environ 200° en poing fermé.

## 2. Le matériel

| Pièce | Rôle | Prix indicatif |
|---|---|---|
| **Main robotique à 5 doigts** : kit en métal ou en plastique avec 5 servos, ou main imprimée en 3D (modèle libre **InMoov**) | Les doigts et leurs tendons | 25 – 60 $ |
| 5 servos **MG90S** (si non fournis) | Un par doigt | 10 – 15 $ |
| Bras **assez solide pour porter la main** : servos **MG996R** (couple élevé) pour base, épaule et coude | Le bras | 20 – 40 $ |
| 1 servo MG90S ou MG996R pour le poignet | La rotation | 3 – 6 $ |
| Carte **PCA9685** (16 sorties servos, I2C) | Génère les signaux des 9 servos | 3 – 5 $ |
| ESP32 DevKit (celui des autres projets) | Reçoit les angles par USB, commande la PCA9685 | 5 – 10 $ |
| **Alimentation 6 V, 10 A** (ou 5 V, 10 A) | Alimente les servos | 10 – 20 $ |
| Condensateur **1000 µF 10 V** | Absorbe les pics de courant | < 1 $ |

**Conseil : commence par la main seule**, fixée sur un support (l'avant-bras posé sur la table).
Les doigts sont la partie la plus délicate (tension des fils, calibration). Le petit bras « MeArm »
en servos SG90 est **trop faible pour porter une main** : prévois des MG996R pour le bras.

## 3. ⚠️ Les règles qui évitent de tout griller

- **Les servos ne s'alimentent JAMAIS par l'ESP32.** 9 servos peuvent tirer plus de 5 A d'un coup.
  L'alimentation 6 V se branche sur le **bornier V+ / GND de la PCA9685**.
- **Masse commune obligatoire** entre l'alimentation, la PCA9685 et l'ESP32.
- **Calibre AVANT d'attacher les fils des doigts et les bras** (étape C). Un servo qui tire un doigt
  déjà fermé force, chauffe, et peut casser le fil ou ses engrenages.
- Garde les doigts loin de la main robot pendant les essais.

## 4. Le schéma

```
  ESP32           PCA9685
  3,3 V   ──────  VCC        (la logique de la carte)
  GND     ──────  GND
  GPIO21  ──────  SDA
  GPIO22  ──────  SCL

  Alimentation 6 V (+) ── V+ (bornier de la PCA9685) ── (+) condensateur 1000 µF
  Alimentation 6 V (−) ── GND (bornier)               ── (−) condensateur

  Sorties de la PCA9685 (une prise 3 broches par servo) :
    0 base   1 épaule   2 coude   3 poignet
    4 pouce  5 index    6 majeur  7 annulaire  8 auriculaire

  ESP32 ── câble USB ── ordinateur (Chrome ou Edge)
```

## 5. Les étapes

### Étape A : la page seule, sans rien acheter

```bash
cd bras-gestes
npm install      # copie aussi le moteur MediaPipe dans public/mediapipe
npm run dev      # http://localhost:5173 dans Chrome ou Edge
npm test         # 11 tests : doigts, poignet, bras, mouvement doux, message, limites identiques
```

Active la caméra et bouge la main : le dessin suit, doigt par doigt.

### Étape B : l'ESP32, la PCA9685 et un servo

1. Arduino IDE → *Gérer les bibliothèques* : installe **Adafruit PWM Servo Driver Library**.
2. Ouvre `firmware/bras-gestes/bras-gestes.ino` et téléverse.
3. Branche **un seul servo** sur la sortie 5 (index), **sans rien attaché dessus**.
4. *Moniteur série* (115200 bauds, fin de ligne « Nouvelle ligne ») :
   `A 90 90 90 90 20 20 20 20 20`, puis `A 90 90 90 90 20 120 20 20 20` : le servo de l'index tourne doucement.
   Essaie `A 90 90 90 90 20 200 20 20 20` : l'ESP32 doit répondre **« Commande refusée »**.

### Étape C : calibrer chaque doigt

1. Envoie `R` : tous les doigts vont en position « ouvert » (20°).
2. **Attache le fil du doigt** sur le palonnier dans cette position, fil juste tendu, doigt droit.
3. Augmente l'angle de ce doigt petit à petit (`A … 60 …`, `A … 100 …`) jusqu'à ce que le doigt soit
   **fermé sans forcer**. Note cet angle : c'est son **max**.
4. Si le doigt se plie quand l'angle **diminue**, ton servo est monté à l'envers : garde les angles,
   et mets `inverse: true` pour ce doigt dans `src/bras/reglages.ts`.
5. Fais pareil pour le poignet, puis pour le bras (base, épaule, coude), avant de visser leurs palonniers.
6. Écris les limites à **deux endroits** : `firmware/bras-gestes/limites.h` **et** `src/bras/reglages.ts`.
   Le test `npm test` vérifie qu'ils sont identiques. Téléverse à nouveau.

### Étape D : piloter avec ta main

1. **Ferme le Moniteur série** (un seul programme peut utiliser le câble USB à la fois).
2. Sur la page, **« Connecter le bras (USB) »**, choisis la carte, puis active la caméra.
3. Montre ta main **paume face à la caméra**, à environ 50 cm, bien éclairée.

**Sécurité :** <kbd>Espace</kbd> met en pause. Si ta main sort de l'image, tout se fige.
Sans commande depuis 3 s, le robot reste immobile.

**Si un doigt réagit mal**, ajuste les plages dans `PLAGES` (`src/bras/angles.ts`) : `pliageDoigt`
(tendu → fermé, en degrés), `pliagePouce`, `poignet`.

## 6. Comment c'est organisé

| Fichier | Rôle |
|---|---|
| `src/bras/angles.ts` | Main → 9 angles : position (bras), pliage de chaque doigt et rotation du poignet en 3D ; mouvement doux |
| `src/bras/reglages.ts` | Limites des 9 servos, position de repos, servos inversés |
| `src/bras/protocole.ts` | Le message USB : `A base épaule coude poignet pouce index majeur annulaire auriculaire` |
| `src/bras/useLienUsb.ts` | La connexion USB (Web Serial) |
| `src/bras/dessinerBras.ts` | Le dessin : bras de côté, main de face |
| `src/useCamera.ts` | Webcam + MediaPipe (points dans l'image et en 3D) |
| `firmware/bras-gestes/protocole.h` | L'ESP32 vérifie chaque ligne et **refuse** tout angle hors limites |
| `firmware/bras-gestes/bras-gestes.ino` | Déplace les 9 servos doucement via la PCA9685 |

Tests de l'ESP32, sur ordinateur :

```bash
g++ -std=c++17 -o test firmware/tests/test_protocole.cpp && ./test
```

## 7. Pour aller plus loin

- **Saisir un objet** : détecter le poing fermé et serrer jusqu'à ce que le courant du servo monte
  (capteur INA219, comme dans le suivi solaire) pour ne pas écraser l'objet.
- **Rejouer un geste** : enregistrer les 9 angles pendant que tu bouges, puis les rejouer.
- **Langue des signes** : faire « dire » des lettres à la main robot.
