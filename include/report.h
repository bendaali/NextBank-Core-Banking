#ifndef REPORT_H
#define REPORT_H

#include <stdbool.h>

// Génère une chaîne de caractères contenant le bilan (Total dépôts, retraits, nombre d'opérations)
// out_buffer doit être assez grand (ex: 1024 char)
void report_generer_statistiques(int id_compte, char *out_buffer);

// Génère un fichier PDF physique "releve_ID.pdf" dans le dossier data/pdf
bool report_generer_pdf(int id_compte, const char *nom_titulaire, double solde_actuel,char *out_buffer);

#endif