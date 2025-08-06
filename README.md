# Cache Server C++

Un serveur de cache haute performance utilisant la mémoire partagée et la communication inter-processus (IPC) pour le cache de code V8.

## Structure du projet

```
cache-server-cpp/
├── src/
│   ├── server/          # Code du serveur
│   │   ├── cache_server.cpp
│   │   ├── cache_server.h
│   │   ├── common.h
│   │   ├── router.h
│   │   └── server_main.cpp
│   ├── client/          # Code du client de test
│   │   ├── client_main.cpp
│   │   ├── client_test.cpp
│   │   └── client_test.h
│   └── m_cache/         # Module de cache V8
│       ├── m_v8_shared_cache.cc
│       ├── m_v8_shared_cache.h
│       └── picosha2.h
├── build/               # Fichiers de build temporaires
├── bin/                 # Exécutables compilés
├── .vscode/             # Configuration VS Code
├── CMakeLists.txt       # Configuration CMake
├── Makefile            # Configuration Make
├── build.sh            # Script de build
└── README.md           # Ce fichier
```

## Prérequis

- **Compilateur**: g++ avec support C++17
- **Build tools**: make et/ou CMake (3.10+)
- **Bibliothèques système**: pthread, rt (pour mmap/shm)
- **Système**: Linux (utilise mmap et sémaphores POSIX)

### Installation des dépendances (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake gdb
```

## Compilation

### Méthode 1: Script de build (recommandé)

```bash
# Compilation complète
./build.sh

# Options disponibles
./build.sh all      # Compile serveur et client
./build.sh server   # Compile uniquement le serveur
./build.sh client   # Compile uniquement le client
./build.sh debug    # Compile en mode debug
./build.sh clean    # Nettoie les fichiers de build
./build.sh cmake    # Utilise CMake au lieu de make
./build.sh help     # Affiche l'aide
```

### Méthode 2: Makefile

```bash
# Compilation complète
make all

# Compilation séparée
make server         # Serveur uniquement
make client         # Client uniquement
make debug          # Mode debug avec symboles

# Nettoyage
make clean          # Nettoie les fichiers de build
make clean-cache    # Nettoie les caches système

# Exécution directe
make run-server     # Compile et lance le serveur
make run-client     # Compile et lance le client

# Informations
make info           # Affiche la configuration
make help           # Affiche l'aide
```

### Méthode 3: CMake

```bash
# Configuration
mkdir -p build && cd build
cmake ..

# Compilation
cmake --build .

# Ou avec des targets spécifiques
cmake --build . --target cache_server
cmake --build . --target cache_client

# Nettoyage
cmake --build . --target clean
cmake --build . --target clean-cache
```

## Exécution

### Démarrage du serveur

```bash
# Depuis la racine du projet
./bin/cache_server

# Ou via make
make run-server
```

### Test avec le client

```bash
# Dans un autre terminal
./bin/cache_client

# Ou via make
make run-client
```

## Développement avec VS Code

Le projet inclut une configuration complète pour VS Code :

### Tâches disponibles (Ctrl+Shift+P → "Tasks: Run Task")

- **Build All**: Compilation complète (Ctrl+Shift+B)
- **Build Server**: Compilation du serveur uniquement
- **Build Client**: Compilation du client uniquement
- **Run Server**: Compilation et lancement du serveur
- **Run Client**: Compilation et lancement du client
- **Clean Build**: Nettoyage des fichiers de build
- **Clean Cache**: Nettoyage des caches système
- **Build Debug**: Compilation en mode debug

### Débogage (F5)

- **Debug Server**: Lance le serveur sous gdb
- **Debug Client**: Lance le client sous gdb
- **Attach to Server**: S'attache à un processus serveur existant

### Extensions recommandées

- **C/C++**: Support IntelliSense et débogage
- **CMake Tools**: Support CMake intégré
- **GitLens**: Historique git amélioré

## Architecture

### Serveur de cache

- **Communication IPC**: Utilise la mémoire partagée pour les échanges
- **Cache V8**: Stockage optimisé des données de compilation V8
- **Router**: Système de routage des requêtes
- **Gestion des signaux**: Arrêt propre avec Ctrl+C

### Client de test

- **Tests automatisés**: Validation du fonctionnement
- **Exemples d'usage**: Démonstration des appels API

### Cache partagé

- **Mémoire mappée**: Fichier de cache persistant de 100MB
- **Synchronisation**: Mutex et sémaphores pour l'accès concurrent
- **Hash des clés**: Identification unique des entrées

## API

### Messages supportés

- `CreateUserRequest`: Création d'utilisateur
- `GetUserRequest`: Récupération d'utilisateur
- `DeleteUserRequest`: Suppression d'utilisateur

### Exemple d'usage

```cpp
#include "client_test.h"

IPCClient client;
if (client.initialize()) {
    client.create_user("alice", "alice@example.com");
    client.get_user(12345);
    client.delete_user(98765);
}
```

## Dépannage

### Problèmes courants

1. **Erreur de compilation**: Vérifiez que g++ supporte C++17
2. **Erreur de linking**: Installez les bibliothèques pthread et rt
3. **Permission denied**: Assurez-vous que `/tmp` et `/dev/shm` sont accessibles
4. **Client ne peut pas se connecter**: Démarrez d'abord le serveur

### Nettoyage des ressources

```bash
# Nettoie tous les caches et ressources
make clean-cache

# Ou manuellement
rm -f /tmp/v8_code_cache
rm -f /dev/shm/ipc_router_shared
```

### Logs et débogage

```bash
# Compilation avec debug
make debug

# Exécution avec gdb
gdb ./bin/cache_server
gdb ./bin/cache_client

# Ou utiliser VS Code (F5)
```

## Contribution

1. Fork du projet
2. Création d'une branche feature
3. Commits avec messages descriptifs
4. Tests avant push
5. Pull request avec description

## Licence

Ce projet est sous licence MIT. Voir le fichier LICENSE pour plus de détails.

## Contact

Pour questions ou support, créez une issue sur le repository GitHub.
