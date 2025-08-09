#include "client_test.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>

bool running = true;

void signal_handler(int) {
    std::cout << "\nArrêt du client..." << std::endl;
    running = false;
}

int main() {
    // Gérer l'arrêt propre avec Ctrl+C
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::cout << "=== CLIENT DE TEST IPC ===" << std::endl;

    IPCClient client;

    if (!client.connect()) {
        std::cerr << "Erreur: impossible de se connecter au serveur" << std::endl;
        std::cerr << "Assurez-vous que le serveur est démarré" << std::endl;
        return 1;
    }

    std::cout << "Client connecté avec succès!" << std::endl;
    std::cout << "Exécution des tests..." << std::endl;

    // Exécuter tous les tests
    bool all_tests_passed = true;

    // Test création utilisateur
    if (!client.test_create_user()) {
        std::cerr << "Échec du test de création d'utilisateur" << std::endl;
        all_tests_passed = false;
    }

    // Test récupération utilisateur
    if (!client.test_get_user()) {
        std::cerr << "Échec du test de récupération d'utilisateur" << std::endl;
        all_tests_passed = false;
    }

    // Test suppression utilisateur
    if (!client.test_delete_user()) {
        std::cerr << "Échec du test de suppression d'utilisateur" << std::endl;
        all_tests_passed = false;
    }

    // Test ajout fonction IR
    if (!client.test_add_function_ir()) {
        std::cerr << "Échec du test d'ajout de fonction IR" << std::endl;
        all_tests_passed = false;
    }

    // Test récupération fonction IR
    if (!client.test_get_function_ir()) {
        std::cerr << "Échec du test de récupération de fonction IR" << std::endl;
        all_tests_passed = false;
    }

    // Afficher le résultat
    std::cout << "\n=== RÉSULTATS DES TESTS ===" << std::endl;
    if (all_tests_passed) {
        std::cout << "✓ Tous les tests ont réussi!" << std::endl;
    } else {
        std::cout << "✗ Certains tests ont échoué" << std::endl;
    }

    std::cout << "\nAppuyez sur Ctrl+C pour quitter ou attendez..." << std::endl;

    // Maintenir le client en vie pour permettre d'autres tests
    while (running) {
        sleep(1);
    }

    client.disconnect();
    std::cout << "Client fermé." << std::endl;

    return all_tests_passed ? 0 : 1;
}
