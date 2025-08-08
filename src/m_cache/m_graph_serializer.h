#ifndef M_GRAPH_SERIALIZER_H_
#define M_GRAPH_SERIALIZER_H_
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

namespace v8 {
namespace internal {
namespace compiler {
// Sérialisation personnalisée (à écrire pour chaque champ du graphe)
struct SerializeNode {
  SerializeNode() = default;
  uint32_t id;
  // uint32_t bit_field;   pas nécessaire vu qu'il est déteminé par id
  // uint32_t mark;
  //------- Sérialisation de l'opérateur
  std::string mnemonic;  // Nom de l'opérateur
  // TODO: Remplacer const char* par un uint8_t pour la sérialisation
  uint16_t opcode;
  uint32_t value_in_;
  uint32_t effect_in_;
  uint32_t control_in_;
  uint32_t value_out_;
  uint8_t effect_out_;
  uint32_t control_out_;
  // Sérialisation du champ properties aka base::Flags<Property, uint8_t>
  // uint8_t flag_type;   Enum Property
  uint8_t mask;  // aka  BitfieldStorageT _mask; we use uint8_t since BitfieldT
                 // is uint8_t
  //-------  Fin de la sérialisation de l'opérateur
  int input_count;
  bool has_extensible_inputs;
  // Ids des Node inputs (NodeIds)
  std::vector<uint32_t> inputs;
};
// Fonction pour afficher le contenu de la map
inline void print_map(const std::unordered_map<uint32_t, SerializeNode>& m,
                      const std::string& title) {
  std::cout << "--- " << title << " ---" << std::endl;
  if (m.empty()) {
    std::cout << "La map est vide." << std::endl;
    return;
  }
  for (const auto& pair : m) {
    bool cycle = false;
    std::cout << "Key: " << pair.first << ", Value: { " << pair.second.id
              << ", \"" << pair.second.mnemonic << "\", inputs: [";
    for (size_t i = 0; i < pair.second.inputs.size(); ++i) {
      std::cout << pair.second.inputs[i];
      if (pair.second.inputs[i] == pair.first) {
        cycle = true;
      }
      if (i + 1 < pair.second.inputs.size()) std::cout << ", ";
    }
    std::cout << "] }" << std::endl;
    if (cycle) {
      std::cout << "  --> Cycle detected in inputs for node id: " << pair.first
                << std::endl;
    }
  }
  std::cout << std::endl;
}
struct SerializeTFGraph {
  SerializeTFGraph() = default;
  uint32_t node_start_id;
  uint32_t node_end_id;
  // uint32_t node_mark_max;
  size_t next_node_id;
  bool has_simd;
  // Sérialisation des décorateurs
  // std::vector<const char*> decorators;  // Nom des décorateurs
  // Sérialisation des nœuds
  // Chaque nœud est sérialisé avec SerializedNode
  std::unordered_map<uint32_t, SerializeNode> graph_nodes;
};
// ...existing code...

// Nouvelles fonctions de sérialisation
class GraphSerializer {
 public:
  // Sérialiser SerializeTFGraph vers un buffer de uint8_t
  static std::vector<uint8_t> serialize_to_bytes(const SerializeTFGraph& graph);

  // Désérialiser depuis un buffer de uint8_t vers SerializeTFGraph
  static SerializeTFGraph deserialize_from_bytes(const uint8_t* data,
                                                 size_t data_size);

 private:
  // Helpers pour la sérialisation
  static void write_uint32(std::vector<uint8_t>& buffer, uint32_t value);
  static void write_string(std::vector<uint8_t>& buffer,
                           const std::string& str);
  static void write_serialize_node(std::vector<uint8_t>& buffer,
                                   const SerializeNode& node);

  // Helpers pour la désérialisation
  static uint32_t read_uint32(const uint8_t*& data, size_t& remaining);
  static std::string read_string(const uint8_t*& data, size_t& remaining);
  static SerializeNode read_serialize_node(const uint8_t*& data,
                                           size_t& remaining);
};

}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif  // M_GRAPH_SERIALIZER_H_
