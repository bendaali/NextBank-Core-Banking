#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#include "../include/security.h"
#include "../include/database.h"
#include "../include/models.h"

// Clé statique pour la démo (En prod : à charger depuis un fichier sécurisé ou variable d'env)
// 128 bits = 16 octets
static const unsigned char AES_KEY[16] = "NEXTBANK2026/KEY"; 
// IV statique pour la compatibilité simple
static const unsigned char AES_IV[16]  = "VecteurInit12345"; 

// --- GESTION DES ERREURS OPENSSL ---
void handle_openssl_error() {
    ERR_print_errors_fp(stderr);
    abort();
}

// =======================================================================
// PARTIE 1 : SHA-256 (Hachage Mots de passe & Fichiers)

void hacher_mot_de_passe(const char *password, char *hash_output) {
    EVP_MD_CTX *mdctx;
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;

    if((mdctx = EVP_MD_CTX_new()) == NULL) handle_openssl_error();

    // Initialisation SHA-256
    if(1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL)) handle_openssl_error();
    
    // Ajout des données
    if(1 != EVP_DigestUpdate(mdctx, password, strlen(password))) handle_openssl_error();
    
    // Finalisation
    if(1 != EVP_DigestFinal_ex(mdctx, hash, &hash_len)) handle_openssl_error();
    
    EVP_MD_CTX_free(mdctx);

    // Conversion Binaire -> Hexa String
    hash_output[0] = '\0';
    for(unsigned int i = 0; i < hash_len; i++) {
        char buf[3];
        sprintf(buf, "%02x", hash[i]);
        strcat(hash_output, buf);
    }
}

bool verifier_login(Compte *c, const char *password_clair) {
    char hash_calcul[SHA256_DIGEST_LENGTH * 2 + 1]; // *2 pour hexa + 1 pour \0
    hacher_mot_de_passe(password_clair, hash_calcul);
    return (strcmp(c->mot_de_passe_hash, hash_calcul) == 0);
}

// =======================================================================
// PARTIE 2 : AES-128 (Chiffrement Structure)


// Fonction générique pour Chiffrer OU Déchiffrer (AES-CTR est symétrique)
void aes_process_struct(void *data, size_t size) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;
    unsigned char *buffer_in = (unsigned char*)data;
    unsigned char *buffer_out = malloc(size); // Buffer temporaire

    if(!buffer_out) return;

    if(!(ctx = EVP_CIPHER_CTX_new())) handle_openssl_error();

    // Initialisation AES-128-CTR
    // Note : CTR chiffre et déchiffre avec la même fonction
    // 1 = Encrypt (mais en CTR, Encrypt == Decrypt logic)
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, AES_KEY, AES_IV))
        handle_openssl_error();

    // Traitement des données
    if(1 != EVP_EncryptUpdate(ctx, buffer_out, &len, buffer_in, size))
        handle_openssl_error();
    ciphertext_len = len;

    // Finalisation (Padding éventuel, mais absent en CTR)
    if(1 != EVP_EncryptFinal_ex(ctx, buffer_out + len, &len)) 
        handle_openssl_error();
    ciphertext_len += len;

    // Recopie du résultat dans la structure originale
    memcpy(data, buffer_out, size);

    EVP_CIPHER_CTX_free(ctx);
    free(buffer_out);
}

void chiffrer_struct(void *data, size_t size) {
    aes_process_struct(data, size);
}

void dechiffrer_struct(void *data, size_t size) {
    // En mode CTR, chiffrer et déchiffrer c'est la même opération mathématique
    aes_process_struct(data, size);
}

// =======================================================================
// PARTIE 3 : INTEGRITE FICHIER (SHA-256 complet)

unsigned long calculer_crc_via_sha256(const char* chemin) {
    FILE *f = fopen(chemin, "rb");
    if (!f) return 0;

    EVP_MD_CTX *mdctx;
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    unsigned char buffer[4096];
    size_t bytes;

    if((mdctx = EVP_MD_CTX_new()) == NULL) handle_openssl_error();
    if(1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL)) handle_openssl_error();

    while ((bytes = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        if(1 != EVP_DigestUpdate(mdctx, buffer, bytes)) handle_openssl_error();
    }

    if(1 != EVP_DigestFinal_ex(mdctx, hash, &hash_len)) handle_openssl_error();
    
    EVP_MD_CTX_free(mdctx);
    fclose(f);

    // On utilise les 8 premiers octets du hash SHA256 comme "Checksum" (simplification pour le stockage unsigned long)
    unsigned long res = 0;
    memcpy(&res, hash, sizeof(unsigned long)); 
    return res;
}

void security_mettre_a_jour_integrite() {
    unsigned long crc = calculer_crc_via_sha256("data/comptes.dat");
    FILE *f = fopen("data/checksum.chk", "wb");
    if (f) {
        fwrite(&crc, sizeof(unsigned long), 1, f);
        fclose(f);
    }
}

bool security_verifier_integrite_fichier() {

    // 1. comptes.dat DOIT exister
    FILE *fd = fopen("data/comptes.dat", "rb");
    if (!fd) {
        printf("[SECURITE] Fichier comptes.dat manquant !\n");
        return false;
    }
    fclose(fd);

    // 2. checksum doit exister
    FILE *f = fopen("data/checksum.chk", "rb");
    if (!f) {
        printf("[ALERTE OPENSSL] Fichier de checksum introuvable !\n");
        return false;
    }

    unsigned long stored_crc;
    fread(&stored_crc, sizeof(unsigned long), 1, f);
    fclose(f);

    // 3. calcul CRC
    unsigned long current_crc =
        calculer_crc_via_sha256("data/comptes.dat");

    if (stored_crc != current_crc) {
        printf("[ALERTE OPENSSL] comptes.dat corrompu !\n");
        return false;
    }

    return true;
}

// =======================================================================