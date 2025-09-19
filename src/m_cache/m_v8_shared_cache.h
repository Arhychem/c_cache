#ifndef M_V8_SHARED_CACHE_H_
#define M_V8_SHARED_CACHE_H_

#include <string>
#include <mutex>
#include <cstdint>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#define CACHE_FILE_PATH "/tmp/v8_code_cache"
#define CACHE_FILE_SIZE (1024 * 1024 * 100) // 100 Mo
#define CACHE_MAX_ENTRIES 1024

namespace m_cache {

    struct CacheEntryHeader
    {
        char function_name[256];    // Hash of the function name
        char key[256];             // Hash of the section source code
        uint32_t length;           // Taille des données
        uint32_t offset;           // Offset dans le fichier mmap
        bool is_used;              // Indique si l'entrée est utilisée
        uint32_t checksum;         // Checksum pour vérifier l'intégrité
    };

    struct CacheHeader
    {
        uint32_t magic_number;     // Pour vérifier la validité du cache
        uint32_t version;          // Version du format de cache
        uint32_t entry_count;      // Nombre d'entrées utilisées
        uint32_t next_offset;      // Prochain offset libre
        uint8_t padding[16];       // Padding pour alignement
    };

    class SharedCache
    {
    public:
        static const uint32_t CACHE_MAGIC = 0xC4C4E001;
        static const uint32_t CACHE_VERSION = 1;

        static SharedCache& Instance()
        {
            static SharedCache instance;
            return instance;
        }

        bool Put(const std::string& key, const uint8_t* data, uint32_t length);
        bool Get(const std::string& key, const uint8_t** data, uint32_t& length) const;
        bool Remove(const std::string& key);
        void Clear();

        uint32_t GetEntryCount() const;
        uint32_t GetUsedSpace() const;
        uint32_t GetFreeSpace() const;
        bool IsValid() const;

    private:
        SharedCache();
        ~SharedCache();
        SharedCache(const SharedCache&) = delete;
        SharedCache& operator=(const SharedCache&) = delete;

        void EnsureInitialized() const;
        bool InitMmap() const;
        void InitializeCache() const;

        CacheHeader* GetHeader() const;
        CacheEntryHeader* GetEntries() const;
        CacheEntryHeader* EntryAt(uint32_t index) const;
        uint8_t* GetDataArea() const;

        int FindEntry(const std::string& key) const;
        int FindFreeEntry() const;
        uint32_t CalculateChecksum(const uint8_t* data, uint32_t length) const;
        bool CompactCache() const;

        mutable std::mutex mutex_;
        mutable void* mmap_base_ = nullptr;
        mutable size_t mmap_size_ = 0;
        mutable bool initialized_ = false;
        mutable int fd_ = -1;
    };

} // namespace m_cache

#endif // M_V8_SHARED_CACHE_H_
