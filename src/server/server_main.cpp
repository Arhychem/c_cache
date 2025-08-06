#include "cache_server.h"
#include <signal.h>

IPCServer* server_instance = nullptr;

void signal_handler(int signal) {
    if (server_instance) {
        printf("\nArrêt du serveur...\n");
        server_instance->stop();
    }
}

int main() {
    IPCServer server;
    server_instance = &server;

    // Gérer l'arrêt propre avec Ctrl+C
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    server.run();

    return 0;
}
