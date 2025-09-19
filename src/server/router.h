#ifndef IPC_ROUTER_H
#define IPC_ROUTER_H

#include "common.h"

class IPCRouter
{
private:
    std::unordered_map<std::string, std::function<void(const char*, size_t)>> routes;

public:
    // Enregistrer une route avec sa fonction de traitement
    template<typename RequestType>
    void register_route(const std::string& route,
        std::function<void(const RequestType&)> handler)
    {
        std::string route_hash = hash_route(route);

        routes[route_hash] = [handler](const char* data, size_t size) {
            if (size >= sizeof(RequestType)) {
                const RequestType* request = reinterpret_cast<const RequestType*>(data);
                handler(*request);
            }
            else {
                printf("Erreur: taille de données insuffisante pour la route\n");
            }
            };

        printf("Route enregistrée: %s (hash: %s)\n", route.c_str(), route_hash.c_str());
    }

    // Nouvelle méthode pour tailles variables
    void register_variable_route(const std::string& route,
        std::function<void(const char*, size_t)> handler)
    {
        std::string route_hash = hash_route(route);
        routes[route_hash] = handler;
        printf("Route variable enregistrée: %s (hash: %s)\n", route.c_str(), route_hash.c_str());
    }

    // Traiter un message reçu
    void dispatch_message(const IPCMessage* message)
    {
        std::string route_hash(message->route_hash);
        auto it = routes.find(route_hash);
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
        printf("Route inconnue: hash %s\n", message->route_hash);
    }
};

#endif // IPC_ROUTER_H
