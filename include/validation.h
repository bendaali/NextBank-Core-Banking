#ifndef VALIDATION_H
#define VALIDATION_H

#include <stdbool.h>
#include "../include/models.h"

// Valide si un montant est positif et réaliste
bool val_montant_positif(double montant);

// Valide les données pour la création d'un compte (Nom non vide, MDP assez long)
bool val_donnees_creation(const char* nom, const char* password, char* msg_erreur);

// Vérifie si le solde est suffisant pour un retrait/virement
bool val_solde_suffisant(double solde_actuel, double montant_a_retirer);

// Vérifie le format de l'ID (doit être > 0)
bool val_id_compte(int id);

#endif