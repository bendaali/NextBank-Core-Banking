#ifndef CONCURRENCY_H
#define CONCURRENCY_H
// Initialisation des verrous
void init_verrous();
// Verrou global pour les opérations affectant plusieurs comptes
void verrouiller_global();
void deverrouiller_global();
// Verrou pour les opérations sur un compte spécifique
void verrouiller_compte(int id_compte);
void deverrouiller_compte(int id_compte);
// Verrou pour le générateur d'ID
void verrouiller_generateur_id();
void deverrouiller_generateur_id();
#endif