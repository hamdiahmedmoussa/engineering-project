/*
  LA SÉCURITÉ COMMUNE À TOUS LES BOÎTIERS WI-FI
  Fichier d'origine : commun/securite.h. Une copie identique est dans chaque dossier firmware/…
  (l'Arduino IDE ne sait pas inclure un fichier d'un autre dossier). Pour modifier : change
  commun/securite.h, puis lance outils/verifier-securite.sh qui recopie et vérifie.

  Ce qu'il fait :
  1. Refuse de COMPILER tant que les mots de passe sont ceux de l'exemple (publics sur GitHub)
     ou trop courts. On ne peut pas « oublier » de les changer.
  2. Demande un mot de passe pour chaque page (méthode « Digest » : le mot de passe ne circule
     jamais en clair sur le Wi-Fi, contrairement à la méthode « Basic »).
  3. Pour les commandes (ex. allumer la clim), exige un en-tête que seule notre page envoie :
     un autre site ouvert sur ton téléphone ne peut pas envoyer la commande à ta place (attaque « CSRF »).
*/
#pragma once
#include <stddef.h>

// --- Vérifications faites par le COMPILATEUR (aucun code n'est généré) ---
constexpr bool textesEgaux(const char* a, const char* b) {
  return *a == *b && (*a == '\0' || textesEgaux(a + 1, b + 1));
}
constexpr size_t longueurTexte(const char* s) { return *s ? 1 + longueurTexte(s + 1) : 0; }

#ifdef MOT_DE_PASSE_TABLEAU
static_assert(!textesEgaux(MOT_DE_PASSE_TABLEAU, "change-moi"),
              "Change MOT_DE_PASSE_TABLEAU dans config.h : celui de l'exemple est public.");
static_assert(longueurTexte(MOT_DE_PASSE_TABLEAU) >= 10, "MOT_DE_PASSE_TABLEAU : 10 caractères minimum.");
static_assert(!textesEgaux(RESEAU_BOITIER_MOT_DE_PASSE, "change-moi-2026"),
              "Change RESEAU_BOITIER_MOT_DE_PASSE dans config.h : celui de l'exemple est public.");
static_assert(longueurTexte(RESEAU_BOITIER_MOT_DE_PASSE) >= 8 && longueurTexte(RESEAU_BOITIER_MOT_DE_PASSE) <= 63,
              "RESEAU_BOITIER_MOT_DE_PASSE : entre 8 et 63 caractères (règle du Wi-Fi WPA2).");
#endif

#ifdef ARDUINO
#include <WebServer.h>

/** Vrai si le visiteur a donné le bon mot de passe. Sinon, le navigateur affiche la fenêtre de connexion. */
inline bool autorise(WebServer& serveur) {
  if (serveur.authenticate(UTILISATEUR_TABLEAU, MOT_DE_PASSE_TABLEAU)) return true;
  serveur.requestAuthentication(DIGEST_AUTH, "Boitier", "Mot de passe requis");
  return false;
}

/** En-tête que seule notre page envoie avec ses commandes (à demander au serveur dans setup()). */
const char* const EN_TETE_COMMANDE = "X-Tableau";

/** Pour une commande : mot de passe ET en-tête de notre page. */
inline bool commandeAutorisee(WebServer& serveur) {
  if (!autorise(serveur)) return false;
  if (serveur.header(EN_TETE_COMMANDE) == "1") return true;
  serveur.send(403, "text/plain", "Commande refusée : elle ne vient pas du tableau de bord.");
  return false;
}
#endif
