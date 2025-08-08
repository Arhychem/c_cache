#include "client_test.h"

IPCClient::IPCClient() : shared_data(nullptr) {}

IPCClient::~IPCClient()
{
    if (shared_data) {
        munmap(shared_data, sizeof(SharedData));
    }
}

SharedData* IPCClient::connect_shared_memory()
{
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

bool IPCClient::initialize()
{
    shared_data = connect_shared_memory();
    return shared_data != nullptr;
}

bool IPCClient::create_user(const std::string& username, const std::string& email)
{
    CreateUserRequest request;
    strncpy(request.username, username.c_str(), sizeof(request.username) - 1);
    request.username[sizeof(request.username) - 1] = '\0';
    strncpy(request.email, email.c_str(), sizeof(request.email) - 1);
    request.email[sizeof(request.email) - 1] = '\0';

    return send_request("user/create", request);
}

bool IPCClient::get_user(uint32_t user_id)
{
    GetUserRequest request;
    request.user_id = user_id;

    return send_request("user/get", request);
}

bool IPCClient::delete_user(uint32_t user_id)
{
    DeleteUserRequest request;
    request.user_id = user_id;

    return send_request("user/delete", request);
}

bool IPCClient::send_variable_request(const std::string& route, const void* data, size_t data_size)
{
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

bool IPCClient::add_function_ir_graph(const std::string& function_hash,
                                     const SerializeTFGraph& graph) {
    // Sérialiser le graphique
    std::vector<uint8_t> serialized_data;
    try {
        serialized_data = GraphSerializer::serialize_to_bytes(graph);
    } catch (const std::exception& e) {
        printf("Erreur de sérialisation: %s\n", e.what());
        return false;
    }

    // Calculer la taille totale de la structure
    size_t total_struct_size = sizeof(AddFunctionIRRequest) + serialized_data.size();

    // Allouer temporairement la structure
    uint8_t* buffer = new uint8_t[total_struct_size];
    AddFunctionIRRequest* request = (AddFunctionIRRequest*)buffer;

    // Remplir la structure
    strncpy(request->function_code_hash, function_hash.c_str(),
            sizeof(request->function_code_hash)-1);
    request->function_code_hash[sizeof(request->function_code_hash)-1] = '\0';
    request->serialized_graph_size = static_cast<uint32_t>(serialized_data.size());

    // Copier les données sérialisées
    memcpy(request->serialized_graph, serialized_data.data(), serialized_data.size());

    // Envoyer la requête
    bool result = send_variable_request("function/add_ir_graph", request, total_struct_size);

    // Libérer la mémoire temporaire
    delete[] buffer;

    printf("Graphique sérialisé envoyé: %zu octets\n", serialized_data.size());
    return result;
}


template<typename RequestType>
bool IPCClient::send_request_with_response(const std::string& route,
    const RequestType& request,
    uint32_t& message_id)
{
    if (!shared_data) return false;

    sem_wait(&shared_data->mutex);

    if (shared_data->has_message) {
        sem_post(&shared_data->mutex);
        return false;
    }

    // Préparer le message
    IPCMessage* message = (IPCMessage*)shared_data->message;
    message_id = generate_message_id();
    message->message_id = message_id;
    message->route_hash = hash_route(route);
    message->payload_size = sizeof(RequestType);

    // Vérifier la taille
    size_t total_size = sizeof(IPCMessage) + sizeof(RequestType);
    if (total_size > MAX_MESSAGE_SIZE) {
        sem_post(&shared_data->mutex);
        return false;
    }

    memcpy(message->payload, &request, sizeof(RequestType));

    shared_data->has_message = true;
    shared_data->message_size = total_size;
    shared_data->current_message_id = message_id;

    // Signaler au serveur
    sem_post(&shared_data->data_ready);

    return true;
}

bool IPCClient::wait_for_response(uint32_t message_id, void* response_buffer,
    size_t buffer_size, size_t& actual_response_size)
{
    if (!shared_data) return false;

    // Attendre la réponse
    sem_wait(&shared_data->response_ready);

    // Vérifier si c'est la bonne réponse
    if (shared_data->response_message_id != message_id) {
        printf("Erreur: ID de réponse incorrect\n");
        return false;
    }

    if (shared_data->response_size > buffer_size) {
        printf("Erreur: Buffer de réponse trop petit\n");
        return false;
    }

    // Copier la réponse
    memcpy(response_buffer, shared_data->response, shared_data->response_size);
    actual_response_size = shared_data->response_size;

    // Marquer la réponse comme lue
    shared_data->has_response = false;

    return true;
}

bool IPCClient::get_function_ir(const std::string& function_hash,
    std::vector<uint8_t>& ir_bits)
{
    GetFunctionIRRequest request;
    strncpy(request.function_code_hash, function_hash.c_str(),
        sizeof(request.function_code_hash) - 1);
    request.function_code_hash[sizeof(request.function_code_hash) - 1] = '\0';

    uint32_t message_id;
    if (!send_request_with_response("function/get_ir", request, message_id)) {
        return false;
    }

    // Buffer pour recevoir la réponse
    uint8_t response_buffer[MAX_MESSAGE_SIZE];
    size_t response_size;

    if (!wait_for_response(message_id, response_buffer,
        sizeof(response_buffer), response_size)) {
        return false;
    }

    // Parser la réponse
    const GetFunctionIRResponse* response =
        (const GetFunctionIRResponse*)response_buffer;

    if (!response->success) {
        printf("Erreur serveur: %s\n", response->error_message);
        return false;
    }

    // Extraire les bits
    uint32_t num_bytes = (response->bit_array_size + 7) / 8;
    ir_bits.resize(num_bytes);
    memcpy(ir_bits.data(), response->bit_array, num_bytes);

    printf("IR récupéré: %u bits (%u octets)\n",
        response->bit_array_size, num_bytes);

    return true;
}
bool IPCClient::get_function_ir_graph(const std::string& function_hash,
                                     SerializeTFGraph& deserialized_graph) {
    GetFunctionIRGraphRequest request;
    strncpy(request.function_code_hash, function_hash.c_str(),
            sizeof(request.function_code_hash)-1);
    request.function_code_hash[sizeof(request.function_code_hash)-1] = '\0';

    uint32_t message_id;
    if (!send_request_with_response("function/get_ir_graph", request, message_id)) {
        printf("Erreur: impossible d'envoyer la requête\n");
        return false;
    }

    // Buffer pour recevoir la réponse
    uint8_t response_buffer[MAX_MESSAGE_SIZE];
    size_t response_size;

    if (!wait_for_response(message_id, response_buffer,
                          sizeof(response_buffer), response_size)) {
        printf("Erreur: timeout ou problème de réception de réponse\n");
        return false;
    }

    // Parser la réponse
    const GetFunctionIRGraphResponse* response =
        (const GetFunctionIRGraphResponse*)response_buffer;

    if (!response->success) {
        printf("Erreur serveur: %s\n", response->error_message);
        return false;
    }

    // Vérifier la cohérence des tailles
    size_t expected_size = sizeof(GetFunctionIRGraphResponse) + response->serialized_graph_size;
    if (response_size != expected_size) {
        printf("Erreur: taille de réponse incohérente\n");
        return false;
    }

    // Désérialiser le graphique
    try {
        deserialized_graph = GraphSerializer::deserialize_from_bytes(
            response->serialized_graph, response->serialized_graph_size);

        printf("Graphique IR récupéré avec succès:\n");
        printf("- Nombre de nœuds: %zu\n", deserialized_graph.graph_nodes.size());
        printf("- Node start ID: %u\n", deserialized_graph.node_start_id);
        printf("- Node end ID: %u\n", deserialized_graph.node_end_id);
        printf("- Has SIMD: %s\n", deserialized_graph.has_simd ? "true" : "false");

        return true;

    } catch (const std::exception& e) {
        printf("Erreur de désérialisation: %s\n", e.what());
        return false;
    }
}
