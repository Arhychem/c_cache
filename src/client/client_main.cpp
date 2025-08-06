#include "client_test.h"
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
    IPCClient client;

    if (!client.initialize()) {
        fprintf(stderr, "Erreur: Impossible de se connecter au serveur\n");
        fprintf(stderr, "Assurez-vous que le serveur est démarré\n");
        return 1;
    }

    printf("=== CLIENT IPC CONNECTÉ ===\n\n");

    // Exemples d'envoi de messages
    printf("Envoi de requêtes de test...\n\n");

    // Créer un utilisateur
    client.create_user("alice_doe", "alice@example.com");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Récupérer un utilisateur
    client.get_user(12345);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Créer un autre utilisateur
    client.create_user("bob_smith", "bob@test.com");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Supprimer un utilisateur
    client.delete_user(98765);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Test de la nouvelle fonctionnalité avec tableau de bits
    printf("Test de la fonction AddFunctionIR...\n");

    // Créer un tableau de bits d'exemple
    uint8_t sample_bits[] = { 0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56 };
    uint32_t num_bits = 40; // 5 octets * 8 bits = 40 bits

    client.add_function_ir("SHA256_HASH_OF_FUNCTION_CODE", sample_bits, num_bits);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Test avec un tableau plus petit
    uint8_t small_bits[] = { 0xFF };
    client.add_function_ir("SMALL_FUNCTION_HASH", small_bits, 8);

    // Test de récupération d'IR
    std::vector<uint8_t> ir_bits;

    printf("Récupération IR pour fonction existante...\n");
    if (client.get_function_ir("EXISTING_FUNCTION", ir_bits)) {
        printf("IR récupéré avec succès! Premiers octets: ");
        for (size_t i = 0; i < std::min(size_t(8), ir_bits.size()); i++) {
            printf("%02X ", ir_bits[i]);
        }
        printf("\n");
    }

    printf("\nRécupération IR pour fonction inexistante...\n");
    if (!client.get_function_ir("NONEXISTENT_FUNCTION", ir_bits)) {
        printf("Fonction non trouvée (comme attendu)\n");
    }

    printf("\nToutes les requêtes ont été envoyées!\n");

    return 0;
}
