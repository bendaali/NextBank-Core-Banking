#ifndef SECURITY_H
#define SECURITY_H

#include <stdbool.h>
#include <stddef.h>
#include "../include/models.h"

void hacher_mot_de_passe(const char *password, char *hash_output);
bool verifier_login(Compte *c, const char *password_clair);

void chiffrer_struct(void *data, size_t size);
void dechiffrer_struct(void *data, size_t size);

void security_mettre_a_jour_integrite();
bool security_verifier_integrite_fichier();

#endif