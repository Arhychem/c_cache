#ifndef CLIENT_TEST_H
#define CLIENT_TEST_H

#include "../server/common.h"

class IPCClient
{
private:
    SharedData* shared_data;
    bool connected;

public:
    IPCClient();
    ~IPCClient();

    bool connect();
    void disconnect();

    // Méthodes de test pour les différentes fonctionnalités
    bool test_create_user();
    bool test_get_user();
    bool test_delete_user();
    bool test_add_function_ir();
    bool test_get_function_ir();

    // Méthodes utilitaires
    bool send_message(const void* message_data, size_t message_size, uint32_t route_hash);
    bool wait_for_response(void* response_buffer, size_t& response_size, uint32_t expected_message_id);

private:
    SharedData* open_shared_memory();
};

#endif // CLIENT_TEST_H
