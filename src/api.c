#define CLIENT_EXPORTS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "../include/client.h"

// Variables globales de session (interne à la DLL)
static SOCKET sock_global = INVALID_SOCKET;
static int user_id_global = -1;
static CRITICAL_SECTION sock_cs;
static int api_initialized = 0;
static WSADATA global_wsa;

// Initialisation unique (WSA + CriticalSection)
static void api_init_once(void) {
    if (api_initialized) return;
    if (WSAStartup(MAKEWORD(2, 2), &global_wsa) != 0) {
        // WSAStartup failed; leave api_initialized 0 to indicate failure
        return;
    }
    InitializeCriticalSection(&sock_cs);
    api_initialized = 1;
}

// --- FONCTIONS EXPORTÉES ---

// Connect simple (single attempt)
CLIENT_API int Api_Connect(const char* ip, int port) {
    api_init_once();
    if (!api_initialized) return 0;

    EnterCriticalSection(&sock_cs);
    if (sock_global != INVALID_SOCKET) {
        // Already connected
        LeaveCriticalSection(&sock_cs);
        return 1;
    }

    sock_global = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_global == INVALID_SOCKET) {
        LeaveCriticalSection(&sock_cs);
        return 0;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons(port);

    int res = connect(sock_global, (struct sockaddr*)&server, sizeof(server));
    if (res < 0) {
        closesocket(sock_global);
        sock_global = INVALID_SOCKET;
        LeaveCriticalSection(&sock_cs);
        return 0; // Échec
    }

    LeaveCriticalSection(&sock_cs);
    return 1; // Succès
}

// Connect with retries
CLIENT_API int Api_TryConnect(const char* ip, int port, int maxRetries, int retryDelayMs) {
    int attempt;
    for (attempt = 0; attempt < maxRetries; ++attempt) {
        if (Api_Connect(ip, port)) return 1;
        Sleep(retryDelayMs);
    }
    return 0;
}

// Send a message; returns bytes sent or -1 on error
CLIENT_API int Api_Send(const char* message) {
    if (!api_initialized) return -1;
    EnterCriticalSection(&sock_cs);
    if (sock_global == INVALID_SOCKET) {
        LeaveCriticalSection(&sock_cs);
        return -1;
    }
    int sent = send(sock_global, message, (int)strlen(message), 0);
    LeaveCriticalSection(&sock_cs);
    return sent;
}

// Receive up to bufLen-1 bytes and NUL-terminate; returns bytes received or -1 on error
CLIENT_API int Api_Recv(char* buffer, int bufLen) {
    if (!api_initialized || bufLen <= 0) return -1;
    EnterCriticalSection(&sock_cs);
    if (sock_global == INVALID_SOCKET) {
        LeaveCriticalSection(&sock_cs);
        return -1;
    }
    int r = recv(sock_global, buffer, bufLen - 1, 0);
    if (r > 0) buffer[r] = '\0';
    LeaveCriticalSection(&sock_cs);
    return r;
}

// Download a file of expected_bytes bytes into local_filename; returns 1 success, 0 fail
CLIENT_API int Api_DownloadFile(const char* local_filename, long expected_bytes) {
    if (!api_initialized || local_filename == NULL) return 0;

    FILE* fp = fopen(local_filename, "wb");
    if (!fp) return 0;

    long total_received = 0;
    char buf[4096];

    while (total_received < expected_bytes) {
        int remaining = (int)(expected_bytes - total_received);
        int to_read = (remaining < (int)sizeof(buf)) ? remaining : (int)sizeof(buf);
        EnterCriticalSection(&sock_cs);
        if (sock_global == INVALID_SOCKET) {
            LeaveCriticalSection(&sock_cs);
            fclose(fp);
            return 0;
        }
        int recv_size = recv(sock_global, buf, to_read, 0);
        LeaveCriticalSection(&sock_cs);

        if (recv_size <= 0) {
            fclose(fp);
            return 0; // error or disconnect
        }
        fwrite(buf, 1, recv_size, fp);
        total_received += recv_size;
    }
    fclose(fp);
    return 1;
}

// Login helper; uses Api_Send/Api_Recv
CLIENT_API int Api_Login(int id, const char* password) {
    if (!api_initialized) return -1;
    char buffer[1024];
    int n = snprintf(buffer, sizeof(buffer), "LOGIN %d %s", id, password);
    if (n < 0) return -1;
    if (Api_Send(buffer) <= 0) return 0;

    int valread = Api_Recv(buffer, sizeof(buffer));
    if (valread > 0) {
        if (strstr(buffer, "Bienvenue") != NULL || strstr(buffer, "OK") != NULL) {
            user_id_global = id;
            return 1;
        }
    }
    return 0;
}
// Get logged in user ID
CLIENT_API int Api_GetLoggedInUserID(void) {
    return user_id_global;
}
// Check connection status
CLIENT_API int Api_IsConnected(void) {
    if (!api_initialized) return 0;
    EnterCriticalSection(&sock_cs);
    int connected = (sock_global != INVALID_SOCKET);
    LeaveCriticalSection(&sock_cs);
    return connected;
}
// Logout
CLIENT_API int Api_LOGOUT(void) {
    if (!api_initialized) return 0;
    char buffer[1024];
    int n = snprintf(buffer, sizeof(buffer), "LOGOUT");
    if (n < 0) return 0;
    if (Api_Send(buffer) <= 0) return 0;
    user_id_global = -1;
    return 1;
}
// Récupère le solde du compte connecté
CLIENT_API int Api_GetSolde(double* out_solde) {
    if (!api_initialized || out_solde == NULL) return 0;
    if (user_id_global == -1) return 0; // Not logged in

    char buffer[1024];
    int n = snprintf(buffer, sizeof(buffer), "SOLDE");
    if (n < 0) return 0;
    if (Api_Send(buffer) <= 0) return 0;

    int valread = Api_Recv(buffer, sizeof(buffer));
    if (valread > 0) {
        if (strncmp(buffer, "SOLDE ", 6) == 0) {
            double solde = atof(buffer + 6);
            *out_solde = solde;
            return 1;
        }
    }
    return 0;
}
// Récupère Nom, Prénom, Type et Date
CLIENT_API int Api_GetClientInfo(char* out_nom, char* out_prenom, char* out_type, char* out_date) {
    if (!Api_Send("INFO")) return 0;

    char buffer[256];
    if (Api_Recv(buffer, 256) <= 0) return 0;

    // Parsing des 4 variables
    if (sscanf(buffer, "INFO %s %s %s %s", out_nom, out_prenom, out_type, out_date) == 4) {
        return 1;
    }
    return 0;
}
CLIENT_API int Api_GetHistoryRaw(char* out_buffer, int buffer_size) {
    if (!api_initialized || out_buffer == NULL || buffer_size <= 0) return 0;
    if (user_id_global == -1) return 0; // Not logged in

    char command[64];
    int n = snprintf(command, sizeof(command), "HISTORY");
    if (n < 0) return 0;
    if (Api_Send(command) <= 0) return 0;

    int valread = Api_Recv(out_buffer, buffer_size);
    if (valread > 0) {
        return valread; // Return number of bytes read
    }
    return 0;

}

// Crée un compte et retourne l'ID généré dans *out_new_id
// Retourne 1 si succès, 0 si erreur
CLIENT_API int Api_CreerCompte(char* nom, char* prenom, char* pass, char* type, int* out_new_id,char* res) {
    char cmd[512];
    // Attention : pas d'espaces dans nom/prenom avec ce format simple
    sprintf(cmd, "CREER %s %s %s %s", nom, prenom, pass, type);

    if (!Api_Send(cmd)) return 0;

    char buffer[128];
    if (Api_Recv(buffer, 128) <= 0) return 0;

    // Le serveur répond "Succes ! Compte cree ID: 15"
    if (sscanf(buffer, "Succes ! Compte cree ID: %d\n", out_new_id) == 1) {
        sprintf(res, "Compte cree avec ID: %d", *out_new_id);
        return 1;
    }
    sprintf(res, buffer);
    return 0;
}
// Effectue une transaction (DEPOT, RETRAIT, VIREMENT)
CLIENT_API int Api_Transaction(const char* command, char* response, int response_size) {
    if (!api_initialized || command == NULL || response == NULL || response_size <= 0) return 0;
    if (user_id_global == -1) return 0; // Not logged in

    if (!Api_Send(command)) return 0;

    int valread = Api_Recv(response, response_size);
    if (valread > 0) {
        return valread; // Return number of bytes read
    }
    return 0;
}
// Helpers for specific transaction types : ici depot
CLIENT_API int Api_Transaction_Depot(double montant, char* response, int response_size) {
    char command[128];
    snprintf(command, sizeof(command), "DEPOT %.2f", montant);
    return Api_Transaction(command, response, response_size);
}
//Retrait
CLIENT_API int Api_Transaction_Retrait(double montant, char* response, int response_size) {
    char command[128];
    snprintf(command, sizeof(command), "RETRAIT %.2f", montant);
    return Api_Transaction(command, response, response_size);
}
//Virement
CLIENT_API int Api_Transaction_Virement(int id_dest, double montant, char* response, int response_size) {
    char command[128];
    snprintf(command, sizeof(command), "VIREMENT %d %.2f", id_dest, montant);
    return Api_Transaction(command, response, response_size);
}
// Déconnexion et nettoyage
CLIENT_API void Api_Disconnect(void) {
    if (!api_initialized) return;
    EnterCriticalSection(&sock_cs);
    if (sock_global != INVALID_SOCKET) {
        closesocket(sock_global);
        sock_global = INVALID_SOCKET;
    }
    LeaveCriticalSection(&sock_cs);
    // Cleanup du Socket et WSA
    DeleteCriticalSection(&sock_cs);
    WSACleanup();
    api_initialized = 0;
}
