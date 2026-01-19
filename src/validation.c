#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../include/validation.h"

// Règle 1 : L'argent doit être positif (pas de virement de -100€ pour voler de l'argent)
bool val_montant_positif(double montant) {
    // On refuse les montants négatifs ou nuls, et les montants absurdes (> 1 milliard)
    if (montant <= 0.01 || montant > 1000000000.0) {
        return false;
    }
    return true;
}

// Règle 2 : Règles de sécurité pour les noms et mots de passe
bool val_donnees_creation(const char* nom, const char* password, char* msg_erreur) {
    // Vérification du Nom
    if (nom == NULL || strlen(nom) < 2) {
        strcpy(msg_erreur, "Erreur: Le nom doit contenir au moins 2 caracteres.");
        return false;
    }
    
    // Vérification alphabétique (Pas de chiffres dans le nom)
    for (int i = 0; i < strlen(nom); i++) {
        if (!isalpha(nom[i]) && nom[i] != '-' && nom[i] != ' ') {
            strcpy(msg_erreur, "Erreur: Le nom contient des caracteres invalides.");
            return false;
        }
    }

    // Vérification du Mot de passe
    if (password == NULL || strlen(password) < 4) {
        strcpy(msg_erreur, "Erreur: Le mot de passe est trop court (min 4).");
        return false;
    }

    return true;
}

// Règle 3 : Pas de découvert autorisé
bool val_solde_suffisant(double solde_actuel, double montant_a_retirer) {
    if (solde_actuel >= montant_a_retirer) {
        return true;
    }
    return false;
}

// Règle 4 : ID valide
bool val_id_compte(int id) {
    return (id > 0);
}