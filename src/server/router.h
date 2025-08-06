#ifndef IPC_ROUTER_H
#define IPC_ROUTER_H

#include "common.h"

class IPCRouter
{
private:
    std::unordered_map<uint32_t, std::function<void(const char*, size_t)>> routes;

public:
    // Enregistrer une route avec sa fonction de traitement
    template<typename RequestType>
    void register_route(const std::string& route,
        std::function<void(const RequestType&)> handler)
    {
        uint32_t route_hash = hash_route(route);

        routes[route_hash] = [handler](const char* data, size_t size) {
            if (size >= sizeof(RequestType)) {
                const RequestType* request = reinterpret_cast<const RequestType*>(data);
                handler(*request);
            }
            else {
                printf("Erreur: taille de données insuffisante pour la route\n");
            }
            };

        printf("Route enregistrée: %s (hash: %u)\n", route.c_str(), route_hash);
    }

    // Nouvelle méthode pour tailles variables
    void register_variable_route(const std::string& route,
        std::function<void(const char*, size_t)> handler)
    {
        uint32_t route_hash = hash_route(route);
        routes[route_hash] = handler;
        printf("Route variable enregistrée: %s (hash: %u)\n", route.c_str(), route_hash);
    }

    // Traiter un message reçu
    void dispatch_message(const IPCMessage* message)
    {
        auto it = routes.find(message->route_hash);
        if (it != routes.end()) {
            it->second(message->payload, message->payload_size);
        }
        else {
            handle_unknown_route(message);
        }
    }

private:
    void handle_unknown_route(const IPCMessage* message)
    {
        printf("Route inconnue: hash %u\n", message->route_hash);
    }
};

#endif // IPC_ROUTER_H
