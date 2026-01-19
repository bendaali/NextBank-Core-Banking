#ifndef DATABASE_H
#define DATABASE_H
#include "../include/models.h"
// Initialisation de la base de données
void init_database();
// Opérations sur les comptes
int db_sauvegarder_compte(Compte *c);
int db_mise_a_jour_compte(Compte *c);
int db_lire_compte(int id, Compte *c_out);
// Opérations sur les transactions/logs
void db_logger_transaction(Transaction *t);
bool db_creer_backup();
void db_rotation_logs();
void db_afficher_tous_logs();
int db_restaurer_depuis_backup();
void db_lister_comptes(char *output_buffer, size_t buffer_size);
void db_lister_transactions(int id_compte, char *output_buffer, size_t buffer_size);
int db_rechercher_par_nom(const char *nom_cible, Compte *resultats_array, int max_results);
// Génération d'ID unique
int db_generer_id_unique();
// Vérification de l'existence d'un backup
int db_backup_existe();


#endif