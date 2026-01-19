#ifndef MODELS_H
#define MODELS_H

#include <time.h>
#include <stdbool.h>

// --- CONSTANTES GLOBALES ---
#define MAX_COMPTES 10000
#define FICHIER_COMPTES "data/comptes.dat"
#define FICHIER_TRANSACTIONS "data/transactions.log"
#define FICHIER_INDEX "data/index.idx"

// --- STRUCTURES ---

typedef struct {
int id_compte;
char numero_compte[20];
char nom_titulaire[100];
char prenom_titulaire[100];
double solde;
char type_compte[20];
char statut[20];
char date_creation[20];
// Securite
char mot_de_passe_hash[256];
time_t derniere_connexion;
bool est_verrouille;
} Compte;

typedef struct {
    // Identification
    int id_transaction;
    int id_compte_source;
    int id_compte_destination;
    
    // Détails
    char type_operation[20];          // "Dépôt", "Retrait", "Transfert"
    double montant;
    double solde_avant;
    double solde_apres;
    
    // Traçabilité
    time_t date_transaction;
    char description[200];
    char statut[20];                  // "Réussie", "Échouée"
} Transaction;

#endif