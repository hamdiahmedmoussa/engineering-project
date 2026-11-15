# Station météo et qualité de l'air

Une station pensée pour Djibouti : elle mesure la température, l'humidité, la pression et la
**poussière dans l'air**, et le dit en phrases, avec des conseils :
« Ressenti 43 °C : l'humidité ajoute 7 °C. **Danger** : coup de chaleur possible. Reste à l'ombre et bois beaucoup. »

ESP32 + **BME280** (température, humidité, pression) + **PMS5003** (particules PM1, PM2,5, PM10).
Même style de tableau de bord que les autres projets.

## 1. Ce que la station calcule

| Mesure | Ce qu'on en tire |
|---|---|
| Température + humidité | **Température ressentie** (indice de chaleur de la NOAA) et un niveau de danger, de « rien de particulier » à « danger extrême » |
| PM2,5 (particules très fines : fumées, pollution) | Niveau de qualité de l'air (seuils de l'EPA, 2024) |
| PM10 (poussière, sable) | Idem. On affiche le **pire** des deux niveaux, en précisant quand c'est surtout de la poussière |
| Pression | **Tendance sur 3 h** : une baisse rapide annonce souvent du vent (et de la poussière) |

Courbes des **24 dernières heures** : température et particules fines, un point toutes les 5 minutes.

## 2. Le matériel

| Pièce | Rôle | Prix indicatif |
|---|---|---|
| ESP32 DevKit | Mesure, calcule, sert la page web | 5 – 10 $ |
| Module **BME280** (bien « BME », pas « BMP » qui n'a pas l'humidité) | Température, humidité, pression | 3 – 6 $ |
| Capteur **PMS5003** + son câble | Particules dans l'air (avec un petit ventilateur) | 12 – 20 $ |
| Boîtier étanche ventilé + **abri** pour le BME280 (voir plus bas) | Protéger du soleil et de la pluie | 3 – 8 $ |
| Alimentation USB 5 V (au moins 1 A) | Le PMS5003 a besoin de 5 V | – |

## 3. Le schéma

```
  BME280 :   VIN → 3,3 V    GND → GND    SDA → GPIO21    SCL → GPIO22

  PMS5003 (câble à 8 fils, vérifie l'ordre sur la fiche de TON câble) :
    VCC (5 V) → broche 5V (VIN) de l'ESP32      GND → GND
    TX  → GPIO16 (l'ESP32 reçoit)               RX  → GPIO17
    SET → GPIO4  (HAUT = réveillé, BAS = endormi)
```

## 4. Bien placer la station (sinon les mesures sont fausses)

- **Le BME280 ne doit JAMAIS voir le soleil**, sinon il mesure la chaleur du soleil sur lui, pas l'air :
  10 °C de trop. Fabrique un **abri** avec 4 ou 5 assiettes en plastique blanches empilées et espacées
  (l'air passe, le soleil non), ou place la station à l'ombre, à 1,5 m du sol, loin d'un mur chaud.
- **Le BME280 doit être loin de l'ESP32** (une dizaine de centimètres) : l'ESP32 chauffe un peu.
- **Le PMS5003** aspire l'air : son entrée ne doit pas être bouchée, et doit être protégée de la pluie
  (ouverture vers le bas).

## 5. Les étapes

### Étape A : sans capteurs (simulation)

1. Arduino IDE → *Gérer les bibliothèques* : installe **Adafruit BME280 Library** (accepte d'installer aussi
   « Adafruit Unified Sensor »).
2. Ouvre `firmware/station-meteo/station-meteo.ino`, **copie `config.exemple.h` en `config.h`**, mets ton Wi-Fi.
3. Téléverse, ouvre l'adresse affichée par le *Moniteur série*.

### Étape B : avec les capteurs

1. Fais le montage, mets `MODE_SIMULATION false` dans `config.h`, téléverse.
2. Si le Moniteur série affiche « BME280 introuvable », essaie `ADRESSE_BME280 0x77`.
3. La première mesure de l'air arrive **au bout de 30 secondes** (le capteur se stabilise),
   puis toutes les 5 minutes.

**Pourquoi le capteur de particules dort ?** Son ventilateur et son laser s'usent (environ un an s'il
tourne sans arrêt). En ne le réveillant que 30 s toutes les 5 min, il dure **environ 10 fois plus longtemps**.

## 6. Les seuils utilisés

**Chaleur ressentie** (NOAA) : moins de 27 °C rien de particulier · 27 – 32 prudence · 32 – 41 grande prudence
· 41 – 54 danger · plus de 54 danger extrême.

**PM2,5** (µg/m³, EPA 2024) : 0 – 9 bon · 9 – 35,4 moyen · 35,5 – 55,4 mauvais pour les personnes sensibles
· 55,5 – 125,4 mauvais · 125,5 – 225,4 très mauvais · plus de 225,4 dangereux.

**PM10** (µg/m³) : 0 – 54 · 55 – 154 · 155 – 254 · 255 – 354 · 355 – 424 · plus de 424.

Les capteurs bon marché comme le PMS5003 donnent une **bonne idée** du niveau et de ses variations,
mais pas une mesure officielle : ils surestiment souvent quand l'air est très humide.

## 7. Organisation du code

| Fichier | Rôle |
|---|---|
| `firmware/station-meteo/station-meteo.ino` | Programme principal : capteurs, cycle du PMS5003, courbe des 24 h, serveur web |
| `firmware/station-meteo/meteo.h` | Température ressentie et niveaux (chaleur, PM2,5, PM10) |
| `firmware/station-meteo/pms.h` | Lecture d'une trame du PMS5003, avec vérification de la somme de contrôle |
| `firmware/station-meteo/page.h` | Le tableau de bord |
| `tests/test_meteo.cpp` | Tests : valeurs du tableau officiel de la NOAA, seuils, trames correctes et abîmées |

```bash
g++ -std=c++17 -o test tests/test_meteo.cpp && ./test
```

## 8. Pour aller plus loin

- **Partager tes mesures** sur la carte mondiale **sensor.community**, qui accepte les stations
  faites maison : Djibouti y a très peu de capteurs, ta station serait utile à tout le monde.
- **Prévenir sur le téléphone** quand la poussière dépasse un seuil (notification par Telegram, par exemple).
- **Comparer avec le suivi solaire** : un jour de poussière, combien le panneau perd-il ?
