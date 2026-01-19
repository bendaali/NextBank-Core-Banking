#ifndef INDEX_H
#define INDEX_H

#include <stdio.h>

// Taille de la table (ex: 1000 slots). 
// Plus c'est grand, moins il y a de collisions, plus c'est rapide.
#define HASH_SIZE 1024 

// Un noeud de l'index
typedef struct IndexNode {
    int id_compte;      // La Clé
    long file_offset;   // La Valeur : Position en octets dans le fichier .dat
    struct IndexNode *next; // Pour gérer les collisions
} IndexNode;

// Fonctions
void index_init();
void index_charger_depuis_disque(const char *filename);
long index_get_offset(int id_compte);
void index_ajouter(int id_compte, long offset);
void index_liberer();
int index_get_next_id();

#endif // INDEX_H