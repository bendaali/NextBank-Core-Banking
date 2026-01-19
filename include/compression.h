#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <stdbool.h>

// Compresse un fichier source vers un fichier destination (format GZIP)
// Retourne true si succès
bool compresser_fichier_gzip(const char* fichier_source, const char* fichier_dest);

#endif