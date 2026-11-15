"""
ÉVALUER HONNÊTEMENT LE DÉTECTEUR : la validation croisée.

On coupe les exemples en 5 paquets. Pour chaque paquet : on apprend sur les 4 autres, et on teste sur
celui-ci, que le modèle n'a JAMAIS vu. Tester sur les messages d'apprentissage serait tricher.

    python evaluer.py
"""
import random

from detecteur import Classifieur, analyser, charger


def main(paquets: int = 5) -> None:
    exemples = charger()
    random.Random(42).shuffle(exemples)  # mélange identique à chaque fois : résultats reproductibles
    vrais_pos = faux_pos = vrais_neg = faux_neg = mefiance_normaux = 0
    erreurs = []
    for k in range(paquets):
        test = exemples[k::paquets]
        apprentissage = [e for i, e in enumerate(exemples) if i % paquets != k]
        modele = Classifieur().apprendre(apprentissage)
        for etiquette, texte in test:
            niveau = analyser(texte, modele).niveau
            alerte = niveau == "arnaque"
            mefiance_normaux += etiquette == "normal" and niveau == "mefiance"
            if etiquette == "arnaque":
                vrais_pos += alerte
                faux_neg += not alerte
            else:
                faux_pos += alerte
                vrais_neg += not alerte
            if alerte != (etiquette == "arnaque"):
                erreurs.append((etiquette, texte))

    total = len(exemples)
    print(f"{total} messages, validation croisée en {paquets} paquets\n")
    print(f"  Arnaques repérées      : {vrais_pos} sur {vrais_pos + faux_neg}")
    print(f"  Fausses alertes        : {faux_pos} sur {faux_pos + vrais_neg} messages normaux")
    print(f"  « Méfiance » sur normal : {mefiance_normaux} (un simple avertissement, pas une accusation)")
    print(f"  Exactitude globale     : {(vrais_pos + vrais_neg) / total:.0%}")
    for etiquette, texte in erreurs:
        print(f"  ✘ raté ({'arnaque non vue' if etiquette == 'arnaque' else 'fausse alerte'}) : {texte[:80]}")
    print("\n⚠️  Ces exemples ont été écrits pour l'apprentissage : ajoute de vrais messages (anonymisés)\n"
          "    dans donnees/messages.csv pour mesurer la performance réelle.")


if __name__ == "__main__":
    main()
