cmake_minimum_required(VERSION 3.10)
project(CacheServerCpp)

# Définir le standard C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options de compilation
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Répertoires d'inclusion
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server
    ${CMAKE_CURRENT_SOURCE_DIR}/src/client
    ${CMAKE_CURRENT_SOURCE_DIR}/src/m_cache
)

# Bibliothèques requises
find_package(Threads REQUIRED)

# Sources communes
set(COMMON_SOURCES
    src/m_cache/m_v8_shared_cache.cc
)

# Sources du serveur
set(SERVER_SOURCES
    src/server/cache_server.cpp
    src/server/server_main.cpp
    ${COMMON_SOURCES}
)

# Sources du client
set(CLIENT_SOURCES
    src/client/client_test.cpp
    src/client/client_main.cpp
    ${COMMON_SOURCES}
)

# Exécutable du serveur
add_executable(cache_server ${SERVER_SOURCES})
target_link_libraries(cache_server
    Threads::Threads
    rt  # Pour shm_open/mmap
)

# Exécutable du client
add_executable(cache_client ${CLIENT_SOURCES})
target_link_libraries(cache_client
    Threads::Threads
    rt  # Pour shm_open/mmap
)

# Dossier de sortie pour les exécutables
set_target_properties(cache_server cache_client
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation
install(TARGETS cache_server cache_client
    RUNTIME DESTINATION bin
)

# Target pour nettoyer les fichiers de cache
add_custom_target(clean-cache
    COMMAND rm -f /tmp/v8_code_cache
    COMMAND rm -f /dev/shm/ipc_router_shared
    COMMENT "Nettoyage des fichiers de cache et mémoire partagée"
)

# Target pour lancer le serveur
add_custom_target(run-server
    COMMAND ${CMAKE_BINARY_DIR}/bin/cache_server
    DEPENDS cache_server
    COMMENT "Lancement du serveur de cache"
)

# Target pour lancer le client
add_custom_target(run-client
    COMMAND ${CMAKE_BINARY_DIR}/bin/cache_client
    DEPENDS cache_client
    COMMENT "Lancement du client de test"
)
