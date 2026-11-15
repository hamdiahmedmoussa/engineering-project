#!/usr/bin/env bash
# Recopie commun/securite.h dans chaque boîtier Wi-Fi, puis lance ses tests.
# À lancer depuis la racine du dépôt :  bash outils/verifier-securite.sh
set -e
for dossier in compteur-energie/firmware/compteur-energie suivi-solaire/firmware/suivi-solaire \
               coupure-clim/firmware/coupure-clim station-meteo/firmware/station-meteo; do
  cp commun/securite.h "$dossier/securite.h"
done
g++ -std=c++17 -o /tmp/test_securite commun/test_securite.cpp && /tmp/test_securite
echo "securite.h recopié dans les 4 boîtiers."
