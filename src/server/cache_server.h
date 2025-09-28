#ifndef CACHE_SERVER_H
#define CACHE_SERVER_H


#include "common.h"
#include "router.h"

class IPCServer
{
private:
    IPCRouter router;
    SharedData* shared_data;
    bool running;

    // Fonctions de gestion des requêtes
    void handle_create_user(const CreateUserRequest& request);
    void handle_get_user(const GetUserRequest& request);
    void handle_delete_user(const DeleteUserRequest& request);
    void handle_add_function_ir_graph(const char* data, size_t size);
    void handle_get_function_ir(const GetFunctionIRRequest& request, uint32_t message_id);
    void handle_get_function_ir_graph(const GetFunctionIRGraphRequest& request, uint32_t message_id);
    void handle_save_bytecode(const char* data, size_t size);
    void handle_get_bytecode(const GetBytecodeRequest& request, uint32_t message_id);

    // Méthode pour envoyer une réponse
    bool send_response(uint32_t message_id, const void* response_data, size_t response_size);

    // Initialisation des routes
    void initialize_routes();

    // Gestion de la mémoire partagée
    SharedData* create_shared_memory();

public:
    IPCServer();
    ~IPCServer();

    bool initialize();
    void run();
    void stop();
};


#endif // CACHE_SERVER_H
