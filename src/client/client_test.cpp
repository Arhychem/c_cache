#include "client_test.h"

IPCClient::IPCClient() : shared_data(nullptr) {}

IPCClient::~IPCClient() {
    if (shared_data) {
        munmap(shared_data, sizeof(SharedData));
    }
}

SharedData* IPCClient::connect_shared_memory() {
    int shm_fd = shm_open(SHARED_MEM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open (client)");
        return nullptr;
    }

    SharedData* data = (SharedData*)mmap(0, sizeof(SharedData),
                                        PROT_READ | PROT_WRITE,
                                        MAP_SHARED, shm_fd, 0);
    close(shm_fd);

    if (data == MAP_FAILED) {
        perror("mmap (client)");
        return nullptr;
    }

    return data;
}

bool IPCClient::initialize() {
    shared_data = connect_shared_memory();
    return shared_data != nullptr;
}

bool IPCClient::create_user(const std::string& username, const std::string& email) {
    CreateUserRequest request;
    strncpy(request.username, username.c_str(), sizeof(request.username)-1);
    request.username[sizeof(request.username)-1] = '\0';
    strncpy(request.email, email.c_str(), sizeof(request.email)-1);
    request.email[sizeof(request.email)-1] = '\0';

    return send_request("user/create", request);
}

bool IPCClient::get_user(uint32_t user_id) {
    GetUserRequest request;
    request.user_id = user_id;

    return send_request("user/get", request);
}

bool IPCClient::delete_user(uint32_t user_id) {
    DeleteUserRequest request;
    request.user_id = user_id;

    return send_request("user/delete", request);
}

bool IPCClient::send_variable_request(const std::string& route, const void* data, size_t data_size) {
    if (!shared_data) return false;

    // Attendre l'accès exclusif
    sem_wait(&shared_data->mutex);

    // Vérifier que le buffer est libre
    if (shared_data->has_message) {
        sem_post(&shared_data->mutex);
        printf("Erreur: Buffer occupé, message non envoyé\n");
        return false;
    }

    // Calculer la taille totale du message
    size_t total_size = sizeof(IPCMessage) + data_size;

    // Vérifier la taille
    if (total_size > MAX_MESSAGE_SIZE) {
        sem_post(&shared_data->mutex);
        printf("Erreur: Message trop volumineux (%zu bytes, max: %d)\n",
               total_size, MAX_MESSAGE_SIZE);
        return false;
    }

    // Préparer le message
    IPCMessage* message = (IPCMessage*)shared_data->message;
    message->message_id = generate_message_id();
    message->route_hash = hash_route(route);
    message->payload_size = data_size;

    // Copier les données
    memcpy(message->payload, data, data_size);

    // Marquer le message comme prêt
    shared_data->has_message = true;
    shared_data->message_size = total_size;

    // Signaler au serveur qu'un message est prêt
    sem_post(&shared_data->data_ready);

    printf("Message variable envoyé vers route: %s (%zu bytes)\n", route.c_str(), data_size);
    return true;
}

bool IPCClient::add_function_ir(const std::string& function_hash, const uint8_t* bits, uint32_t num_bits) {
    // Calculer le nombre d'octets nécessaires pour stocker num_bits
    uint32_t num_bytes = (num_bits + 7) / 8;  // Arrondir vers le haut

    // Calculer la taille totale de la structure
    size_t total_struct_size = sizeof(AddFunctionIRRequest) + num_bytes;

    // Allouer temporairement la structure
    uint8_t* buffer = new uint8_t[total_struct_size];
    AddFunctionIRRequest* request = (AddFunctionIRRequest*)buffer;

    // Remplir la structure
    strncpy(request->function_code_hash, function_hash.c_str(), sizeof(request->function_code_hash)-1);
    request->function_code_hash[sizeof(request->function_code_hash)-1] = '\0';
    request->bit_array_size = num_bits;

    // Copier les bits
    memcpy(request->bit_array, bits, num_bytes);

    // Envoyer la requête
    bool result = send_variable_request("function/add_ir", request, total_struct_size);

    // Libérer la mémoire temporaire
    delete[] buffer;

    return result;
}
bool IPCClient::get_function_ir(const std::string& function_hash, const uint8_t* bits, uint32_t num_bits) {
    // Calculer le nombre d'octets nécessaires pour stocker num_bits
    uint32_t num_bytes = (num_bits + 7) / 8;  // Arrondir vers le haut

    // Calculer la taille totale de la structure
    size_t total_struct_size = sizeof(AddFunctionIRRequest) + num_bytes;

    // Allouer temporairement la structure
    uint8_t* buffer = new uint8_t[total_struct_size];
    AddFunctionIRRequest* request = (AddFunctionIRRequest*)buffer;

    // Remplir la structure
    strncpy(request->function_code_hash, function_hash.c_str(), sizeof(request->function_code_hash)-1);
    request->function_code_hash[sizeof(request->function_code_hash)-1] = '\0';
    request->bit_array_size = num_bits;

    // Copier les bits
    memcpy(request->bit_array, bits, num_bytes);

    // Envoyer la requête
    bool result = send_variable_request("function/get_ir", request, total_struct_size);

    // Libérer la mémoire temporaire
    delete[] buffer;

    return result;
}
