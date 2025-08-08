#include "cache_server.h"
#include "../m_cache/m_v8_shared_cache.h"
#include "../m_cache/m_graph_serializer.h"

IPCServer::IPCServer() : shared_data(nullptr), running(false) {}

IPCServer::~IPCServer()
{
    if (shared_data) {
        // Nettoyer les sémaphores
        sem_destroy(&shared_data->mutex);
        sem_destroy(&shared_data->data_ready);
        sem_destroy(&shared_data->response_ready);

        // Détacher la mémoire partagée
        munmap(shared_data, sizeof(SharedData));

        // Supprimer le segment de mémoire partagée
        shm_unlink(SHARED_MEM_NAME);
    }
}

SharedData* IPCServer::create_shared_memory()
{
    // Créer le segment de mémoire partagée
    int shm_fd = shm_open(SHARED_MEM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return nullptr;
    }

    // Définir la taille
    if (ftruncate(shm_fd, sizeof(SharedData)) == -1) {
        perror("ftruncate");
        close(shm_fd);
        return nullptr;
    }

    // Mapper en mémoire
    SharedData* data = (SharedData*)mmap(0, sizeof(SharedData),
        PROT_READ | PROT_WRITE,
        MAP_SHARED, shm_fd, 0);
    close(shm_fd);

    if (data == MAP_FAILED) {
        perror("mmap");
        return nullptr;
    }

    // Initialiser les sémaphores (1 = partagé entre processus)
    if (sem_init(&data->mutex, 1, 1) == -1 ||
        sem_init(&data->data_ready, 1, 0) == -1 ||
        sem_init(&data->response_ready, 1, 0) == -1) {
        perror("sem_init");
        munmap(data, sizeof(SharedData));
        return nullptr;
    }

    // Initialiser les flags
    data->has_message = false;
    data->has_response = false;
    data->message_size = 0;
    data->response_size = 0;

    return data;
}

bool IPCServer::initialize()
{
    // Créer la mémoire partagée
    shared_data = create_shared_memory();
    if (!shared_data) {
        return false;
    }

    // Configurer les routes
    initialize_routes();
    return true;
}

void IPCServer::initialize_routes()
{
    router.register_route<CreateUserRequest>("user/create",
        [this](const CreateUserRequest& req) {
            handle_create_user(req);
        });

    router.register_route<GetUserRequest>("user/get",
        [this](const GetUserRequest& req) {
            handle_get_user(req);
        });

    router.register_route<DeleteUserRequest>("user/delete",
        [this](const DeleteUserRequest& req) {
            handle_delete_user(req);
        });
    router.register_variable_route("function/add_ir_graph",
        [this](const char* data, size_t size) {
            std::cout << "Handling variable route for function/add_ir_graph" << std::endl;
            handle_add_function_ir_graph(data, size);
        });
    router.register_route<GetFunctionIRRequest>("function/get_ir",
        [this](const GetFunctionIRRequest& req) {
            // Récupérer l'ID du message depuis shared_data
            uint32_t message_id = shared_data->current_message_id;
            handle_get_function_ir(req, message_id);
        });
    router.register_route<GetFunctionIRGraphRequest>("function/get_ir_graph",
        [this](const GetFunctionIRGraphRequest& req) {
            uint32_t message_id = shared_data->current_message_id;
            handle_get_function_ir_graph(req, message_id);
        });
}

void IPCServer::handle_create_user(const CreateUserRequest& request)
{
    printf("=== CRÉATION UTILISATEUR ===\n");
    printf("Nom d'utilisateur: %s\n", request.username);
    printf("Email: %s\n", request.email);
    printf("Utilisateur créé avec succès!\n\n");
}

void IPCServer::handle_get_user(const GetUserRequest& request)
{
    printf("=== RÉCUPÉRATION UTILISATEUR ===\n");
    printf("ID utilisateur demandé: %u\n", request.user_id);
    printf("Données utilisateur récupérées!\n\n");
}

void IPCServer::handle_delete_user(const DeleteUserRequest& request)
{
    printf("=== SUPPRESSION UTILISATEUR ===\n");
    printf("ID utilisateur à supprimer: %u\n", request.user_id);
    printf("Utilisateur supprimé avec succès!\n\n");
}

void IPCServer::handle_add_function_ir_graph(const char* data, size_t size)
{
    if (size < sizeof(AddFunctionIRRequest)) {
        printf("Erreur: données insuffisantes pour AddFunctionIRRequest\n");
        return;
    }

    const AddFunctionIRRequest* request = (const AddFunctionIRRequest*)data;

    printf("=== AJOUT GRAPHIQUE IR AVEC CACHE ===\n");
    printf("Hash de la fonction: %s\n", request->function_code_hash);
    printf("Taille des données sérialisées: %u octets\n", request->serialized_graph_size);

    // Vérifier la cohérence des tailles
    size_t expected_total_size = sizeof(AddFunctionIRRequest) + request->serialized_graph_size;
    if (size != expected_total_size) {
        printf("Erreur: taille des données incorrecte\n");
        return;
    }

    try {
        // Désérialiser le graphique pour validation
        // v8::internal::compiler::SerializeTFGraph deserialized_graph = v8::internal::compiler::GraphSerializer::deserialize_from_bytes(
        //     request->serialized_graph, request->serialized_graph_size);

        // Stocker dans le cache partagé
        m_cache::SharedCache& cache = m_cache::SharedCache::Instance();

        if (cache.Put(std::string(request->function_code_hash),
            request->serialized_graph, request->serialized_graph_size)) {
            printf("Graphique IR stocké dans le cache avec succès!\n");
            printf("- Entrées dans le cache: %u\n", cache.GetEntryCount());
            printf("- Espace utilisé: %u octets\n", cache.GetUsedSpace());
        }
        else {
            printf("Erreur: impossible de stocker dans le cache\n");
        }

    }
    catch (const std::exception& e) {
        printf("Erreur de désérialisation: %s\n", e.what());
    }

    printf("\n");
}


void IPCServer::handle_get_function_ir(const GetFunctionIRRequest& request,
    uint32_t message_id)
{
    printf("=== RÉCUPÉRATION IR FONCTION ===\n");
    printf("Hash demandé: %s\n", request.function_code_hash);

    // Simuler la recherche de la fonction
    // Dans une vraie application, vous chercheriez dans une base de données

    if (strcmp(request.function_code_hash, "EXISTING_FUNCTION") == 0) {
        // Fonction trouvée - créer une réponse avec des données IR
        uint8_t sample_ir[] = { 0x48, 0x89, 0xe5, 0x48, 0x83, 0xec, 0x10, 0xc7, 0x45, 0xfc };
        uint32_t num_bits = 80; // 10 octets * 8 bits

        // Calculer la taille totale de la réponse
        size_t response_size = sizeof(GetFunctionIRResponse) + sizeof(sample_ir);

        // Allouer temporairement la réponse
        uint8_t* buffer = new uint8_t[response_size];
        GetFunctionIRResponse* response = (GetFunctionIRResponse*)buffer;

        response->success = true;
        response->bit_array_size = num_bits;
        strcpy(response->error_message, "");
        memcpy(response->bit_array, sample_ir, sizeof(sample_ir));

        // Envoyer la réponse
        send_response(message_id, response, response_size);

        printf("IR envoyé: %u bits\n", num_bits);
        delete[] buffer;

    }
    else {
        // Fonction non trouvée
        GetFunctionIRResponse response;
        response.success = false;
        response.bit_array_size = 0;
        strcpy(response.error_message, "Fonction non trouvée");

        send_response(message_id, &response, sizeof(response));
        printf("Fonction non trouvée\n");
    }

    printf("\n");
}

void IPCServer::handle_get_function_ir_graph(const GetFunctionIRGraphRequest& request,
                                           uint32_t message_id) {
    printf("=== RÉCUPÉRATION GRAPHIQUE IR ===\n");
    printf("Hash de la fonction demandée: %s\n", request.function_code_hash);

    // Rechercher dans le cache partagé
    m_cache::SharedCache& cache = m_cache::SharedCache::Instance();

    const uint8_t* cached_data = nullptr;
    uint32_t cached_size = 0;

    if (cache.Get(std::string(request.function_code_hash), &cached_data, cached_size)) {
        printf("Graphique trouvé dans le cache (%u octets)\n", cached_size);

        // Créer la réponse avec les données du cache
        size_t response_size = sizeof(GetFunctionIRGraphResponse) + cached_size;

        // Allouer temporairement la réponse
        uint8_t* buffer = new uint8_t[response_size];
        GetFunctionIRGraphResponse* response = (GetFunctionIRGraphResponse*)buffer;

        response->success = true;
        response->serialized_graph_size = cached_size;
        strcpy(response->error_message, "");

        // Copier les données sérialisées du cache
        memcpy(response->serialized_graph, cached_data, cached_size);

        // Envoyer la réponse
        if (send_response(message_id, response, response_size)) {
            printf("Graphique IR envoyé avec succès au client\n");
        } else {
            printf("Erreur: impossible d'envoyer la réponse\n");
        }

        delete[] buffer;

    } else {
        // Fonction non trouvée dans le cache
        printf("Fonction non trouvée dans le cache\n");

        GetFunctionIRGraphResponse response;
        response.success = false;
        response.serialized_graph_size = 0;
        strcpy(response.error_message, "Fonction non trouvée dans le cache");

        send_response(message_id, &response, sizeof(response));
    }

    printf("\n");
}

bool IPCServer::send_response(uint32_t message_id, const void* response_data,
    size_t response_size)
{
    if (response_size > MAX_MESSAGE_SIZE) {
        printf("Erreur: Réponse trop volumineuse\n");
        return false;
    }

    // Copier la réponse dans le buffer partagé
    memcpy(shared_data->response, response_data, response_size);
    shared_data->response_size = response_size;
    shared_data->response_message_id = message_id;
    shared_data->has_response = true;

    // Signaler au client que la réponse est prête
    sem_post(&shared_data->response_ready);

    return true;
}
void IPCServer::run()
{
    if (!initialize()) {
        fprintf(stderr, "Erreur lors de l'initialisation du serveur\n");
        return;
    }

    printf("=== SERVEUR IPC DÉMARRÉ ===\n");
    printf("En attente de messages...\n\n");

    running = true;

    while (running) {
        // Attendre qu'un message arrive
        sem_wait(&shared_data->data_ready);

        if (!running) break;

        if (shared_data->has_message) {
            // Traiter le message
            IPCMessage* message = (IPCMessage*)shared_data->message;
            std::cout << "Message reçu: ID " << message->message_id
                      << ", Route hash " << message->route_hash
                      << ", Taille " << message->payload_size << std::endl;
            router.dispatch_message(message);

            // Marquer le message comme traité
            shared_data->has_message = false;
        }

        // Libérer le mutex pour permettre de nouveaux messages
        sem_post(&shared_data->mutex);
    }

    printf("Serveur arrêté.\n");
}

void IPCServer::stop()
{
    running = false;
    sem_post(&shared_data->data_ready); // Débloquer la boucle principale
}
