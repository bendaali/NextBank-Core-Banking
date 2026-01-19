# 🏦 NextBank - Système de Gestion Bancaire

Un système bancaire complet intégralement en C, comprenant une application serveur, une interface client et une API pour gérer les comptes financiers et les transactions avec sécurité, concurrence et intégrité des données.

## Aperçu

NextBank est une plateforme bancaire multi-composants construite en C qui fournit :
- **Serveur** : Service bancaire central gérant plusieurs clients simultanés
- **Client** : Interface en ligne de commande pour les opérations bancaires
- **API** : API basée sur DLL pour l'intégration tierces

## Fonctionnalités

- **Gestion des Comptes** : Créer, mettre à jour et gérer les comptes bancaires
- **Transactions** : Enregistrer et suivre les transactions financières
- **Sécurité** : Hachage des mots de passe, chiffrement et vérification d'intégrité des données
- **Concurrence** : Serveur multi-thread supportant les connexions clients simultanées
- **Base de Données** : Stockage persistent des données avec sauvegarde et récupération
- **Compression** : Support de la compression des données pour une transmission efficace
- **Rapports** : Générer et gérer les rapports financiers
- **Indexation** : Recherche rapide des comptes et transactions
- **Validation** : Validation des entrées et vérification de l'intégrité des données

## Structure du Projet

```
.
├── include/              # Fichiers d'en-tête
│   ├── client.h
│   ├── compression.h
│   ├── concurrency.h
│   ├── database.h
│   ├── index.h
│   ├── models.h
│   ├── report.h
│   ├── security.h
│   └── validation.h
├── src/                  # Fichiers sources
│   ├── api.c
│   ├── client.c
│   ├── compression.c
│   ├── concurrency.c
│   ├── database.c
│   ├── index.c
│   ├── report.c
│   ├── security.c
│   ├── server.c
│   ├── validation.c
│   └── output/          # Fichiers de sortie générés
├── librairies/          # Bibliothèques externes
└── README.md            # Ce fichier
```

## Configuration & Installation

### Prérequis

- Compilateur GCC
- Bibliothèques OpenSSL (`libssl`, `libcrypto`)
- Bibliothèque de compression Zlib (`libz`)
- Bibliothèque LibHPDF (`libhpdf`) pour la génération de rapports PDF
- Windows Sockets 2 (`ws2_32`)

### Compilation

Utilisez les commandes de compilation fournies pour construire chaque composant :

#### Compiler le Serveur
```bash
gcc src/server.c src/database.c src/concurrency.c src/security.c src/validation.c src/compression.c src/report.c src/index.c -o server.exe -lws2_32 -lssl -lcrypto -lz -lhpdf
```

#### Compiler le Client
```bash
gcc src/client.c -o client.exe -lws2_32
```

#### Compiler l'API (DLL)
```bash
gcc -shared -o BankCore.dll src/api.c -lws2_32 "-Wl,--kill-at"
```

## Composants

### Serveur (`server.c`)
- Gère les connexions clients avec support multi-thread
- Traite les opérations sur comptes et transactions
- Maintient la cohérence de la base de données
- Traite les requêtes clients et envoie les réponses

### Client (`client.c`)
- Interface en ligne de commande pour les utilisateurs finaux
- Se connecte au serveur pour les opérations sur comptes
- Gère l'authentification et les transactions

### Base de Données (`database.c`)
- Stockage permanent des comptes et transactions
- Fonctionnalités de sauvegarde et récupération
- Enregistrement des transactions
- Recherche et récupération des comptes

### Sécurité (`security.c`)
- Hachage et vérification des mots de passe
- Chiffrement/déchiffrement des données
- Vérification d'intégrité des fichiers
- Gestion de la sécurité

### Concurrence (`concurrency.c`)
- Gestion multi-thread des clients
- Synchronisation des threads
- Traitement concurrent des requêtes

### Compression (`compression.c`)
- Compression des données pour la transmission réseau
- Optimisation de la bande passante

### Validation (`validation.c`)
- Validation des entrées pour toutes les opérations
- Vérification des types de données
- Validation des formats

### Indexation (`index.c`)
- Recherche rapide des comptes
- Indexation des transactions
- Optimisation des requêtes

### Rapports (`report.c`)
- Génération de rapports financiers
- Analyse et export des données
- Création de rapports PDF

## Utilisation

### Démarrer le Serveur
```bash
./server.exe
```
Le serveur démarre et écoute les connexions clients sur le port 8080. Il initialise la base de données, vérifie l'intégrité des données et est prêt à accepter les connexions clients.

### Démarrer le Client
```bash
./client.exe
```
Le client se connecte au serveur en cours d'exécution (localhost:8080) et fournit une interface interactive pour les opérations bancaires. Le client tente automatiquement de se reconnecter si la connexion est perdue.

### Utiliser l'API
La `BankCore.dll` peut être intégrée dans des applications tierces en appelant les fonctions exportées de `api.c`.

## Commandes Disponibles

### Avant la Connexion (Commandes Publiques)

#### CREER
Créer un nouveau compte bancaire.
```
CREER <Nom> <Prenom> <Password> <Type>
```
- **Nom** : Nom de famille du titulaire du compte
- **Prenom** : Prénom du titulaire du compte
- **Password** : Mot de passe du compte (validé avant la création)
- **Type** : Type de compte (ex. "COURANT", "EPARGNE")

Exemple : `CREER Dupont Jean motdepasse123 COURANT`

#### LOGIN
Se connecter à un compte existant.
```
LOGIN <ID> <Password>
```
- **ID** : ID du compte (attribué lors de la création)
- **Password** : Mot de passe du compte

Exemple : `LOGIN 1 motdepasse123`

#### HELP
Afficher les commandes disponibles selon votre état de session.
```
HELP
```

---

### Après la Connexion (Commandes Utilisateur)

#### DEPOT
Déposer de l'argent sur votre compte.
```
DEPOT <Montant>
```
- **Montant** : Montant à déposer (doit être positif)

Exemple : `DEPOT 500.50`

#### RETRAIT
Retirer de l'argent de votre compte.
```
RETRAIT <Montant>
```
- **Montant** : Montant à retirer (doit être positif et ne pas dépasser le solde)

Exemple : `RETRAIT 100`

#### VIREMENT
Transférer de l'argent vers un autre compte.
```
VIREMENT <ID_Destination> <Montant>
```
- **ID_Destination** : ID du compte destinataire
- **Montant** : Montant à transférer (doit être positif et ne pas dépasser le solde)

Exemple : `VIREMENT 2 250.75`

#### SOLDE
Consulter le solde actuel de votre compte.
```
SOLDE
```

#### INFO
Afficher les informations de votre compte.
```
INFO
```
Retourne : Nom, Prénom, Type de compte, Date de création

#### HISTORY
Voir l'historique complet des transactions de votre compte.
```
HISTORY
```

#### STATS
Obtenir des informations statistiques sur votre compte.
```
STATS
```
Affiche les statistiques du compte y compris le nombre de transactions, montant moyen des transactions, etc.

#### PDF
Générer et télécharger un rapport PDF de votre compte avec l'historique des transactions.
```
PDF
```
Le fichier PDF généré (`releve_nextbank.pdf`) est automatiquement téléchargé et ouvert sur votre ordinateur.

#### LOGOUT
Se déconnecter de votre compte.
```
LOGOUT
```

#### HELP
Afficher toutes les commandes disponibles.
```
HELP
```

---

### Commandes Administrateur (ID Utilisateur = 1 uniquement)

Les administrateurs ont accès à des commandes administratives supplémentaires :

#### LISTE_COMPTES
Lister tous les comptes du système.
```
LISTE_COMPTES
```

#### BACKUP
Créer une sauvegarde complète de la base de données.
```
BACKUP
```

#### ROTATE_LOGS
Effectuer une rotation des fichiers journaux de transactions.
```
ROTATE_LOGS
```

#### AFFICHER_LOGS
Afficher tous les journaux de transactions.
```
AFFICHER_LOGS
```

#### RECHERCHER_NOM
Rechercher des comptes par le nom du titulaire.
```
RECHERCHER_NOM <Nom>
```
- **Nom** : Nom à rechercher

Exemple : `RECHERCHER_NOM Dupont`

---

### Général

#### exit
Fermer l'application client.
```
exit
```

## Modèles de Données

Le système utilise les structures de données principales suivantes (définies dans `models.h`) :
- **Compte** (Compte) : Stocke les informations du compte et le solde
- **Transaction** : Enregistre les transactions financières
- Structures supplémentaires pour la sécurité et la validation

## Fonctionnalités de Sécurité

- **Hachage des Mots de Passe** : Stockage sécurisé des mots de passe via hachage cryptographique
- **Chiffrement des Données** : Les données sensibles sont chiffrées au repos
- **Vérification d'Intégrité** : Les vérifications d'intégrité des fichiers garantissent que les données n'ont pas été altérées
- **Système de Sauvegarde** : Sauvegardes automatiques avec capacité de restauration

## Fonctionnalités de Base de Données

- **Gestion des Comptes** : Sauvegarder, mettre à jour et récupérer les comptes
- **Enregistrement des Transactions** : Journal d'audit complet de toutes les transactions
- **Sauvegarde & Récupération** : Création automatique de sauvegarde et restauration
- **Fonctionnalité de Recherche** : Trouver les comptes par nom et autres critères
- **Rotation des Journaux** : Gérer la taille des fichiers journaux

## Construction & Distribution

Les exécutables compilés sont stockés dans le répertoire `output/` :
- `server.exe` - Serveur bancaire
- `client.exe` - Client bancaire
- `BankCore.dll` - API bancaire

## Notes

- Tous les fichiers sources utilisent l'extension `.c` (langage C)
- Le projet utilise GCC pour la compilation
- Windows Sockets 2 est utilisé pour la mise en réseau (plateforme : Windows)
- OpenSSL est requis pour les opérations cryptographiques
- LibHPDF est utilisé pour la génération de rapports PDF

## Licence

Licence MIT - voir [LICENCE](/LICENSE) pour les détails.
