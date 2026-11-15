"""
LE DÉTECTEUR D'ARNAQUES : des règles qui EXPLIQUENT, et une petite IA qui APPREND.

1. Des RÈGLES repèrent les ruses classiques (demande de code secret, « argent envoyé par erreur »,
   urgence, gain trop beau, frais à payer d'avance, lien, faux agent). Chacune donne une raison lisible.
2. Un CLASSIFIEUR BAYÉSIEN NAÏF apprend, à partir d'exemples, quels mots reviennent plus souvent dans
   les arnaques. Pour un nouveau message, il calcule la probabilité que ce soit une arnaque.
3. Le VERDICT combine les deux. Un signal « fort » (code secret, erreur de transfert, frais d'avance)
   suffit à conclure : aucun opérateur ni aucune banque ne demande jamais ton code.
   L'IA seule ne suffit JAMAIS à accuser un message : avec peu d'exemples, elle apprend aussi des hasards
   (« mon frère » apparaît dans une arnaque → « Joyeux anniversaire mon frère » paraîtrait suspect).
   Il faut l'IA ET au moins une règle qui explique. Sinon, au plus « Méfiance ».

Aucune bibliothèque : tout est écrit ici, pour comprendre comment ça marche.
"""
from __future__ import annotations

import csv
import math
import re
import unicodedata
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

DONNEES = Path(__file__).with_name("donnees") / "messages.csv"
LONGUEUR_MAX = 2000  # un SMS ou un message WhatsApp ; au-delà, on refuse (protection du serveur)


def normaliser(texte: str) -> str:
    """Minuscules, sans accents, espaces simplifiés : « Félicitations ! » → « felicitations ! »."""
    sans_accents = unicodedata.normalize("NFKD", texte).encode("ascii", "ignore").decode()
    return re.sub(r"\s+", " ", sans_accents.lower()).strip()


# ---------- 1. Les règles ----------

@dataclass(frozen=True)
class Regle:
    nom: str
    motif: re.Pattern
    raison: str
    forte: bool = False  # un signal fort suffit pour conclure « arnaque probable »
    sauf: re.Pattern | None = None  # si ce motif est présent, la règle ne s'applique pas


def _r(motif: str) -> re.Pattern:
    return re.compile(motif)


REGLES = [
    # Un vrai SMS officiel CONTIENT un code et dit de ne pas le partager ; une arnaque le DEMANDE.
    Regle("code", _r(r"(envo\w*|donne\w*|communiqu\w*|transmet\w*|tape\w*|indique\w*|connect\w*).{0,40}"
                     r"\b(code|pin|otp|mot de passe)\b"),
          "Il demande un code secret ou un code reçu par SMS. Aucun opérateur ni aucune banque ne le demande jamais.", True,
          sauf=_r(r"\bne (le|la|les) \w+ (a|avec) personne|ne (le|la) (partagez|partage|communiquez|donnez|donne)")),
    Regle("erreur", _r(r"par erreur|trompe de numero|erreur de transfert"),
          "Le coup classique de « l'argent envoyé par erreur » : vérifie ton solde toi-même, ne renvoie rien.", True),
    Regle("frais", _r(r"(pay\w*|envo\w*|regl\w*).{0,40}\bfrais\b|\bfrais\b.{0,30}(dossier|livraison|ouverture|inscription|visa|participation)"
                      r"|payez? .{0,25}(pour|avant)"),
          "On te demande de payer d'abord pour recevoir un gain, un prêt ou un emploi.", True),
    Regle("gain", _r(r"felicitations|gagn\w*|tirage|tire au sort|loterie|\bprix\b|cadeau|bonus|recompense|gratuit"),
          "Il promet un gain ou un cadeau inattendu."),
    Regle("urgence", _r(r"urgent|immediatement|derniere chance|avant (minuit|ce soir|expiration)|dans \d+ ?(h|heures?|minutes?)\b"
                        r"|sera (bloque|suspendu|desactive)|vite\b"),
          "Il te presse, pour que tu agisses sans réfléchir.",
          sauf=_r(r"expire dans \d+")),  # « le code expire dans 5 minutes » : normal pour un vrai code
    Regle("menace", _r(r"bloque|suspen\w*|desactiv\w*|plainte|police"),
          "Il menace de bloquer un compte ou de porter plainte."),
    Regle("lien", _r(r"https?://|www\.|bit\.ly|tinyurl|\.(xyz|top|click)\b"),
          "Il contient un lien. Ne clique pas : va toi-même sur le site ou l'application officielle."),
    Regle("agent", _r(r"(service client|agent|conseiller)\b.{0,25}(d-?money|waafi|telecom|banque)|(d-?money|waafi)\b.{0,10}agent"),
          "Il se présente comme un agent officiel. Rappelle toi-même le numéro officiel pour vérifier."),
]


def signaux(texte: str) -> list[Regle]:
    t = normaliser(texte)
    return [r for r in REGLES if r.motif.search(t) and not (r.sauf and r.sauf.search(t))]


# ---------- 2. Le classifieur bayésien naïf ----------

# Les « mots vides » : très fréquents partout, ils ne disent rien sur l'arnaque. On les ignore.
MOTS_VIDES = set("""
le la les un une des de du au aux et ou a en sur par pour avec dans ce cet cette ces se sa son ses
mon ma mes ton ta tes votre vos notre nos leur leurs je tu il elle on nous vous ils elles me te lui
y est sont ai as avez avons ont etre que qui quoi ne pas plus si mais donc car ca stp merci bonjour
salut salam bonsoir cher chere
""".split())


def mots(texte: str) -> list[str]:
    """Découpe en mots, et remplace liens, montants et numéros par des « jetons » (le détail importe peu)."""
    t = normaliser(texte)
    t = re.sub(r"https?://\S+|www\.\S+|\S+\.(xyz|top|click)\S*|bit\.ly\S*|tinyurl\S*", " _lien_ ", t)
    t = re.sub(r"\d[\d ]*\s?(fdj|francs?|dollars?)", " _montant_ ", t)
    t = re.sub(r"\b(77|78)[ ]?\d{2}[ ]?\d{2}[ ]?\d{2}\b", " _numero_ ", t)
    return [m for m in re.findall(r"[a-z_]{2,}", t) if m not in MOTS_VIDES]


class Classifieur:
    """
    Bayes naïf : P(arnaque | mots) est proportionnel à P(arnaque) × produit des P(mot | arnaque).
    « Naïf » parce qu'on fait comme si les mots étaient indépendants : faux, mais ça marche très bien
    pour du texte court. On additionne des logarithmes (plus stable que multiplier de petits nombres).
    """

    def __init__(self) -> None:
        self.comptes: dict[str, Counter] = {"arnaque": Counter(), "normal": Counter()}
        self.messages = Counter()

    def apprendre(self, exemples: list[tuple[str, str]]) -> "Classifieur":
        for etiquette, texte in exemples:
            self.messages[etiquette] += 1
            self.comptes[etiquette].update(set(mots(texte)))  # présence du mot, pas le nombre de fois
        return self

    def probabilite_arnaque(self, texte: str) -> float:
        vocabulaire = len(set(self.comptes["arnaque"]) | set(self.comptes["normal"])) or 1
        total = sum(self.messages.values())
        scores = {}
        for classe in ("arnaque", "normal"):
            n = self.messages[classe]
            score = math.log((n + 1) / (total + 2))
            for m in set(mots(texte)):
                # Lissage de Laplace (+1) : un mot jamais vu ne met pas la probabilité à zéro.
                score += math.log((self.comptes[classe][m] + 1) / (n + 2))
            scores[classe] = score
        ecart = max(-50.0, min(50.0, scores["normal"] - scores["arnaque"]))
        return 1 / (1 + math.exp(ecart))  # retour aux probabilités (fonction logistique)

    def mots_suspects(self, texte: str, combien: int = 4) -> list[str]:
        """Les mots du message qui pèsent le plus vers « arnaque » (pour l'explication)."""
        def poids(m: str) -> float:
            a = (self.comptes["arnaque"][m] + 1) / (self.messages["arnaque"] + 2)
            n = (self.comptes["normal"][m] + 1) / (self.messages["normal"] + 2)
            return math.log(a / n)
        candidats = sorted({m for m in mots(texte) if not m.startswith("_")}, key=poids, reverse=True)
        return [m for m in candidats if poids(m) > 0.7][:combien]


def charger(fichier: Path = DONNEES) -> list[tuple[str, str]]:
    with fichier.open(encoding="utf-8") as f:
        return [(ligne["etiquette"], ligne["texte"]) for ligne in csv.DictReader(f, delimiter=";")]


# ---------- 3. Le verdict ----------

@dataclass
class Verdict:
    niveau: str           # "arnaque", "mefiance" ou "rien"
    titre: str
    probabilite: float
    raisons: list[str]
    mots: list[str]


def analyser(texte: str, modele: Classifieur) -> Verdict:
    if not texte.strip():
        raise ValueError("message vide")
    if len(texte) > LONGUEUR_MAX:
        raise ValueError("message trop long")
    trouves = signaux(texte)
    p = modele.probabilite_arnaque(texte)
    suspects = modele.mots_suspects(texte)
    fort = any(r.forte for r in trouves)
    # L'IA seule : seulement si elle est très sûre ET que plusieurs mots vont dans ce sens
    # (un seul mot, comme « numéro », ne suffit pas).
    ia_seule = p >= 0.97 and len(suspects) >= 2
    if fort or (p >= 0.8 and trouves):
        niveau, titre = "arnaque", "Arnaque probable"
    elif trouves or ia_seule:
        niveau, titre = "mefiance", "Méfiance"
    else:
        niveau, titre = "rien", "Rien de suspect"
    raisons = [r.raison for r in sorted(trouves, key=lambda r: not r.forte)]
    if niveau == "mefiance" and not trouves:
        raisons = ["Plusieurs mots ressemblent à ceux des arnaques connues, sans ruse précise repérée."]
    return Verdict(niveau, titre, round(p, 3), raisons, suspects if niveau != "rien" else [])
