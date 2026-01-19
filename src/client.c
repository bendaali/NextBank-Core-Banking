#include <stdio.h>
#include <winsock2.h>
#include <windows.h> // Pour Sleep()
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define MAX_RETRIES 5        // Nombre max d'essais avant abandon
#define RETRY_DELAY_MS 3000  // Attente de 3 secondes entre chaque essai

// Fonction pour gérer la tentative de connexion initiale
int tenter_connexion(SOCKET *s, struct sockaddr_in *server) {
    int tentative = 0;
    
    while (tentative < MAX_RETRIES) {
        // Création du socket (il faut le recréer à chaque tentative échouée)
        *s = socket(AF_INET, SOCK_STREAM, 0);
        if (*s == INVALID_SOCKET) {
            printf("Erreur critique creation socket : %d\n", WSAGetLastError());
            return 0; // Echec total
        }

        printf("Tentative de connexion au serveur (%d/%d)...\n", tentative + 1, MAX_RETRIES);

        // Tentative de connect
        if (connect(*s, (struct sockaddr *)server, sizeof(*server)) < 0) {
            printf("Serveur injoignable. Nouvelle tentative dans %d sec...\n", RETRY_DELAY_MS/1000);
            closesocket(*s); // Important : fermer le socket raté
            Sleep(RETRY_DELAY_MS); // Attendre (Windows Sleep est en ms)
            tentative++;
        } else {
            printf(">>> CONNEXION REUSSIE !\n");
            return 1; // Succès
        }
    }

    printf("Echec : Impossible de joindre le serveur apres %d tentatives.\n", MAX_RETRIES);
    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in server;
    char message[1000], server_reply[2000];
    int recv_size;

    // 1. Init Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Echec initialisation Winsock.\n");
        return 1;
    }

    // Configuration de l'adresse serveur
    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    // --- BOUCLE GLOBALE (Gère la reconnexion) ---
    while (1) {
        
        // 2. Tenter de se connecter
        if (!tenter_connexion(&s, &server)) {
            // Si toutes les tentatives échouent, on quitte le programme
            break;
        }

        // --- BOUCLE DE SESSION (L'utilisateur est connecté) ---
        while(1) {
            printf("\nVotre commande (ou 'exit' pour quitter) : ");
            
            // Lecture propre (évite les bugs de scanf avec les espaces)
            memset(message, 0, 1000); // Nettoyage buffer
            fgets(message, 1000, stdin);
            message[strcspn(message, "\n")] = 0; // Enlever le \n à la fin

            if (strcmp(message, "exit") == 0) {
                closesocket(s);
                WSACleanup();
                return 0;
            }

            // Envoi
            if(send(s, message, strlen(message), 0) < 0) {
                printf("Erreur d'envoi. Connexion probablement perdue.\n");
                break; // Sort de la boucle session -> Retour boucle globale
            }

            printf("En attente de reponse...\n");

            // Réception
            recv_size = recv(s, server_reply, 2000, 0);
            
            if(recv_size == SOCKET_ERROR || recv_size == 0) {
                printf("Serveur deconnecte ou erreur de reception.\n");
                break; // Sort de la boucle session -> Retour boucle globale
            }
            
            server_reply[recv_size] = '\0';
                    if (strncmp(server_reply, "DOWNLOAD", 8) == 0) {
            long filesize = 0;
            sscanf(server_reply, "DOWNLOAD %ld", &filesize);
            
            printf("[INFO] Reception d'un fichier PDF (%ld octets)...\n", filesize);

            // Création du fichier côté Client
            FILE *fp = fopen("releve_nextbank.pdf", "wb"); // wb = Write Binary
            if (!fp) {
                printf("Erreur: Impossible de creer le fichier localement.\n");
            } else {
                long total_received = 0;
                char file_chunk[4096];
                int chunk_size;

                // Boucle de réception tant qu'on n'a pas tout reçu
                while (total_received < filesize) {
                    chunk_size = recv(s, file_chunk, sizeof(file_chunk), 0);
                    if (chunk_size <= 0) break; // Erreur ou déconnexion

                    fwrite(file_chunk, 1, chunk_size, fp);
                    total_received += chunk_size;
                    printf("\rTelechargement : %ld / %ld octets", total_received, filesize);
                }
                
                fclose(fp);
                printf("\n[SUCCES] Fichier 'releve_nextbank.pdf' telecharge avec succes !\n");
                
                // Ouvrir le PDF automatiquement (Windows)
                system("start releve_nextbank.pdf");
            }
        } 
        else {
            // Cas normal (Texte standard : Login ok, Solde, Stats...)
            printf("%s\n", server_reply);
        }
}
        // Si on arrive ici, c'est que la boucle de session a cassé (break)
        printf("Tentative de reconnexion automatique...\n");
        closesocket(s); // Nettoyage de l'ancien socket
        Sleep(2000); // Petite pause avant de réessayer
    }

    WSACleanup();
    return 0;
}