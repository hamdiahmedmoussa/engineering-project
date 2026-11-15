# Sécurité des boîtiers ESP32

Les 4 boîtiers Wi-Fi (compteur d'énergie, suivi solaire, coupure clim, station météo) servent
une page web sur le réseau de la maison. Ce document dit **ce qui pourrait mal tourner**,
**ce qui est déjà protégé**, et **ce qu'il reste à faire**. Le bras robot passe par le câble USB :
il n'est pas exposé au réseau.

## 1. Les menaces (le « modèle de menace »)

| Qui | Ce qu'il pourrait faire | Gravité |
|---|---|---|
| Un invité ou un voisin connecté à ton Wi-Fi | Ouvrir les tableaux de bord, **allumer ou éteindre la clim** | Moyenne |
| Quelqu'un à proximité, si le boîtier a créé son propre réseau | Se connecter avec le mot de passe d'exemple, **public sur GitHub** | Élevée |
| Un site web malveillant ouvert sur ton téléphone | Envoyer en cachette « éteindre la clim » au boîtier (attaque **CSRF**) | Moyenne |
| Quelqu'un qui écoute le Wi-Fi | Lire les mesures qui passent en clair (HTTP) | Faible |
| Quelqu'un qui **vole le boîtier** | Lire la mémoire de l'ESP32 : mot de passe du Wi-Fi de la maison | Moyenne |

## 2. Ce qui est déjà protégé (`commun/securite.h`)

1. **Les mots de passe d'exemple sont interdits.** Si `MOT_DE_PASSE_TABLEAU` ou
   `RESEAU_BOITIER_MOT_DE_PASSE` sont ceux de l'exemple, ou trop courts, **le programme refuse de compiler**
   et l'Arduino IDE affiche par exemple : « Change MOT_DE_PASSE_TABLEAU dans config.h : celui de l'exemple est public. »
   On ne peut pas oublier.
2. **Chaque page demande un mot de passe**, avec la méthode **Digest** : le navigateur prouve qu'il
   connaît le mot de passe sans jamais l'envoyer en clair (la méthode « Basic », plus simple, l'enverrait
   presque en clair).
3. **Les commandes de la clim exigent un en-tête spécial** (`X-Tableau: 1`) que seule notre page envoie.
   Un autre site ne peut pas ajouter cet en-tête à une requête vers ton boîtier sans autorisation
   du boîtier (règle « CORS » des navigateurs) : l'attaque CSRF échoue.
4. **Les entrées sont vérifiées** : action inconnue refusée (clim), lignes USB invalides refusées (bras),
   trames de capteur abîmées ignorées (station météo).
5. **Pas de mise à jour à distance (OTA)** : personne ne peut remplacer le programme par le Wi-Fi.

## 3. Ce qu'il reste à faire (par ordre d'importance)

1. **Mettre les boîtiers sur un réseau séparé.** La plupart des box ont un **réseau invité** :
   connecte-y les boîtiers. Si l'un d'eux est piraté, l'attaquant n'atteint pas ton ordinateur.
2. **Choisir des mots de passe différents** pour chaque boîtier, et différents de ton Wi-Fi.
3. **Chiffrer la mémoire de l'ESP32** (*flash encryption*) et n'accepter que tes programmes
   (*secure boot*). Le vol du boîtier ne révélera alors plus ton mot de passe Wi-Fi.
   ⚠️ **Irréversible** : ces options « brûlent » des fusibles électroniques de la puce (*eFuses*).
   Une erreur peut rendre l'ESP32 inutilisable. À faire **sur une carte d'essai**, en suivant la
   documentation d'Espressif, quand tout le reste fonctionne.
4. **HTTPS** : chiffrer aussi les mesures. Possible sur ESP32, mais le navigateur affichera un avertissement
   (certificat fait maison). Utile surtout si les boîtiers sortent un jour de la maison.
5. **Limiter les essais de mot de passe** : bloquer quelques minutes après plusieurs erreurs.

## 4. Vérifier

```bash
bash outils/verifier-securite.sh   # recopie securite.h dans les 4 boîtiers et lance ses tests
```

Après avoir changé `commun/securite.h`, lance toujours ce script : il garde les 4 copies identiques.
