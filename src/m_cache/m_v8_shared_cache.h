#ifndef M_V8_SHARED_CACHE_H_
#define M_V8_SHARED_CACHE_H_

#include <mutex>
#include <string>

#define CACHE_FILE_PATH "/tmp/v8_code_cache"
#define CACHE_FILE_SIZE (1024 * 1024 * 100)  // 100 Mo
#define CACHE_MAX_ENTRIES 1024  // Nombre maximum d'entrées dans le cache

namespace v8 {
namespace m_cache {

enum CacheType { MAGLEV, TURBOFAN };
struct CacheEntryHeader {
  char function_name[256]; //Hash of the function name
  char key[256]; // Hash of the section source code (e.g., a loop in the function body)
  uint32_t length;
  uint32_t offset;
  CacheType type;
};

class SharedCache {
 public:
  // Accès singleton
  static SharedCache& Instance() {
    static SharedCache* instance = new SharedCache();
    instance->EnsureInitialized();
    return *instance;
  }

  // Initialise le mmap (à appeler avant toute opération)

  // Ajoute une entrée dans le cache (clé, buffer, taille)
  void Put(const std::string& key, const uint8_t* data, uint32_t length);

  // Récupère une entrée du cache (remplit buffer et length si trouvé)
  // Retourne true si trouvé, false sinon
  bool Get(const std::string& key, const uint8_t** data,
           uint32_t& length) const;

  // Supprime une entrée du cache
  void Remove(const std::string& key);

  // Vide tout le cache
  void Clear();

  CacheEntryHeader* EntryAt(uint32_t i) const;
  int FindEntry(const std::string& key) const;

 private:
  void EnsureInitialized() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) {
      InitMmap();
      initialized_ = true;
    }
  }

  void InitMmap();
  bool initialized_ = false;
  SharedCache() = default;
  ~SharedCache();
  SharedCache(const SharedCache&) = delete;
  SharedCache& operator=(const SharedCache&) = delete;
  mutable std::mutex mutex_;
  void* mmap_base_ = nullptr;
  uint32_t next_offset_ = 0;
  uint32_t entry_count_ = 0;
  // Pas de std::map : tout est dans le mmap
  int FindFreeEntry() const;
};

}  // namespace m_cache
}  // namespace v8

#endif  // V8_SRC_M_CACHE_M_SHARED_CACHE_H_
