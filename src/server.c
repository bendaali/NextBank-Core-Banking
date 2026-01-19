#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include "../include/models.h"
#include "../include/database.h"
#include "../include/concurrency.h"
#include "../include/security.h"
#include "../include/validation.h"
#include <direct.h> // Pour _mkdir
#include "../include/report.h"
#include "../include/index.h"

#pragma comment(lib, "ws2_32.lib")

// --- THREAD CLIENT ---
DWORD WINAPI ClientHandler(LPVOID socket_desc) {
    SOCKET sock = *(SOCKET*)socket_desc;
    free(socket_desc); // socket_desc is expected to be allocated with malloc in main accept loop

    char buffer[1024];
    int read_size;
    
    // Variables de session
    int logged_in_id = -1; 
    char current_username[100] = "";

    while((read_size = recv(sock, buffer, 1023, 0)) > 0) {
        buffer[read_size] = '\0';
        char reponse[1024] = "";
        printf("Requete recue: %s\n", buffer);
        // COMMANDE: CREER <Nom> <Password>
        if (strncmp(buffer, "CREER", 5) == 0) {
            char nom[100] = "";
            char prenom[100] = "";
            char pass[100] = "";
            char type[100] = "";
            char erreur[200] = "";

            // 1. Parsing
            if(sscanf(buffer, "CREER %s %s %s %s", nom, prenom, pass, type) != 4) {
                send(sock, "Erreur: Format invalide. Usage: CREER <Nom> <Prenom> <Pass> <Type>\n", 50, 0);
                continue; // On passe à la prochaine commande
            }

            // 2. VALIDATION (validation des entrées avant de proceder)
            if (!val_donnees_creation(nom, pass, erreur)) {
                // Si invalide, on envoie l'erreur précise et on s'arrête là
                strcat(erreur, "\n");
                send(sock, erreur, strlen(erreur), 0);
                continue;
            }

            // 3. Logique métier (Seulement si validé)
            Compte c = {0};
            c.id_compte = index_get_next_id();
            
            strcpy(c.nom_titulaire, nom);
            strcpy(c.prenom_titulaire, prenom);
            strcpy(c.type_compte, type);
            strcpy(c.statut, "ACTIF");
            c.solde = 0.0;
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            sprintf(c.date_creation, "%02d/%02d/%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
            hacher_mot_de_passe(pass, c.mot_de_passe_hash); // Module Security

            verrouiller_compte(c.id_compte);
            db_sauvegarder_compte(&c);
            deverrouiller_compte(c.id_compte);
            sprintf(reponse, "Succes ! Compte cree ID: %d\n", c.id_compte);
        }
        
        // le LOGIN
        else if (strncmp(buffer, "LOGIN", 5) == 0) {
            int id;
            char pass[100];
            if(sscanf(buffer, "LOGIN %d %s", &id, pass) == 2) {
                Compte temp;
                if(db_lire_compte(id, &temp)) { //
                    if(verifier_login(&temp, pass)) { //
                        logged_in_id = temp.id_compte;
                        strcpy(current_username, temp.nom_titulaire);
                        sprintf(reponse, "Bienvenue %s. Solde: %.2f\n", temp.nom_titulaire, temp.solde);
                    } else { sprintf(reponse, "Mot de passe incorrect.\n"); }
                } else { sprintf(reponse, "Compte introuvable.\n"); }
            }
        }
        // --- GESTION DES TRANSACTIONS FINANCIERES ---
        else if (logged_in_id != -1) {
            // Commandes réservées à ADMIN (ID 1)
            if (logged_in_id == 1) {
            // commande ADMIN: LISTE_COMPTES
            if (strncmp(buffer, "LISTE_COMPTES", 14) == 0) {
                char liste[2048] = "Liste des comptes:\n";
                db_lister_comptes(liste + strlen(liste), sizeof(liste) - strlen(liste));
                strcpy(reponse, liste);
            }
            // Création backup : BACKUP
            else if (strncmp(buffer, "BACKUP", 6) == 0) {
                if (db_creer_backup()) {
                    sprintf(reponse, "Backup cree avec succes.\n");
                } else {
                    sprintf(reponse, "Erreur lors de la creation du backup.\n");
                }
            }
            // Rotation logs : ROTATE_LOGS
            else if (strncmp(buffer, "ROTATE_LOGS", 12) == 0) {
                db_rotation_logs();
                sprintf(reponse, "Rotation des logs effectuee.\n");
            }
            // Afficher tous les logs
            else if (strncmp(buffer, "AFFICHER_LOGS", 13) == 0) {
                db_afficher_tous_logs();
                sprintf(reponse, "Logs affiches dans la console serveur.\n");
            } else if (strncmp(buffer, "HELP", 4) == 0) {
                sprintf(reponse,
                        "Commandes admin disponibles:\n"
                        "LISTE_COMPTES - Lister tous les comptes\n"
                        "BACKUP - Creer une sauvegarde de la base de donnees\n"
                        "ROTATE_LOGS - Effectuer la rotation des fichiers de logs\n"
                        "AFFICHER_LOGS - Afficher tous les logs de transactions\n"
                        "LOGOUT - Se deconnecter\n"
                );
            }
            //Rechercher par nom
            else if (strncmp(buffer, "RECHERCHER_NOM", 14) == 0) {
                char nom_cible[100];
                if (sscanf(buffer, "RECHERCHER_NOM %s", nom_cible) == 1) {
                    Compte resultats[10];
                    int nb_trouves = db_rechercher_par_nom(nom_cible, resultats, 10);
                    if (nb_trouves > 0) {
                        strcpy(reponse, "Comptes trouves:\n");
                        for (int i = 0; i < nb_trouves; i++) {
                            char ligne[200];
                            sprintf(ligne, "ID: %d | Nom: %s | Prenom: %s | Solde: %.2f\n",
                                    resultats[i].id_compte, resultats[i].nom_titulaire, resultats[i].prenom_titulaire, resultats[i].solde);
                            strcat(reponse, ligne);
                        }
                    } else {
                        sprintf(reponse, "Aucun compte trouve avec ce nom.\n");
                    }
                } else {
                    sprintf(reponse, "Erreur de syntaxe. Utilisez RECHERCHER_NOM <nom>\n");
                }
            }
        }
            // 1. DEPOT
            if (strncmp(buffer, "DEPOT", 5) == 0) {
                double montant;
                if(sscanf(buffer, "DEPOT %lf", &montant) == 1 && montant > 0) {
                    verrouiller_compte(logged_in_id); //
                    
                    Compte c;
                    db_lire_compte(logged_in_id, &c);
                    
                    // Préparation du log
                    Transaction t = {0};
                    t.id_transaction = (int)time(NULL) + (rand() % 1000); //db_generer_id_unique() plus sûr mais ce choix est visuel
                    t.id_compte_source = logged_in_id;
                    t.id_compte_destination = logged_in_id; // Soi-même
                    t.solde_avant = c.solde;
                    
                    // Action
                    c.solde += montant;
                    db_mise_a_jour_compte(&c); // Sauvegarde solde
                    
                    // Finalisation du log
                    t.montant = montant;
                    t.solde_apres = c.solde;
                    strcpy(t.type_operation, "DEPOT");
                    strcpy(t.statut, "REUSSIE");
                    sprintf(t.description, "Depot especes via guichet");
                    t.date_transaction = time(NULL);
                    
                    // ECRIRE DANS LE FICHIER LOG
                    db_logger_transaction(&t); //
                    
                    deverrouiller_compte(logged_in_id);
                    sprintf(reponse, "Succes. Nouveau solde: %.2f\n", c.solde);
                }
            }

            // 2. RETRAIT
            else if (strncmp(buffer, "RETRAIT", 7) == 0) {
                double montant;
                if(sscanf(buffer, "RETRAIT %lf", &montant) == 1 && montant > 0) {
                    verrouiller_compte(logged_in_id);
                    Compte c;
                    db_lire_compte(logged_in_id, &c);
                    
                    Transaction t = {0};
                    t.id_transaction = (int)time(NULL) + (rand() % 1000);
                    t.id_compte_source = logged_in_id;
                    t.id_compte_destination = 0; // Pas de destination
                    t.solde_avant = c.solde;
                    t.montant = montant;
                    strcpy(t.type_operation, "RETRAIT");
                    t.date_transaction = time(NULL);

                    if (c.solde >= montant) {
                        c.solde -= montant;
                        db_mise_a_jour_compte(&c);
                        
                        t.solde_apres = c.solde;
                        strcpy(t.statut, "REUSSIE");
                        sprintf(t.description, "Retrait guichet");
                        sprintf(reponse, "Succes. Nouveau solde: %.2f\n", c.solde);
                    } else {
                        t.solde_apres = c.solde; // Inchangé
                        strcpy(t.statut, "ECHOUEE");
                        sprintf(t.description, "Solde insuffisant");
                        sprintf(reponse, "Erreur: Solde insuffisant.\n");
                    }
                    
                    // ECRIRE DANS LE FICHIER LOG (Même si échoué !)
                    db_logger_transaction(&t); //
                    deverrouiller_compte(logged_in_id);
                }
            }

            // 3. VIREMENT
            else if (strncmp(buffer, "VIREMENT", 8) == 0) {
                int id_dest;
                double montant;
                if(sscanf(buffer, "VIREMENT %d %lf", &id_dest, &montant) == 2 && montant > 0) {
                    verrouiller_compte(logged_in_id); // Verrou global
                    
                    Compte src, dest;
                    int src_ok = db_lire_compte(logged_in_id, &src);
                    int dest_ok = db_lire_compte(id_dest, &dest);
                    
                    Transaction t = {0};
                    t.id_transaction = (int)time(NULL) + (rand() % 1000);
                    t.id_compte_source = logged_in_id;
                    t.id_compte_destination = id_dest;
                    t.solde_avant = src.solde; // Solde de l'émetteur
                    t.montant = montant;
                    strcpy(t.type_operation, "TRANSFERT");
                    t.date_transaction = time(NULL);

                    if (src_ok && dest_ok && src.id_compte != dest.id_compte) {
                        if (src.solde >= montant) {
                            src.solde -= montant;
                            dest.solde += montant;
                            
                            db_mise_a_jour_compte(&src);
                            db_mise_a_jour_compte(&dest);
                            
                            t.solde_apres = src.solde;
                            strcpy(t.statut, "REUSSIE");
                            sprintf(t.description, "Virement de ID %d vers ID %d", logged_in_id, id_dest);
                            sprintf(reponse, "Succes. Virement effectue.\n");
                        } else {
                            t.solde_apres = src.solde;
                            strcpy(t.statut, "ECHOUEE");
                            sprintf(t.description, "Solde insuffisant");
                            sprintf(reponse, "Erreur: Solde insuffisant.\n");
                        }
                    } else {
                        t.solde_apres = src.solde;
                        strcpy(t.statut, "ECHOUEE");
                        sprintf(t.description, "Destinataire invalide");
                        sprintf(reponse, "Erreur: Compte destinataire invalide.\n");
                    }

                    // ECRIRE DANS LE LOG
                    db_logger_transaction(&t); //
                    deverrouiller_compte(logged_in_id);
                }
            } else if (strncmp(buffer, "STATS", 5) == 0) {
                // Pas besoin de mot de passe car l'utilisateur est déjà logged_in_id
                report_generer_statistiques(logged_in_id, reponse);
                // La fonction remplit directement 'reponse' avec le texte du bilan
            
            } 
            else if (strncmp(buffer, "PDF", 3) == 0) {
        
        // On utilise logged_in_id directement !

        Compte c;
        // On relit le compte pour avoir le solde à jour et le nom
        if(db_lire_compte(logged_in_id, &c)) {
            char nom_complet[200];
            sprintf(nom_complet, "%s %s", c.nom_titulaire, c.prenom_titulaire);
            // 1. Génération du PDF
            char filename[100];
            if (report_generer_pdf(logged_in_id, nom_complet, c.solde, filename)) {
                printf("PDF genere pour le compte ID %d\n", logged_in_id);
                // 2. Ouverture du fichier créé
                char server_filename[100];
                sprintf(server_filename, filename, logged_in_id);
                FILE *fp = fopen(server_filename, "rb");
                if (fp) {
                    fseek(fp, 0, SEEK_END);
                    long filesize = ftell(fp);
                    rewind(fp);

                    // 3. Protocole de téléchargement
                    char header[128];
                    sprintf(header, "DOWNLOAD %ld", filesize);
                    send(sock, header, strlen(header), 0);
                    
                    Sleep(50); // Petit délai technique

                    // 4. Envoi des données
                    char file_buffer[4096];
                    int bytes_read;
                    while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), fp)) > 0) {
                        int total_sent = 0;
                        while (total_sent < bytes_read) {
                            int sent = send(sock, file_buffer + total_sent, bytes_read - total_sent, 0);
                            if (sent == SOCKET_ERROR) {
                                break; // handle error if needed
                            }
                            total_sent += sent;
                        }
                    }
                    fclose(fp);
                    
                    //On continue la boucle pour ne pas envoyer 'reponse' par dessus
                    continue; 
                } else {
                    sprintf(reponse, "Erreur serveur: Impossible d'ouvrir le fichier PDF.\n");
                }
            } else {
                sprintf(reponse, "Erreur: Echec de la creation du PDF.\n");
            }
        } else {
            sprintf(reponse, "Erreur critique: Compte introuvable en base.\n");
        }
    }
            // ... (Autres commandes) ...
            else if (strncmp(buffer, "SOLDE", 5) == 0) {
                Compte c;
                if(db_lire_compte(logged_in_id, &c)) {
                    sprintf(reponse, "SOLDE %.2f", c.solde);
                } else {
                    sprintf(reponse, "Erreur: Compte introuvable.\n");
                }
            } else if (strncmp(buffer,"HELP",4) ==0) {
                sprintf(reponse,
                "Commandes disponibles:\n"
                "DEPOT <Montant> - Deposer de l'argent\n"
                "RETRAIT <Montant> - Retirer de l'argent\n"
                "VIREMENT <ID_Dest> <Montant> - Faire un virement\n"
                "HISTORY - Voir l'historique des transactions\n"
                "SOLDE - Voir le solde du compte\n"
                "STATS - Obtenir les statistiques de compte\n"
                "PDF - Generer un rapport PDF des transactions\n"
                "LOGOUT - Se deconnecter\n"
                );

            }
            else if (strncmp(buffer, "LOGOUT", 6) == 0) {
                logged_in_id = -1;
                strcpy(current_username, "");
                sprintf(reponse, "Deconnecte.\n");
            }
            else if (strncmp(buffer, "INFO", 4) == 0) {
                Compte c;
                if(db_lire_compte(logged_in_id, &c)) {
                    sprintf(reponse, "INFO %s %s %s %s", c.nom_titulaire, c.prenom_titulaire, c.type_compte, c.date_creation);
                } else {
                    sprintf(reponse, "Erreur: Compte introuvable.\n");
                }
            }
            else if (strncmp(buffer, "HISTORY", 7) == 0) {
                char history[2048] = "";
                db_lister_transactions(logged_in_id, history, sizeof(history));
                strcpy(reponse, history);
            }
            else if (strlen(reponse) == 0) {
                 sprintf(reponse, "Commande inconnue ou invalide.\n");
            }
            
        } else if (strcmp(buffer,"HELP") ==0) {
            sprintf(reponse,
            "Commandes disponibles:\n"
            "CREER <Nom> <Prenom> <Pass> <Type> - Creer un compte\n"
            "LOGIN <ID> <Pass> - Se connecter a un compte\n"
            );
        }else {
            sprintf(reponse, "Erreur: Veuillez vous connecter d'abord (LOGIN) ou creer un compte (CREER).\n");
        }
        send(sock, reponse, strlen(reponse), 0);
        memset(buffer, 0, sizeof(buffer));
    }
    if(read_size == 0) {
        printf("Client deconnecte.\n");
    } else if(read_size == SOCKET_ERROR) {
        printf("Erreur recv(). Code: %d\n", WSAGetLastError());
    }
    closesocket(sock);
    return 0;
} 

/*
 * main() - Point d'entrée du serveur NextBank.
 * Initialise les modules de base de données, sécurité et verrous, vérifie l'intégrité des données,
 * puis démarre le serveur TCP en attente de connexions clients, chaque client étant géré dans un thread séparé.
 */

int main() {  
    srand(time(NULL)); // Initialize random seed once at startup
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080);
    bind(server_socket, (struct sockaddr *)&server, sizeof(server));
    listen(server_socket, 3);

    // --- INITIALISATION DES MODULES ---
    printf("Demarrage du systeme NextBank...\n");
    init_database();   // Crée les dossiers/fichiers
    init_verrous();    // Prépare les mutex

    // --- VERIFICATION DE SECURITE ---
    printf("[Systeme] Verification de l'integrite des donnees...\n");
  if (!security_verifier_integrite_fichier()) {
        printf("\n[URGENT] ALERTE SECURITE !\n");
        printf("Le fichier 'comptes.dat' a ete modifie manuellement ou est corrompu.\n");

        // Vérifier si un backup est disponible
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
                    return 1;
                }
            } else {
                printf("[Arret] Refus de restauration. Le serveur ne peut pas demarrer avec des donnees corrompues.\n");
                system("pause");
                return 1;
            }
        } else {
            // Pas de backup disponible
            printf("\n[Fatal] Aucun backup n'a ete trouve pour reparer les degats.\n");
            printf("Le serveur refuse de demarrer pour proteger les donnees.\n");
            system("pause");
            return 1;
        }
    } else {
        printf("[Systeme] Integrite validee. Tout est OK.\n");
    }
    // --- CHARGEMENT DE L'INDEX EN MEMOIRE ---
    printf("Chargement de l'index...\n");
    index_charger_depuis_disque("data/comptes.dat");
    // --------------------------------------------------

    printf("Pret. En attente de clients...\n");

    SOCKET client_sock;
    struct sockaddr_in client;
    int c = sizeof(struct sockaddr_in);

    while (1) {
        client_sock = accept(server_socket, (struct sockaddr *)&client, &c);
        if (client_sock == INVALID_SOCKET) {
            printf("[Erreur] Echec de accept(). Code: %d\n", WSAGetLastError());
            break;
        }
        SOCKET *new_sock = malloc(sizeof(SOCKET));
        if (new_sock == NULL) {
            printf("[Erreur] Echec d'allocation memoire pour le socket client.\n");
            closesocket(client_sock);
            continue;
        }
        *new_sock = client_sock;
        CreateThread(NULL, 0, ClientHandler, (void*)new_sock, 0, NULL);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}