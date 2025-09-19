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
#include "../m_cache/picosha2.h"

// Taille maximale pour un message
#define MAX_MESSAGE_SIZE 4096*5
#define SHARED_MEM_NAME "/ipc_router_shared"

// Structure de données partagée
struct SharedData
{
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

     // ID de message pour associer requête/réponse
    uint32_t current_message_id;
    uint32_t response_message_id;
};

// Structure de base pour tous les messages
struct IPCMessage
{
    uint32_t message_id;     // ID unique du type de message
    char route_hash[256];     // Hash de la route
    uint32_t payload_size;   // Taille des données utiles
    char payload[];          // Données variables
};

// Exemples de structures de requêtes
struct CreateUserRequest
{
    char username[64];
    char email[128];
};

// Structure pour la sérialisation de SerializeTFGraph
struct AddFunctionIRRequest {
    char function_code_hash[256];
    uint32_t serialized_graph_size;  // Taille des données sérialisées
    uint8_t serialized_graph[];      // Données sérialisées du graphe
};

// Structure pour la réponse
struct AddFunctionIRResponse {
    bool success;
    char error_message[128];
};

// Structure de requête pour récupérer un graphique IR
struct GetFunctionIRGraphRequest {
    char function_code_hash[256];
};

// Structure de réponse avec graphique IR sérialisé
struct GetFunctionIRGraphResponse {
    bool success;                     // Indique si la fonction a été trouvée
    uint32_t serialized_graph_size;   // Taille des données sérialisées
    char error_message[128];          // Message d'erreur si success = false
    uint8_t serialized_graph[];       // Graphique sérialisé (Flexible Array Member)
};


struct GetFunctionIRRequest
{
    char function_code_hash[256];
};


// Structure de réponse avec tableau de bits variable
struct GetFunctionIRResponse {
    bool success;                    // Indique si la fonction a été trouvée
    uint32_t bit_array_size;        // Nombre de bits dans le tableau
    char error_message[128];        // Message d'erreur si success = false
    uint8_t bit_array[];            // Tableau de bits flexible
};

struct GetUserRequest
{
    uint32_t user_id;
};

struct DeleteUserRequest
{
    uint32_t user_id;
};

// Fonctions utilitaires
inline std::string hash_route(const std::string& route)
{
    return picosha2::hash256_hex_string(route);
}

inline uint32_t generate_message_id()
{
    static uint32_t counter = 0;
    return ++counter;
}

#endif // IPC_COMMON_H
