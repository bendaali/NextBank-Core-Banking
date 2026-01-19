#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // Pour mkdir (Windows/Linux)
#include "../include/database.h"
#include "../include/security.h"
#include "../include/concurrency.h"
#include "../include/compression.h"
#include "../include/index.h"
#include <direct.h> // Pour _mkdir sur Windows
#include <errno.h>  // Pour gérer les codes d'erreur
#define FICHIER_COMPTES "data/comptes.dat"
#define FICHIER_TRANSACTIONS "data/transactions.log"
#define MAX_LOG_SIZE 100 * 1024 * 1024 // 100 MB
// Initialisation : Crée le dossier data et les fichiers vides s'ils n'existent pas
void init_database() {
    // Crée le dossier data (retourne -1 si existe déjà, ce qu'on ignore)
    _mkdir("data");

    // Crée le sous-dossier archive
    _mkdir("data/archives");
    // Crée le sous-dossier pdf
    _mkdir("data/pdf");

    FILE *fp = fopen(FICHIER_COMPTES, "rb");

    if (fp == NULL) {
        // --- CAS 1 : PREMIER LANCEMENT ---
        printf("[INFO] Base de donnees introuvable. Creation d'une nouvelle base vide...\n");
        //si backup existe, le restaurer
        if (db_backup_existe()) {
            printf("\n--- OPTION DE RECUPERATION ---\n");
            printf("Un fichier de sauvegarde (backup) a ete detecte.\n");
            printf("Voulez-vous ecraser les donnees corrompues par le backup ? (O/N) : ");
            
            char choix;
            char input_buffer[16];
            if (fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
                choix = input_buffer[0];
            } else {
                choix = 'N'; // Par défaut, refuser si erreur de lecture
            }

            if (choix == 'O' || choix == 'o') {
                printf("[Restoration] Copie du backup en cours...\n");
                
                if (db_restaurer_depuis_backup()) {
                    printf("[Succes] Donnees restaurees et signature de securite mise a jour.\n");
                    printf("[Info] Le serveur va demarrer normalement avec les donnees du backup.\n");
                    // On laisse le programme continuer vers le démarrage du socket
                } else {
                    printf("[Erreur] Echec de la restauration (disque plein ou droits d'acces ?).\n");
                    system("pause");
                }
            } else {
                printf("[Arret] Refus de restauration. Le serveur ne peut pas demarrer avec des donnees corrompues.\n");
                system("pause");
            }
        } else {
            // Pas de backup disponible
            // 1. Créer le fichier de comptes vide
        fp = fopen(FICHIER_COMPTES, "wb");
        if (!fp) {
            perror("[ERREUR] Impossible de creer le fichier DB");
        }
        fclose(fp);

        // 2. IMPORTANT : Créer le premier checksum (intégrité)
        // Cela va calculer le hash du fichier vide et créer 'comptes.integrity'
        security_mettre_a_jour_integrite(); 
        
        printf("[INFO] Base initialisee et integrite referencee.\n");
        }   
    } 
    
    // --- CAS 2 : LANCEMENT NORMAL ---
    fclose(fp);
    FILE *f = fopen(FICHIER_TRANSACTIONS, "ab");
    if (f) fclose(f);

    printf("[DB] Base de donnees initialisee.\n");
}

// Sauvegarder un compte (Append Only)
int db_sauvegarder_compte(Compte *c) {
    FILE *fp = fopen(FICHIER_COMPTES, "ab");
    if (!fp) return 0;

    // 1. Créer une COPIE temporaire pour ne pas corrompre la mémoire du programme
    Compte copy = *c;
    fseek(fp, 0, SEEK_END);
    long position = ftell(fp);
    index_ajouter(copy.id_compte, position); // Mise à jour RAM
    // 2. Chiffrer la copie
    chiffrer_struct(&copy, sizeof(Compte));

    // 3. Écrire la version cryptée
    fwrite(&copy, sizeof(Compte), 1, fp);
    fflush(fp);
    fclose(fp); 

    // 4. Mettre à jour l'intégrité après chaque modification
    security_mettre_a_jour_integrite();
    return 1;
}

int db_lire_compte(int id, Compte *c_out) {
    // 1. Interroger l'index (Opération immédiate en RAM)
    long offset = index_get_offset(id);

    if (offset == -1) {
        return 0; // Compte inexistant
    }

    FILE *fp = fopen("data/comptes.dat", "rb");
    if (!fp) return 0;

    // 2. Saut direct au bon endroit sur le disque
    fseek(fp, offset, SEEK_SET);

    // 3. Lecture d'un seul bloc
    fread(c_out, sizeof(Compte), 1, fp);
    fclose(fp);

    // 4. Déchiffrement
    dechiffrer_struct(c_out, sizeof(Compte));

    // Double sécurité : on vérifie que l'ID correspond bien
    if (c_out->id_compte == id) {
        return 1;
    }
    return 0;
}

// Fonction pour générer un ID unique et incrémental
int db_generer_id_unique() {
    int next_id = 1; // Valeur par défaut si le fichier n'existe pas
    verrouiller_generateur_id();
    FILE *f = fopen("data/seq.dat", "rb+");
    if (!f) {
        f = fopen("data/seq.dat", "wb");
        if (!f) {
            deverrouiller_generateur_id(); // Toujours déverrouiller en cas d'erreur !
            return -1;
        }
        fwrite(&next_id, sizeof(int), 1, f);
    } else {
        fread(&next_id, sizeof(int), 1, f);
        int new_val = next_id + 1;
        fseek(f, 0, SEEK_SET);
        fwrite(&new_val, sizeof(int), 1, f);
    }
    
    fflush(f);
    fclose(f);

    // 2. On libère le générateur
    deverrouiller_generateur_id();
    return next_id; // On retourne l'ID qui était dispo
}
// Rotation des logs si la taille dépasse MAX_LOG_SIZE
// src/database.c

void db_rotation_logs() {
    FILE *f = fopen(FICHIER_TRANSACTIONS, "rb");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long taille = ftell(f);
    fclose(f);

    // Seuil de déclenchement (ex: 100 Mo, ici mis bas pour tester)
    if (taille >= MAX_LOG_SIZE) { 
        printf("[Maintenance] Rotation des logs declenchee...\n");
        
        char temp_name[100];
        char archive_name[100];
        time_t now = time(NULL);
        
        // Nom temporaire
        sprintf(temp_name, "data/transactions_temp.log");
        // Nom final compressé : transactions_170123456.log.gz
        sprintf(archive_name, "data/archives/transactions_%ld.log.gz", now);

        // 1. On renomme le fichier actuel pour libérer "transactions.log"
        // Le serveur peut recréer un nouveau transactions.log vide immédiatement après.
        rename(FICHIER_TRANSACTIONS, temp_name);
        
        // 2. Compression
        // Note: Idéalement, ceci devrait être fait dans un thread séparé pour ne pas bloquer le serveur
        if (compresser_fichier_gzip(temp_name, archive_name)) {
            // Si compression OK, on supprime le fichier temporaire non compressé
            remove(temp_name);
            printf("[Maintenance] Log archive et compresse : %s\n", archive_name);
        } else {
            printf("[Erreur] Echec compression logs.\n");
        }
    }
}
// Logger une transaction (Append Only)
void db_logger_transaction(Transaction *t) {
    // Vérifier si une rotation des logs est nécessaire
    db_rotation_logs();
    FILE *fp = fopen(FICHIER_TRANSACTIONS, "ab");
    if (!fp) return;
    chiffrer_struct(t, sizeof(Transaction));
    fwrite(t, sizeof(Transaction), 1, fp);
    fflush(fp); // Sécurité anti-crash
    fclose(fp);
}
// Afficher tous les logs (Pour debug/admin)
void db_afficher_tous_logs() {
    FILE *fp = fopen(FICHIER_TRANSACTIONS, "rb");
    Transaction t;
    printf("\n--- LOGS DES TRANSACTIONS ---\n");
    while(fread(&t, sizeof(Transaction), 1, fp)) {
        dechiffrer_struct(&t, sizeof(Transaction));
        printf("[%s] Type: %s | Montant: %.2f | De: %d -> A: %d | Statut: %s\n", 
               ctime(&t.date_transaction), t.type_operation, t.montant, 
               t.id_compte_source, t.id_compte_destination, t.statut);
    }
    fclose(fp);
}

// Rechercher des comptes par nom (retourne le nombre de résultats trouvés)
int db_rechercher_par_nom(const char *nom_cible, Compte *resultats_array, int max_results) {
    FILE *fp = fopen(FICHIER_COMPTES, "rb");
    if (!fp) return 0;
    Compte temp;
    int count = 0;
    while(fread(&temp, sizeof(Compte), 1, fp) && count < max_results) {
        dechiffrer_struct(&temp, sizeof(Compte)); // AES Decrypt
        if (strcmp(temp.nom_titulaire, nom_cible) == 0) {
            resultats_array[count++] = temp;
        }
    }
    fclose(fp);
    return count;
}

// Crée une copie de sauvegarde complète du fichier comptes.dat
bool db_creer_backup() {
    printf("[Maintenance] Creation du backup...\n");
    bool test = false;
    // 1. Verrouillage global (Personne ne bouge pendant la copie !)
    verrouiller_global();
    security_verifier_integrite_fichier(); // Juste pour s'assurer que tout est OK avant la copie
    FILE *src = fopen(FICHIER_COMPTES, "rb");
    FILE *dest = fopen("data/comptes_backup.dat", "wb");

    if (src && dest) {
        char buffer[4096];
        size_t bytes;
        
        // Copie par blocs de 4KB (très rapide)
        while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
            fwrite(buffer, 1, bytes, dest);
        }
        
        printf("[Maintenance] Backup termine : data/comptes_backup.dat\n");
        test = true;
    }

    if (src) fclose(src);
    if (dest) fclose(dest);
    
    deverrouiller_global();
    return test;
}

// Tente de restaurer depuis le backup
// Retourne 1 si succès, 0 si échec (ex: pas de backup)
int db_restaurer_depuis_backup() {
    FILE *src = fopen("data/comptes_backup.dat", "rb");
    if (!src) return 0; // Le fichier backup n'existe pas

    FILE *dest = fopen(FICHIER_COMPTES, "wb");
    if (!dest) {
        fclose(src);
        return 0; // Impossible d'écrire sur le disque
    }

    // Copie brute (byte-to-byte)
    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dest);
    }

    fclose(src);
    fclose(dest);

    // CRUCIAL : Maintenant que le fichier est remplacé par une version saine,
    // on doit recalculer sa signature SHA-256 pour que le système de sécurité l'accepte.
    security_mettre_a_jour_integrite();

    return 1;
}

// Vérifie juste si le fichier backup existe
int db_backup_existe() {
    FILE *f = fopen("data/comptes_backup.dat", "rb");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

// Mise à jour d'un compte existant
int db_mise_a_jour_compte(Compte *c) {
    int result = 0;

    // Utiliser l'index pour trouver l'offset directement
    long offset = index_get_offset(c->id_compte);
    if (offset == -1) return 0; // Compte inexistant

    // Verrouiller globalement pendant l'opération d'écriture
    verrouiller_global();

    FILE *fp = fopen(FICHIER_COMPTES, "rb+");
    if (!fp) {
        deverrouiller_global();
        return 0;
    }

    // Aller directement à l'offset connu
    if (fseek(fp, offset, SEEK_SET) != 0) {
        fclose(fp);
        deverrouiller_global();
        return 0;
    }

    // Lire la structure existante pour vérifier l'ID
    Compte stored;
    if (fread(&stored, sizeof(Compte), 1, fp) != 1) {
        fclose(fp);
        deverrouiller_global();
        return 0;
    }

    // Déchiffrer pour valider
    dechiffrer_struct(&stored, sizeof(Compte));
    if (stored.id_compte != c->id_compte) {
        // Incohérence : l'index semble incorrect
        fclose(fp);
        deverrouiller_global();
        return 0;
    }

    // Revenir se positionner et écrire la nouvelle version chiffrée
    if (fseek(fp, offset, SEEK_SET) != 0) {
        fclose(fp);
        deverrouiller_global();
        return 0;
    }

    Compte copy = *c;
    chiffrer_struct(&copy, sizeof(Compte));

    if (fwrite(&copy, sizeof(Compte), 1, fp) == 1) {
        fflush(fp);
        result = 1;
    }

    fclose(fp);

    if (result) {
        security_mettre_a_jour_integrite();
    }

    deverrouiller_global();
    return result;
}

//lister tous les comptes (pour admin)
void db_lister_comptes(char *output_buffer, size_t buffer_size) {
    FILE *fp = fopen(FICHIER_COMPTES, "rb");
    if (!fp) {
        snprintf(output_buffer, buffer_size, "Erreur: Impossible d'ouvrir la base de donnees.\n");
        return;
    }

    Compte temp;
    char line[256];
    while(fread(&temp, sizeof(Compte), 1, fp)) {
        dechiffrer_struct(&temp, sizeof(Compte)); // AES Decrypt
        snprintf(line, sizeof(line), "ID: %d | Nom: %s | Solde: %.2f | Type: %s | Statut: %s\n",
                 temp.id_compte, temp.nom_titulaire, temp.solde, temp.type_compte, temp.statut);
        strncat(output_buffer, line, buffer_size - strlen(output_buffer) - 1);
    }
    fclose(fp);
}
//Raw Transaction history listing (for user)
void db_lister_transactions(int id_compte, char *output_buffer, size_t buffer_size) {
    FILE *fp = fopen(FICHIER_TRANSACTIONS, "rb");
    if (!fp) {
        snprintf(output_buffer, buffer_size, "Erreur: Impossible d'ouvrir le fichier de logs.\n");
        return;
    }

    Transaction t;
    char line[512];
    char date_str[30];

    while(fread(&t, sizeof(Transaction), 1, fp)) {
        dechiffrer_struct(&t, sizeof(Transaction));
        bool est_source = (t.id_compte_source == id_compte);
        bool est_dest   = (t.id_compte_destination == id_compte);

        if (!(est_source || est_dest)) continue;

        // Format date like dd/mm/YYYY HH:MM
        strftime(date_str, sizeof(date_str), "%d/%m/%Y %H:%M", localtime(&t.date_transaction));

        // Prepare debit/credit and solde columns similar to PDF report
        char deb[64] = "-";
        char cred[64] = "-";
        char sold[64] = "-";
        double solde_resultant = 0.0;

        if (strcmp(t.type_operation, "TRANSFERT") == 0) {
            if (est_source) {
                snprintf(deb, sizeof(deb), "%.2f", t.montant);
                solde_resultant = t.solde_apres; // this line stores sender's solde after
            } else {
                snprintf(cred, sizeof(cred), "%.2f", t.montant);
                // destination line may not have solde_apres, leave sold as '-'
            }
        } else if (strcmp(t.type_operation, "RETRAIT") == 0) {
            snprintf(deb, sizeof(deb), "%.2f", t.montant);
            solde_resultant = t.solde_apres;
        } else { // DEPOT and others treated as credit
            snprintf(cred, sizeof(cred), "%.2f", t.montant);
            solde_resultant = t.solde_apres;
        }

        if (solde_resultant > 0.001) {
            snprintf(sold, sizeof(sold), "%.2f", solde_resultant);
        }

        // Build the output line
        snprintf(line, sizeof(line), "%s | REF:#%d | TYPE:%s | DEBIT:%6s | CREDIT:%6s | SOLDE:%6s | STATUT:%s | %s\n",
                 date_str, t.id_transaction, t.type_operation, deb, cred, sold, t.statut, t.description);

        strncat(output_buffer, line, buffer_size - strlen(output_buffer) - 1);
    }
    fclose(fp);
}
