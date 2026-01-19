#include <windows.h>
#include <stdio.h>
#include "../include/concurrency.h"

//on utilise un gros verrou global pour la BDD
CRITICAL_SECTION global_db_lock;
//on utilise un verrou spécifique pour le générateur d'ID
CRITICAL_SECTION id_seq_lock;

void init_verrous() {
    InitializeCriticalSection(&global_db_lock);
    InitializeCriticalSection(&id_seq_lock);
    // Création du fichier verrous.lock vide
    FILE *f = fopen("data/verrous.lock", "rb");
    if (!f) {
        f = fopen("data/verrous.lock", "wb");
    }
    if(f) fclose(f);
}
void verrouiller_global() {
    // 1. Verrouillage mémoire (rapide)
    EnterCriticalSection(&global_db_lock);
    // 2. Inscription dans verrous.lock pour audit externe
    FILE *f = fopen("data/verrous.lock", "ab");
    if(f) { 
        fprintf(f, "GLOBAL LOCKED\n");
        fclose(f);
    }
}
void deverrouiller_global() {
    // 1. Déverrouillage mémoire
    LeaveCriticalSection(&global_db_lock);
    // 2. Mise à jour fichier
    FILE *f = fopen("data/verrous.lock", "ab");
    if(f) {
        fprintf(f, "GLOBAL UNLOCKED\n");
        fclose(f);
    }
}

void verrouiller_compte(int id_compte) {
    // 1. Verrouillage mémoire (rapide)
    EnterCriticalSection(&global_db_lock);
    
    // 2. Inscription dans verrous.lock pour audit externe
    FILE *f = fopen("data/verrous.lock", "ab");
    if(f) {
        fprintf(f, "LOCKED: ID %d\n", id_compte);
        fclose(f);
    }
}

void deverrouiller_compte(int id_compte) {
    // 1. Mise à jour fichier
    FILE *f = fopen("data/verrous.lock", "ab");
    if(f) {
        fprintf(f, "UNLOCKED: ID %d\n", id_compte);
        fclose(f);
    }
    
    // 2. Déverrouillage mémoire
    LeaveCriticalSection(&global_db_lock);
}

void verrouiller_generateur_id() {
    EnterCriticalSection(&id_seq_lock);
}

void deverrouiller_generateur_id() {
    LeaveCriticalSection(&id_seq_lock);
}