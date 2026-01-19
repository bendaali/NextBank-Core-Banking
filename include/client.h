#ifndef CLIENT_H
#define CLIENT_H

#ifdef _WIN32
  #ifdef CLIENT_EXPORTS
    #define CLIENT_API __declspec(dllexport)
  #else
    #define CLIENT_API __declspec(dllimport)
  #endif
#else
  #define CLIENT_API
#endif

// Connexion / déconnexion
CLIENT_API int Api_Connect(const char* ip, int port);
CLIENT_API int Api_TryConnect(const char* ip, int port, int maxRetries, int retryDelayMs);
CLIENT_API void Api_Disconnect(void);

// Communication
CLIENT_API int Api_Send(const char* message);
CLIENT_API int Api_Recv(char* buffer, int bufLen);

// Utilitaires
CLIENT_API int Api_Login(int id, const char* password);
// Actions sur le compte
CLIENT_API int Api_DownloadFile(const char* local_filename, long expected_bytes);
CLIENT_API int Api_GetSolde(double* out_solde);
CLIENT_API int Api_GetClientInfo(char* out_nom, char* out_prenom, char* out_type, char* out_date);
CLIENT_API int Api_CreerCompte(char* nom, char* prenom, char* pass, char* type, int* out_new_id, char* res);
CLIENT_API int Api_GetHistoryRaw(char* out_buffer, int buffer_size);
CLIENT_API int Api_Transaction_Depot(double montant, char* response, int response_size);
CLIENT_API int Api_Transaction_Retrait(double montant, char* response, int response_size);
CLIENT_API int Api_Transaction_Virement(int id_dest, double montant, char* response, int response_size);
// Statut
CLIENT_API int Api_IsConnected(void);
CLIENT_API int Api_GetLoggedInUserID(void);
CLIENT_API int Api_LOGOUT(void);



#endif // CLIENT_H
