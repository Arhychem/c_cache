#include "client_test.h"
#include <iostream>
#include <cstring>
#include <unistd.h>

IPCClient::IPCClient() : shared_data(nullptr), connected(false) {}

IPCClient::~IPCClient()
{
    disconnect();
}

bool IPCClient::connect()
{
    shared_data = open_shared_memory();
    if (shared_data) {
        connected = true;
        std::cout << "Client connecté au serveur IPC" << std::endl;
        return true;
    }
    return false;
}

void IPCClient::disconnect()
{
    if (shared_data && connected) {
        munmap(shared_data, sizeof(SharedData));
        shared_data = nullptr;
        connected = false;
        std::cout << "Client déconnecté du serveur IPC" << std::endl;
    }
}

SharedData* IPCClient::open_shared_memory()
{
    int shm_fd = shm_open(SHARED_MEM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open client");
        return nullptr;
    }

    SharedData* data = (SharedData*)mmap(0, sizeof(SharedData),
        PROT_READ | PROT_WRITE,
        MAP_SHARED, shm_fd, 0);
    close(shm_fd);

    if (data == MAP_FAILED) {
        perror("mmap client");
        return nullptr;
    }

    return data;
}

bool IPCClient::send_message(const void* message_data, size_t message_size, const std::string& route_hash)
{
    if (!connected || !shared_data) {
        std::cerr << "Client non connecté" << std::endl;
        return false;
    }

    if (message_size > MAX_MESSAGE_SIZE - sizeof(IPCMessage)) {
        std::cerr << "Message trop volumineux" << std::endl;
        return false;
    }

    // Attendre l'accès exclusif
    sem_wait(&shared_data->mutex);

    // Construire le message IPC
    IPCMessage* ipc_msg = (IPCMessage*)shared_data->message;
    ipc_msg->message_id = generate_message_id();
    strncpy(ipc_msg->route_hash, route_hash.c_str(), sizeof(ipc_msg->route_hash) - 1);
    ipc_msg->route_hash[sizeof(ipc_msg->route_hash) - 1] = '\0';
    ipc_msg->payload_size = message_size;

    // Copier les données
    memcpy(ipc_msg->payload, message_data, message_size);

    shared_data->message_size = sizeof(IPCMessage) + message_size;
    shared_data->has_message = true;
    shared_data->current_message_id = ipc_msg->message_id;

    // Signaler qu'un message est prêt
    sem_post(&shared_data->data_ready);

    // Libérer l'accès
    sem_post(&shared_data->mutex);

    return true;
}

bool IPCClient::wait_for_response(void* response_buffer, size_t& response_size, uint32_t expected_message_id)
{
    if (!connected || !shared_data) {
        return false;
    }

    // Attendre la réponse
    sem_wait(&shared_data->response_ready);

    // Vérifier l'ID du message
    if (shared_data->response_message_id != expected_message_id) {
        std::cerr << "ID de message de réponse incorrect" << std::endl;
        return false;
    }

    // Copier la réponse
    response_size = shared_data->response_size;
    memcpy(response_buffer, shared_data->response, response_size);

    // Nettoyer
    shared_data->has_response = false;
    shared_data->response_size = 0;

    return true;
}

bool IPCClient::test_create_user()
{
    std::cout << "\n=== TEST CRÉATION UTILISATEUR ===" << std::endl;

    CreateUserRequest request;
    strncpy(request.username, "john_doe", sizeof(request.username) - 1);
    strncpy(request.email, "john@example.com", sizeof(request.email) - 1);

    std::string route_hash = hash_route("user/create");

    if (send_message(&request, sizeof(request), route_hash)) {
        std::cout << "Requête de création d'utilisateur envoyée" << std::endl;
        sleep(1); // Attendre le traitement
        return true;
    }

    return false;
}

bool IPCClient::test_get_user()
{
    std::cout << "\n=== TEST RÉCUPÉRATION UTILISATEUR ===" << std::endl;

    GetUserRequest request;
    request.user_id = 123;

    std::string route_hash = hash_route("user/get");

    if (send_message(&request, sizeof(request), route_hash)) {
        std::cout << "Requête de récupération d'utilisateur envoyée" << std::endl;
        sleep(1);
        return true;
    }

    return false;
}

bool IPCClient::test_delete_user()
{
    std::cout << "\n=== TEST SUPPRESSION UTILISATEUR ===" << std::endl;

    DeleteUserRequest request;
    request.user_id = 456;

    std::string route_hash = hash_route("user/delete");

    if (send_message(&request, sizeof(request), route_hash)) {
        std::cout << "Requête de suppression d'utilisateur envoyée" << std::endl;
        sleep(1);
        return true;
    }

    return false;
}

bool IPCClient::test_add_function_ir()
{
    std::cout << "\n=== TEST AJOUT FONCTION IR ===" << std::endl;

    // Créer des données de test
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint32_t data_size = sizeof(test_data);

    // Calculer la taille totale
    size_t total_size = sizeof(AddFunctionIRRequest) + data_size;
    char* buffer = new char[total_size];

    AddFunctionIRRequest* request = (AddFunctionIRRequest*)buffer;
    strncpy(request->function_code_hash, "test_function_hash", sizeof(request->function_code_hash) - 1);
    request->serialized_graph_size = data_size;

    // Copier les données
    memcpy(request->serialized_graph, test_data, data_size);

    std::string route_hash = hash_route("function/add_ir_graph");

    bool result = send_message(buffer, total_size, route_hash);
    delete[] buffer;

    if (result) {
        std::cout << "Requête d'ajout de fonction IR envoyée" << std::endl;
        sleep(1);
        return true;
    }

    return false;
}

bool IPCClient::test_get_function_ir()
{
    std::cout << "\n=== TEST RÉCUPÉRATION FONCTION IR ===" << std::endl;

    GetFunctionIRRequest request;
    strncpy(request.function_code_hash, "EXISTING_FUNCTION", sizeof(request.function_code_hash) - 1);

    std::string route_hash = hash_route("function/get_ir");

    if (send_message(&request, sizeof(request), route_hash)) {
        std::cout << "Requête de récupération de fonction IR envoyée" << std::endl;
        sleep(1);
        return true;
    }

    return false;
}
