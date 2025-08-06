#ifndef IPC_COMMON_H
#define IPC_COMMON_H

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <unistd.h>

// Taille maximale pour un message
#define MAX_MESSAGE_SIZE 4096
#define SHARED_MEM_NAME "/ipc_router_shared"

// Structure de données partagée
struct SharedData {
    sem_t mutex;           // Sémaphore pour l'accès exclusif
    sem_t data_ready;      // Sémaphore pour signaler qu'un message est prêt
    sem_t response_ready;  // Sémaphore pour signaler qu'une réponse est prête

    // Buffer pour les messages
    char message[MAX_MESSAGE_SIZE];

    // Métadonnées du message actuel
    bool has_message;      // Indique si un message est présent
    size_t message_size;   // Taille réelle du message

    // Buffer pour les réponses (optionnel)
    char response[MAX_MESSAGE_SIZE];
    size_t response_size;
    bool has_response;
};

// Structure de base pour tous les messages
struct IPCMessage {
    uint32_t message_id;     // ID unique du type de message
    uint32_t route_hash;     // Hash de la route
    uint32_t payload_size;   // Taille des données utiles
    char payload[];          // Données variables
};

// Exemples de structures de requêtes
struct CreateUserRequest {
    char username[64];
    char email[128];
};

struct AddFunctionIRRequest {
    char function_code_hash[256];
    uint32_t bit_array_size;     // Nombre de bits dans le tableau
    uint8_t bit_array[];         // Tableau de bits flexible (Flexible Array Member)
};

struct GetFunctionIRRequest {
    char function_code_hash[256];
    uint32_t bit_array_size; // Taille du tableau de bits
    uint8_t bit_array[];     // Tableau de bits flexible
};

struct GetUserRequest {
    uint32_t user_id;
};

struct DeleteUserRequest {
    uint32_t user_id;
};

// Fonctions utilitaires
inline uint32_t hash_route(const std::string& route) {
    std::hash<std::string> hasher;
    return hasher(route);
}

inline uint32_t generate_message_id() {
    static uint32_t counter = 0;
    return ++counter;
}

#endif // IPC_COMMON_H
