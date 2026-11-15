# Détecteur d'arnaques par SMS et WhatsApp

Tu colles un message reçu ; le détecteur dit **« Arnaque probable »**, **« Méfiance »** ou **« Rien de suspect »**,
et surtout **pourquoi** : « Il demande un code secret. Aucun opérateur ni aucune banque ne le demande jamais. »

Python, **sans aucune bibliothèque**. Le message est analysé sur ton ordinateur : rien n'est envoyé sur
internet, rien n'est enregistré.

## 1. Comment il décide

**Des règles qui expliquent** (`REGLES` dans `detecteur.py`). Elles repèrent les ruses classiques :

| Ruse | Exemple | Signal |
|---|---|---|
| Demande de code secret | « Envoyez le code reçu par SMS » | **fort** |
| Argent « envoyé par erreur » | « Je t'ai envoyé 20 000 FDJ par erreur, renvoie-les » | **fort** |
| Frais à payer d'avance | « Payez 5 000 FDJ de frais pour recevoir votre prix » | **fort** |
| Gain inattendu | « Félicitations, vous avez gagné… » | |
| Urgence, menace | « Votre compte sera bloqué dans 24 h » | |
| Lien | « Cliquez sur www.… » | |
| Faux agent | « Ici le service client D-Money » | |

Chaque règle connaît aussi ses **exceptions** : un vrai SMS officiel *contient* un code et dit « ne le partagez
avec personne », alors qu'une arnaque le *demande*.

**Une petite IA qui apprend** : un **classifieur bayésien naïf**, écrit à la main. À partir d'exemples, il
apprend quels mots reviennent plus souvent dans les arnaques, et donne une probabilité.

**Le verdict** :
- **Arnaque probable** : un signal fort, ou l'IA sûre à 80 % **et** au moins une règle qui explique.
- **Méfiance** : au moins une règle, ou l'IA très sûre (97 %) avec plusieurs mots suspects.
- **Rien de suspect** sinon.

**L'IA seule ne suffit jamais à accuser un message.** Avec peu d'exemples, elle apprend aussi des hasards :
au début, « tu as le numéro du plombier ? » paraissait suspect à cause du seul mot « numéro ».

## 2. Lancer

```bash
python serveur.py        # http://127.0.0.1:8090
python evaluer.py        # mesure honnête de la performance (validation croisée)
python -m unittest discover -s tests -t .
```

## 3. Les résultats, honnêtement

Sur les 66 exemples (validation croisée en 5 paquets : chaque message est testé par un modèle qui ne l'a
jamais vu) : **30 arnaques repérées sur 30, 0 fausse alerte**, et 4 messages normaux reçoivent un simple
« Méfiance ».

⚠️ **Ces exemples ont été écrits pour l'apprentissage**, en s'inspirant des arnaques courantes. Il n'existe pas
de recueil public de SMS d'arnaque de Djibouti. Un score parfait sur des exemples écrits soi-même ne garantit
rien sur de vrais messages : les vrais escrocs varient leurs phrases, font des fautes, mélangent les langues.

## 4. Le rendre vraiment fiable

1. **Collecte de vrais messages**, avec l'accord des personnes : arnaques reçues par ta famille et tes amis,
   et autant de messages normaux.
2. **Anonymise toujours** : remplace les noms, numéros et montants réels avant de les ajouter
   (`Ahmed` → `un prénom`, `77 12 34 56` → `77 00 00 00`).
3. Ajoute-les dans `donnees/messages.csv` (`arnaque;texte` ou `normal;texte`), puis relance `python evaluer.py`.
4. Quand un message est mal classé, ne le corrige pas en ajoutant une règle sur mesure : demande-toi
   quelle **ruse générale** il utilise.
5. **Les langues** : beaucoup de messages mélangent français, somali, afar et arabe. Le classifieur apprend
   n'importe quelle langue si on lui donne des exemples ; les règles, elles, sont en français pour l'instant.

## 5. Organisation du code

| Fichier | Rôle |
|---|---|
| `detecteur.py` | Normalisation du texte, règles, classifieur bayésien, verdict |
| `donnees/messages.csv` | Les exemples d'apprentissage (écrits pour l'apprentissage, à compléter) |
| `evaluer.py` | Validation croisée : arnaques repérées, fausses alertes, erreurs détaillées |
| `serveur.py` + `page.html` | La page « Arnaque ou pas ? » (même style que les autres projets) |
| `tests/test_detecteur.py` | 10 tests : texte, règles et exceptions, classifieur, verdict, entrées invalides |

## 6. Pour aller plus loin

- **Un bot WhatsApp ou Telegram** : on lui transfère le message douteux, il répond le verdict.
- **Une application Android** qui analyse les SMS reçus, sur le téléphone, sans rien envoyer.
- **Partager les tendances** (sans données personnelles) avec les opérateurs ou une association de consommateurs.
