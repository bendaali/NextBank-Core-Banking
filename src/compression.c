#include <stdio.h>
#include <string.h>
#include <zlib.h> // L'include officiel de Zlib
#include "../include/compression.h"

#define CHUNK_SIZE 16384 // Lecture par blocs de 16KB

bool compresser_fichier_gzip(const char* fichier_source, const char* fichier_dest) {
    // 1. Ouvrir le fichier source en lecture binaire
    FILE *in = fopen(fichier_source, "rb");
    if (!in) {
        printf("[Erreur Zlib] Impossible d'ouvrir la source : %s\n", fichier_source);
        return false;
    }

    // 2. Ouvrir le fichier destination avec l'API Zlib (mode "wb" = write binary + compression par défaut)
    // "wb9" pour compression max, "wb1" pour rapide. "wb" est le défaut (6).
    gzFile out = gzopen(fichier_dest, "wb");
    if (!out) {
        printf("[Erreur Zlib] Impossible de creer l'archive : %s\n", fichier_dest);
        fclose(in);
        return false;
    }

    // 3. Boucle de lecture/compression
    char buffer[CHUNK_SIZE];
    int bytes_read = 0;
    int total_bytes = 0;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        // gzwrite compresse à la volée
        int bytes_written = gzwrite(out, buffer, bytes_read);
        
        if (bytes_written == 0) {
            printf("[Erreur Zlib] Echec lors de l'ecriture compressee.\n");
            fclose(in);
            gzclose(out);
            return false;
        }
        total_bytes += bytes_read;
    }

    // 4. Nettoyage
    fclose(in);
    gzclose(out); // Important : finalise le fichier .gz (CRC, footer)

    printf("[Zlib] Compression reussie : %s -> %s (%d bytes lus)\n", fichier_source, fichier_dest, total_bytes);
    return true;
}