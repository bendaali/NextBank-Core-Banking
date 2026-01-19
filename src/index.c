#include <stdio.h>
#include <stdlib.h>
#include "../include/index.h"
#include "../include/database.h" // Pour la struct Compte
#include "../include/security.h" // Pour dechiffrer_struct
static int g_max_id_connu = 0;
// La table de hachage globale (en RAM)
static IndexNode* hashTable[HASH_SIZE];

// Fonction de hachage simple
int hash_func(int id) {
    return id % HASH_SIZE;
}

void index_init() {
    for (int i = 0; i < HASH_SIZE; i++) {
        hashTable[i] = NULL;
    }
}

void index_ajouter(int id_compte, long offset) {
    int index = hash_func(id_compte);
    
    // Création du noeud
    IndexNode *newNode = (IndexNode*)malloc(sizeof(IndexNode));
    newNode->id_compte = id_compte;
    newNode->file_offset = offset;
    newNode->next = NULL;

    // Insertion en tête de liste (plus rapide)
    if (hashTable[index] == NULL) {
        hashTable[index] = newNode;
    } else {
        newNode->next = hashTable[index];
        hashTable[index] = newNode;
    }
    printf("[INDEX] Ajout ID %d -> Offset %ld\n", id_compte, offset);
    if (id_compte > g_max_id_connu) {
        g_max_id_connu = id_compte;
    }
}

int index_get_next_id() {
    return g_max_id_connu + 1;
}
// C'est la fonction magique pour retrouver l'offset d'un compte via son ID O(1)
long index_get_offset(int id_compte) {
    int index = hash_func(id_compte);
    IndexNode *current = hashTable[index];

    // On parcourt la liste chaînée à cet index
    while (current != NULL) {
        if (current->id_compte == id_compte) {
            return current->file_offset; // Trouvé !
        }
        current = current->next;
    }
    return -1; // Pas trouvé
}

// À appeler au lancement du serveur
void index_charger_depuis_disque(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return;

    Compte temp;
    long current_pos = 0;

    index_init(); // Reset

while (fread(&temp, sizeof(Compte), 1, fp)) {
    // 1. Sauvegarder la position AVANT lecture
    // (Note : ftell renvoie la pos APRES lecture, donc on recule ou on calcule)
    long current_pos = ftell(fp) - sizeof(Compte); 

    // 2. DÉCHIFFRER la structure
    dechiffrer_struct(&temp, sizeof(Compte));

    // 3. Ajouter à l'index avec l'ID propre
    index_ajouter(temp.id_compte, current_pos);
}
    
    printf("[INDEX] Base de donnees indexee en RAM avec succes.\n");
    fclose(fp);
}

void index_liberer() {
    for (int i = 0; i < HASH_SIZE; i++) {
        IndexNode *current = hashTable[i];
        while (current != NULL) {
            IndexNode *temp = current;
            current = current->next;
            free(temp);
        }
        hashTable[i] = NULL;
    }
}