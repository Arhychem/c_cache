#ifndef IPC_CLIENT_H
#define IPC_CLIENT_H

#include "../server/common.h"

class IPCClient
{
private:
    SharedData* shared_data;

    // Gestion de la mémoire partagée
    SharedData* connect_shared_memory();

public:
    IPCClient();
    ~IPCClient();

    bool initialize();

    // Méthode générique pour envoyer des requêtes
    template <typename RequestType>
    bool send_request(const std::string& route, const RequestType& request);
    bool send_variable_request(const std::string& route, const void* data, size_t data_size);
    bool add_function_ir(const std::string& function_hash, const uint8_t* bits, uint32_t num_bits);
    // méthode pour requête avec réponse attendue
    template<typename RequestType>
    bool send_request_with_response(const std::string& route,
        const RequestType& request,
        uint32_t& message_id);

    // Attendre et récupérer une réponse
    bool wait_for_response(uint32_t message_id, void* response_buffer,
        size_t buffer_size, size_t& actual_response_size);


    // Helpers spécifiques pour chaque type de requête
    bool create_user(const std::string& username, const std::string& email);
    bool get_user(uint32_t user_id);
    bool delete_user(uint32_t user_id);
    // Helper spécifique pour GetFunctionIR
    bool get_function_ir(const std::string& function_hash,
        std::vector<uint8_t>& ir_bits);
};

// Implémentation du template dans le header
template<typename RequestType>
bool IPCClient::send_request(const std::string& route, const RequestType& request)
{
    if (!shared_data) return false;

    // Attendre l'accès exclusif
    sem_wait(&shared_data->mutex);

    // Vérifier que le buffer est libre4
    if (shared_data->has_message) {
        sem_post(&shared_data->mutex);
        printf("Erreur: Buffer occupé, message non envoyé\n");
        return false;
    }

    // Préparer le message
    IPCMessage* message = (IPCMessage*)shared_data->message;
    message->message_id = generate_message_id();
    message->route_hash = hash_route(route);
    message->payload_size = sizeof(RequestType);

    // Vérifier la taille
    size_t total_size = sizeof(IPCMessage) + sizeof(RequestType);
    if (total_size > MAX_MESSAGE_SIZE) {
        sem_post(&shared_data->mutex);
        printf("Erreur: Message trop volumineux\n");
        return false;
    }

    // Copier les données
    memcpy(message->payload, &request, sizeof(RequestType));

    // Marquer le message comme prêt
    shared_data->has_message = true;
    shared_data->message_size = total_size;

    // Signaler au serveur qu'un message est prêt
    sem_post(&shared_data->data_ready);

    printf("Message envoyé vers route: %s\n", route.c_str());
    return true;
}

#endif // IPC_CLIENT_H
