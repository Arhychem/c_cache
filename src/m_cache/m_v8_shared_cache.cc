
#include "m_v8_shared_cache.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>

namespace v8 {
namespace m_cache {

SharedCache::~SharedCache() {}

// Assure que le mmap est initialisé avant toute opération
void SharedCache::InitMmap() {
  int fd = open("/dev/zero", O_RDWR);
  if (fd == -1) {
    fprintf(stderr, "open cache file failed");
    return;
  }
  void* addr = mmap(nullptr, CACHE_FILE_SIZE, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE, fd, 0);
  if (addr == MAP_FAILED){ fprintf(stderr, "mmap failed");
    close(fd);
    return;
  };
  close(fd);
  mmap_base_ = addr;
  // Initialisation si premier lancement
  auto* entries = reinterpret_cast<CacheEntryHeader*>(mmap_base_);
  if (entries[0].key[0] == '\0') {
    // On suppose que le champ key vide signifie "vierge"
    memset(mmap_base_, 0, sizeof(CacheEntryHeader) * CACHE_MAX_ENTRIES);
    next_offset_ = sizeof(CacheEntryHeader) * CACHE_MAX_ENTRIES;
    entry_count_ = 0;
  } else {
    // Recherche du prochain offset libre
    entry_count_ = 0;
    next_offset_ = sizeof(CacheEntryHeader) * CACHE_MAX_ENTRIES;
    for (int i = 0; i < CACHE_MAX_ENTRIES; ++i) {
      if (entries[i].key[0] != '\0') {
        ++entry_count_;
        uint32_t end = entries[i].offset + entries[i].length;
        if (end > next_offset_) next_offset_ = end;
      }
    }
  }
}

CacheEntryHeader* SharedCache::EntryAt(uint32_t i) const {
  return reinterpret_cast<CacheEntryHeader*>(
      reinterpret_cast<uint8_t*>(mmap_base_) + i * sizeof(CacheEntryHeader));
}

// Recherche une entrée par clé
// Retourne -1 si non trouvé
// Retourne l'index de l'entrée si trouvée
int SharedCache::FindEntry(const std::string& key) const {
  auto* entries = reinterpret_cast<CacheEntryHeader*>(mmap_base_);
  for (int i = 0; i < CACHE_MAX_ENTRIES; ++i) {
    if (entries[i].key[0] != '\0' &&
        strncmp(entries[i].key, key.c_str(), sizeof(entries[i].key)) == 0) {
      return i;
    }
  }
  return -1;
}

// Recherche une entrée libre
// Retourne -1 si aucune entrée libre
// Retourne l'index de la première entrée libre trouvée
int SharedCache::FindFreeEntry() const {
  auto* entries = reinterpret_cast<CacheEntryHeader*>(mmap_base_);
  for (int i = 0; i < CACHE_MAX_ENTRIES; ++i) {
    if (entries[i].key[0] == '\0') return i;
  }
  return -1;
}

void SharedCache::Put(const std::string& key, const uint8_t* data,
                      uint32_t length) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!mmap_base_) {
    fprintf(stderr, "SharedCache not initialized");
    return;
  };
  int idx = FindEntry(key);
  if (idx == -1) {
    idx = FindFreeEntry();
    if (idx == -1) {
      fprintf(stderr, "Cache full");
      return;
    }
    ++entry_count_;
  }
  CacheEntryHeader* entry = EntryAt(idx);
  strncpy(entry->key, key.c_str(), sizeof(entry->key));
  entry->length = length;
  entry->offset = next_offset_;
  memcpy(reinterpret_cast<uint8_t*>(mmap_base_) + entry->offset, data, length);
  next_offset_ += length;
  msync(mmap_base_, CACHE_FILE_SIZE, MS_SYNC);
}

bool SharedCache::Get(const std::string& key, const uint8_t** data,
                      uint32_t& length) const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!mmap_base_) {
    fprintf(stderr, "SharedCache not initialized");
    return false;
  }
  int idx = FindEntry(key);
  if (idx == -1) return false;
  CacheEntryHeader* entry = EntryAt(idx);
  *data = reinterpret_cast<const uint8_t*>(mmap_base_) + entry->offset;
  length = entry->length;
  return true;
}

void SharedCache::Remove(const std::string& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!mmap_base_) {
    fprintf(stderr, "SharedCache not initialized");
    return;
  }
  int idx = FindEntry(key);
  if (idx == -1) return;
  CacheEntryHeader* entry = EntryAt(idx);
  if (entry->length > 0) {
    memset(reinterpret_cast<uint8_t*>(mmap_base_) + entry->offset, 0,
           entry->length);
  }
  entry->key[0] = '\0';
  entry->length = 0;
  entry->offset = 0;
  --entry_count_;
  // msync(mmap_base_, CACHE_FILE_SIZE, MS_SYNC);
}

void SharedCache::Clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!mmap_base_) return;
  memset(mmap_base_, 0, sizeof(CacheEntryHeader) * CACHE_MAX_ENTRIES);
  entry_count_ = 0;
  next_offset_ = sizeof(CacheEntryHeader) * CACHE_MAX_ENTRIES;
  msync(mmap_base_, CACHE_FILE_SIZE, MS_SYNC);
}

}  // namespace m_cache
}  // namespace v8
