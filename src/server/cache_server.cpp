#include "cache_server.h"

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
    // Nouvelle route pour AddFunctionIR avec taille variable
    router.register_variable_route("function/add_ir",
        [this](const char* data, size_t size) {
            handle_add_function_ir(data, size);
        });
    // Nouvelle route pour GetFunctionIR avec réponse
    router.register_route<GetFunctionIRRequest>("function/get_ir",
        [this](const GetFunctionIRRequest& req) {
            // Récupérer l'ID du message depuis shared_data
            uint32_t message_id = shared_data->current_message_id;
            handle_get_function_ir(req, message_id);
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

void IPCServer::handle_add_function_ir(const char* data, size_t size)
{
    if (size < sizeof(AddFunctionIRRequest)) {
        printf("Erreur: données insuffisantes pour AddFunctionIRRequest\n");
        return;
    }

    const AddFunctionIRRequest* request = (const AddFunctionIRRequest*)data;

    printf("=== AJOUT FONCTION IR ===\n");
    printf("Hash de la fonction: %s\n", request->function_code_hash);
    printf("Nombre de bits: %u\n", request->bit_array_size);

    // Calculer le nombre d'octets attendus
    uint32_t expected_bytes = (request->bit_array_size + 7) / 8;
    size_t expected_total_size = sizeof(AddFunctionIRRequest) + expected_bytes;

    if (size != expected_total_size) {
        printf("Erreur: taille des données incorrecte (reçu: %zu, attendu: %zu)\n",
            size, expected_total_size);
        return;
    }

    // Afficher quelques bits pour vérification
    printf("Premiers octets du tableau de bits: ");
    for (uint32_t i = 0; i < std::min(8u, expected_bytes); i++) {
        printf("%02X ", request->bit_array[i]);
    }
    printf("\n");

    printf("Fonction IR ajoutée avec succès!\n\n");
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
