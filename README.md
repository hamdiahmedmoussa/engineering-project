# engineering-project

Mes projets d'ingénierie (Électricité & Énergie, électronique, vision par ordinateur).

| Projet | Description |
|---|---|
| [`detecteur-arnaques/`](detecteur-arnaques/) | Détecteur d'arnaques par SMS et WhatsApp : règles qui expliquent + classifieur bayésien écrit à la main, page « Arnaque ou pas ? ». Python sans bibliothèque. |
| [`drone-gestes/`](drone-gestes/) | Drone piloté par les gestes de la main : webcam + MediaPipe (21 points de la main) → geste → drone. Pour l'instant un simulateur 3D ; ensuite un vrai drone DJI Tello. |
| [`bras-gestes/`](bras-gestes/) | Bras et main robotiques à 5 doigts (9 servos) qui copient la main devant la webcam (MediaPipe 3D), commandés par USB depuis Chrome. |
| [`compteur-energie/`](compteur-energie/) | Compteur d'énergie connecté : ESP32 + pince ampèremétrique, consommation en W, kWh et FDJ sur le téléphone. |
| [`coupure-clim/`](coupure-clim/) | Coupure intelligente de climatiseur : pilotage en infrarouge, radar de présence, mode éco, alerte de surchauffe. |
| [`labo-ot/`](labo-ot/) | Laboratoire de sécurité industrielle : poste électrique simulé en Modbus, démonstration d'attaque (labo uniquement), pare-feu Modbus qui lit chaque message, supervision web. Python sans bibliothèque. |
| [`station-meteo/`](station-meteo/) | Station météo et qualité de l'air : température ressentie, poussière (PM2,5 et PM10) avec conseils, tendance de la pression. ESP32 + BME280 + PMS5003. |
| [`suivi-solaire/`](suivi-solaire/) | Suivi d'un panneau solaire : production réelle, perte due à la chaleur, effet de la poussière. ESP32 + INA219 + DS18B20. |

## Sécurité

Les boîtiers Wi-Fi sont protégés par mot de passe et refusent de compiler avec les mots de passe d'exemple.
Menaces, protections et prochaines étapes : [`SECURITE.md`](SECURITE.md).

## Prochains projets (dans l'ordre)

1. ~~Compteur d'énergie connecté~~ (fait)
2. ~~Suivi d'un panneau solaire~~ (fait)
3. ~~Coupure intelligente de climatiseur~~ (fait)
4. ~~Bras robot piloté par les gestes~~ (fait)
5. ~~Station météo et qualité de l'air~~ (fait)
