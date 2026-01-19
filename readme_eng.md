# NextBank - Core Banking System

A comprehensive C-based banking system featuring a server application, client interface, and API for managing financial accounts and transactions with security, concurrency, and data integrity features.

## Overview

NextBank is a multi-component banking platform built in C that provides:
- **Server**: Core banking service handling multiple concurrent clients
- **Client**: Command-line interface for bank operations
- **API**: DLL-based API for third-party integration

## Features

- **Account Management**: Create, update, and manage bank accounts
- **Transactions**: Record and track financial transactions
- **Security**: Password hashing, encryption, and data integrity verification
- **Concurrency**: Multi-threaded server supporting concurrent client connections
- **Database**: Persistent data storage with backup and recovery capabilities
- **Compression**: Data compression support for efficient transmission
- **Reporting**: Generate and manage financial reports
- **Indexing**: Fast account and transaction lookup
- **Validation**: Input validation and data integrity checks

## Project Structure

```
.
├── include/              # Header files
│   ├── client.h
│   ├── compression.h
│   ├── concurrency.h
│   ├── database.h
│   ├── index.h
│   ├── models.h
│   ├── report.h
│   ├── security.h
│   └── validation.h
├── src/                  # Source files
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
│   └── output/          # Generated output files
├── librairies/          # External libraries
└── README_eng.md            # This file
```

## Setup & Installation

### Prerequisites

- GCC compiler
- OpenSSL development libraries (`libssl`, `libcrypto`)
- Zlib compression library (`libz`)
- LibHPDF library (`libhpdf`) for PDF report generation
- Windows Sockets 2 (`ws2_32`)

### Compilation

Use the provided compilation commands to build each component:

#### Build Server
```bash
gcc src/server.c src/database.c src/concurrency.c src/security.c src/validation.c src/compression.c src/report.c src/index.c -o server.exe -lws2_32 -lssl -lcrypto -lz -lhpdf
```

#### Build Client
```bash
gcc src/client.c -o client.exe -lws2_32
```

#### Build API (DLL)
```bash
gcc -shared -o BankCore.dll src/api.c -lws2_32 "-Wl,--kill-at"
```

## Components

### Server (`server.c`)
- Manages client connections with multi-threaded support
- Handles account operations and transactions
- Maintains database consistency
- Processes client requests and sends responses

### Client (`client.c`)
- Command-line interface for end users
- Connects to the server for account operations
- Handles user authentication and transactions

### Database (`database.c`)
- Persistent account and transaction storage
- Backup and recovery functionality
- Transaction logging
- Account search and retrieval

### Security (`security.c`)
- Password hashing and verification
- Data encryption/decryption
- File integrity verification
- Security management

### Concurrency (`concurrency.c`)
- Multi-threaded client handling
- Thread synchronization
- Concurrent request processing

### Compression (`compression.c`)
- Data compression for network transmission
- Bandwidth optimization

### Validation (`validation.c`)
- Input validation for all operations
- Data type checking
- Format validation

### Indexing (`index.c`)
- Fast account lookups
- Transaction indexing
- Query optimization

### Reporting (`report.c`)
- Financial report generation
- Data analysis and export
- PDF report creation

## Usage

### Starting the Server
```bash
./server.exe
```
The server will start and listen for client connections on port 8080. It initializes the database, verifies data integrity, and is ready to accept client connections.

### Starting the Client
```bash
./client.exe
```
The client will connect to the running server (localhost:8080) and provide an interactive interface for banking operations. The client automatically attempts to reconnect if the connection is lost.

### Using the API
The `BankCore.dll` can be integrated into third-party applications by calling the exported functions from `api.c`.

## Available Commands

### Before Login (Public Commands)

#### CREER
Create a new bank account.
```
CREER <Nom> <Prenom> <Password> <Type>
```
- **Nom**: Account holder's last name
- **Prenom**: Account holder's first name
- **Password**: Account password (validated before creation)
- **Type**: Account type (e.g., "COURANT", "EPARGNE")

Example: `CREER Dupont Jean motdepasse123 COURANT`

#### LOGIN
Connect to an existing account.
```
LOGIN <ID> <Password>
```
- **ID**: Account ID (assigned during account creation)
- **Password**: Account password

Example: `LOGIN 1 motdepasse123`

#### HELP
Display available commands for your current session state.
```
HELP
```

---

### After Login (User Commands)

#### DEPOT
Deposit money into your account.
```
DEPOT <Amount>
```
- **Amount**: Amount to deposit (must be positive)

Example: `DEPOT 500.50`

#### RETRAIT
Withdraw money from your account.
```
RETRAIT <Amount>
```
- **Amount**: Amount to withdraw (must be positive and not exceed balance)

Example: `RETRAIT 100`

#### VIREMENT
Transfer money to another account.
```
VIREMENT <ID_Destination> <Amount>
```
- **ID_Destination**: ID of the destination account
- **Amount**: Amount to transfer (must be positive and not exceed balance)

Example: `VIREMENT 2 250.75`

#### SOLDE
Check your current account balance.
```
SOLDE
```

#### INFO
Display your account information.
```
INFO
```
Returns: Name, First name, Account type, Creation date

#### HISTORY
View the complete transaction history of your account.
```
HISTORY
```

#### STATS
Get statistical information about your account.
```
STATS
```
Displays account statistics including transaction counts, average transaction amount, etc.

#### PDF
Generate and download a PDF report of your account with transaction history.
```
PDF
```
The generated PDF file (`releve_nextbank.pdf`) is automatically downloaded and opened on your computer.

#### LOGOUT
Disconnect from your account.
```
LOGOUT
```

#### HELP
Display all available commands.
```
HELP
```

---

### Admin Commands (User ID = 1 only)

Admin users have access to additional administrative commands:

#### LISTE_COMPTES
List all accounts in the system.
```
LISTE_COMPTES
```

#### BACKUP
Create a backup of the entire database.
```
BACKUP
```

#### ROTATE_LOGS
Perform rotation of transaction log files.
```
ROTATE_LOGS
```

#### AFFICHER_LOGS
Display all transaction logs.
```
AFFICHER_LOGS
```

#### RECHERCHER_NOM
Search for accounts by account holder's name.
```
RECHERCHER_NOM <Name>
```
- **Name**: Last name to search for

Example: `RECHERCHER_NOM Dupont`

---

### General

#### exit
Close the client application.
```
exit
```

## Data Models

The system uses the following core data structures (defined in `models.h`):
- **Compte** (Account): Stores account information and balance
- **Transaction**: Records financial transactions
- Additional structures for security and validation

## Security Features

- **Password Hashing**: Secure password storage using cryptographic hashing
- **Data Encryption**: Sensitive data is encrypted at rest
- **Integrity Verification**: File integrity checks ensure data hasn't been tampered with
- **Backup System**: Automated backups with restoration capability

## Database Features

- **Account Management**: Save, update, and retrieve accounts
- **Transaction Logging**: Complete audit trail of all transactions
- **Backup & Recovery**: Automatic backup creation and restoration
- **Search Functionality**: Find accounts by name and other criteria
- **Log Rotation**: Manage log file sizes

## Building & Distribution

Compiled executables are stored in the `output/` directory:
- `server.exe` - Bank server
- `client.exe` - Bank client
- `BankCore.dll` - Banking API

## Notes

- All source files use the `.c` extension (C language)
- The project uses GCC for compilation
- Windows Sockets 2 is used for networking (platform: Windows)
- OpenSSL is required for cryptographic operations
- LibHPDF is used for PDF report generation

## License

MIT License - see LICENSE for details.
