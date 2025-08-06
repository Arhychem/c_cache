#!/bin/bash

# Script de build pour le projet Cache Server C++
# Usage: ./build.sh [option]
#   all     - Compile serveur et client (défaut)
#   server  - Compile uniquement le serveur
#   client  - Compile uniquement le client
#   clean   - Nettoie les fichiers de build
#   debug   - Compile en mode debug
#   cmake   - Utilise CMake au lieu de make

set -e  # Arrêter le script en cas d'erreur

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

# Couleurs pour l'affichage
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Fonction pour vérifier les dépendances
check_dependencies() {
    print_status "Vérification des dépendances..."

    if ! command -v g++ &> /dev/null; then
        print_error "g++ n'est pas installé"
        exit 1
    fi

    if ! command -v make &> /dev/null; then
        print_error "make n'est pas installé"
        exit 1
    fi

    print_success "Toutes les dépendances sont installées"
}

# Fonction pour nettoyer les caches
clean_cache() {
    print_status "Nettoyage des caches..."
    rm -f /tmp/v8_code_cache
    rm -f /dev/shm/ipc_router_shared
    print_success "Caches nettoyés"
}

# Fonction de build avec Make
build_with_make() {
    local target="${1:-all}"

    print_status "Compilation avec Make (target: $target)..."

    case $target in
        "clean")
            make clean
            clean_cache
            ;;
        "debug")
            make debug
            ;;
        *)
            make "$target"
            ;;
    esac

    if [ "$target" != "clean" ]; then
        print_success "Compilation terminée avec succès"

        if [ -f "bin/cache_server" ]; then
            print_status "Serveur: $(realpath bin/cache_server)"
        fi

        if [ -f "bin/cache_client" ]; then
            print_status "Client: $(realpath bin/cache_client)"
        fi
    fi
}

# Fonction de build avec CMake
build_with_cmake() {
    local target="${1:-all}"

    print_status "Compilation avec CMake..."

    # Créer le dossier de build
    mkdir -p build
    cd build

    if [ "$target" = "clean" ]; then
        rm -rf *
        cd ..
        clean_cache
        print_success "Build CMake nettoyé"
        return
    fi

    # Configuration CMake
    if [ "$target" = "debug" ]; then
        cmake -DCMAKE_BUILD_TYPE=Debug ..
    else
        cmake -DCMAKE_BUILD_TYPE=Release ..
    fi

    # Compilation
    if [ "$target" = "server" ]; then
        cmake --build . --target cache_server
    elif [ "$target" = "client" ]; then
        cmake --build . --target cache_client
    else
        cmake --build .
    fi

    cd ..

    print_success "Compilation CMake terminée avec succès"

    if [ -f "build/bin/cache_server" ]; then
        print_status "Serveur: $(realpath build/bin/cache_server)"
    fi

    if [ -f "build/bin/cache_client" ]; then
        print_status "Client: $(realpath build/bin/cache_client)"
    fi
}

# Affichage de l'aide
show_help() {
    echo "Usage: $0 [option]"
    echo ""
    echo "Options:"
    echo "  all     - Compile serveur et client (défaut)"
    echo "  server  - Compile uniquement le serveur"
    echo "  client  - Compile uniquement le client"
    echo "  clean   - Nettoie les fichiers de build"
    echo "  debug   - Compile en mode debug"
    echo "  cmake   - Utilise CMake au lieu de make"
    echo "  help    - Affiche cette aide"
    echo ""
    echo "Exemples:"
    echo "  $0          # Compile tout avec make"
    echo "  $0 server   # Compile uniquement le serveur"
    echo "  $0 cmake    # Compile tout avec CMake"
    echo "  $0 debug    # Compile en mode debug"
}

# Main
main() {
    local option="${1:-all}"
    local use_cmake=false

    case $option in
        "help"|"-h"|"--help")
            show_help
            exit 0
            ;;
        "cmake")
            use_cmake=true
            option="all"
            ;;
    esac

    print_status "=== Build Cache Server C++ ==="

    check_dependencies

    if [ "$use_cmake" = true ]; then
        build_with_cmake "$option"
    else
        build_with_make "$option"
    fi
}

# Exécution du script
main "$@"
