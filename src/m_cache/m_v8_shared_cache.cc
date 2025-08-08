#include "m_v8_shared_cache.h"
#include <iostream>
#include <cstdio>
#include <cerrno>
#include <algorithm>

namespace m_cache {

SharedCache::SharedCache() = default;

SharedCache::~SharedCache() {
    if (mmap_base_ != nullptr && mmap_base_ != MAP_FAILED) {
        msync(mmap_base_, mmap_size_, MS_SYNC);
        munmap(mmap_base_, mmap_size_);
    }
    if (fd_ != -1) {
        close(fd_);
    }
}

// CHANGEMENT : Ajouter const à la signature
void SharedCache::EnsureInitialized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) {
        if (InitMmap()) {
            initialized_ = true;
        } else {
            fprintf(stderr, "Failed to initialize SharedCache\n");
        }
    }
}

// CHANGEMENT : Ajouter const à la signature
bool SharedCache::InitMmap() const {
    fd_ = open(CACHE_FILE_PATH, O_RDWR | O_CREAT, 0666);
    if (fd_ == -1) {
        fprintf(stderr, "Failed to open cache file: %s\n", strerror(errno));
        return false;
    }

    if (ftruncate(fd_, CACHE_FILE_SIZE) == -1) {
        fprintf(stderr, "Failed to truncate cache file: %s\n", strerror(errno));
        close(fd_);
        fd_ = -1;
        return false;
    }

    mmap_base_ = mmap(nullptr, CACHE_FILE_SIZE, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd_, 0);
    if (mmap_base_ == MAP_FAILED) {
        fprintf(stderr, "Failed to mmap cache file: %s\n", strerror(errno));
        close(fd_);
        fd_ = -1;
        mmap_base_ = nullptr;
        return false;
    }

    mmap_size_ = CACHE_FILE_SIZE;

    CacheHeader* header = GetHeader();
    if (header->magic_number != CACHE_MAGIC || header->version != CACHE_VERSION) {
        InitializeCache();
    }

    return true;
}

// CHANGEMENT : Ajouter const à la signature
void SharedCache::InitializeCache() const {
    CacheHeader* header = GetHeader();
    header->magic_number = CACHE_MAGIC;
    header->version = CACHE_VERSION;
    header->entry_count = 0;
    header->next_offset = sizeof(CacheHeader) + sizeof(CacheEntryHeader) * CACHE_MAX_ENTRIES;

    CacheEntryHeader* entries = GetEntries();
    memset(entries, 0, sizeof(CacheEntryHeader) * CACHE_MAX_ENTRIES);

    msync(mmap_base_, mmap_size_, MS_SYNC);
}

CacheHeader* SharedCache::GetHeader() const {
    return reinterpret_cast<CacheHeader*>(mmap_base_);
}

CacheEntryHeader* SharedCache::GetEntries() const {
    return reinterpret_cast<CacheEntryHeader*>(
        static_cast<uint8_t*>(mmap_base_) + sizeof(CacheHeader));
}

CacheEntryHeader* SharedCache::EntryAt(uint32_t index) const {
    if (index >= CACHE_MAX_ENTRIES) return nullptr;
    return &GetEntries()[index];
}

uint8_t* SharedCache::GetDataArea() const {
    return static_cast<uint8_t*>(mmap_base_) + sizeof(CacheHeader) +
           sizeof(CacheEntryHeader) * CACHE_MAX_ENTRIES;
}

int SharedCache::FindEntry(const std::string& key) const {
    CacheEntryHeader* entries = GetEntries();
    for (int i = 0; i < CACHE_MAX_ENTRIES; ++i) {
        if (entries[i].is_used &&
            strncmp(entries[i].key, key.c_str(), sizeof(entries[i].key) - 1) == 0) {
            return i;
        }
    }
    return -1;
}

int SharedCache::FindFreeEntry() const {
    CacheEntryHeader* entries = GetEntries();
    for (int i = 0; i < CACHE_MAX_ENTRIES; ++i) {
        if (!entries[i].is_used) {
            return i;
        }
    }
    return -1;
}

uint32_t SharedCache::CalculateChecksum(const uint8_t* data, uint32_t length) const {
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < length; ++i) {
        checksum = ((checksum << 5) + checksum) + data[i];
    }
    return checksum;
}

bool SharedCache::Put(const std::string& key, const uint8_t* data, uint32_t length) {
    EnsureInitialized();
    if (!initialized_ || !data || length == 0) return false;

    std::lock_guard<std::mutex> lock(mutex_);

    CacheHeader* header = GetHeader();

    uint32_t available_space = CACHE_FILE_SIZE - header->next_offset;
    if (length > available_space) {
        if (!CompactCache()) {
            fprintf(stderr, "Cache full, cannot add entry\n");
            return false;
        }
        available_space = CACHE_FILE_SIZE - header->next_offset;
        if (length > available_space) {
            fprintf(stderr, "Cache full after compaction\n");
            return false;
        }
    }

    int idx = FindEntry(key);
    if (idx == -1) {
        idx = FindFreeEntry();
        if (idx == -1) {
            fprintf(stderr, "No free entries available\n");
            return false;
        }
        header->entry_count++;
    }

    CacheEntryHeader* entry = EntryAt(idx);

    strncpy(entry->key, key.c_str(), sizeof(entry->key) - 1);
    entry->key[sizeof(entry->key) - 1] = '\0';
    entry->length = length;
    entry->offset = header->next_offset;
    entry->is_used = true;
    entry->checksum = CalculateChecksum(data, length);

    uint8_t* dest = GetDataArea() + (entry->offset - (sizeof(CacheHeader) +
                    sizeof(CacheEntryHeader) * CACHE_MAX_ENTRIES));
    memcpy(dest, data, length);

    header->next_offset += length;

    msync(mmap_base_, mmap_size_, MS_SYNC);

    return true;
}

bool SharedCache::Get(const std::string& key, const uint8_t** data, uint32_t& length) const {
    EnsureInitialized();
    if (!initialized_) return false;

    std::lock_guard<std::mutex> lock(mutex_);

    int idx = FindEntry(key);
    if (idx == -1) return false;

    CacheEntryHeader* entry = EntryAt(idx);
    if (!entry || !entry->is_used) return false;

    uint8_t* data_ptr = GetDataArea() + (entry->offset - (sizeof(CacheHeader) +
                       sizeof(CacheEntryHeader) * CACHE_MAX_ENTRIES));

    uint32_t calculated_checksum = CalculateChecksum(data_ptr, entry->length);
    if (calculated_checksum != entry->checksum) {
        fprintf(stderr, "Data corruption detected for key: %s\n", key.c_str());
        return false;
    }

    *data = data_ptr;
    length = entry->length;
    return true;
}

bool SharedCache::Remove(const std::string& key) {
    EnsureInitialized();
    if (!initialized_) return false;

    std::lock_guard<std::mutex> lock(mutex_);

    int idx = FindEntry(key);
    if (idx == -1) return false;

    CacheEntryHeader* entry = EntryAt(idx);
    entry->is_used = false;
    memset(entry->key, 0, sizeof(entry->key));
    entry->length = 0;
    entry->offset = 0;
    entry->checksum = 0;

    CacheHeader* header = GetHeader();
    header->entry_count--;

    msync(mmap_base_, mmap_size_, MS_SYNC);

    return true;
}

void SharedCache::Clear() {
    EnsureInitialized();
    if (!initialized_) return;

    std::lock_guard<std::mutex> lock(mutex_);
    InitializeCache();
}

// CHANGEMENT : Ajouter const à la signature
bool SharedCache::CompactCache() const {
    CacheHeader* header = GetHeader();
    CacheEntryHeader* entries = GetEntries();

    uint32_t write_offset = sizeof(CacheHeader) + sizeof(CacheEntryHeader) * CACHE_MAX_ENTRIES;
    uint8_t* data_area = GetDataArea();

    for (int i = 0; i < CACHE_MAX_ENTRIES; ++i) {
        if (entries[i].is_used && entries[i].length > 0) {
            uint32_t old_offset = entries[i].offset;
            uint32_t data_offset = old_offset - (sizeof(CacheHeader) +
                                  sizeof(CacheEntryHeader) * CACHE_MAX_ENTRIES);

            if (write_offset != old_offset) {
                memmove(data_area + (write_offset - (sizeof(CacheHeader) +
                       sizeof(CacheEntryHeader) * CACHE_MAX_ENTRIES)),
                       data_area + data_offset, entries[i].length);
                entries[i].offset = write_offset;
            }

            write_offset += entries[i].length;
        }
    }

    header->next_offset = write_offset;
    return true;
}

uint32_t SharedCache::GetEntryCount() const {
    EnsureInitialized();
    if (!initialized_) return 0;

    std::lock_guard<std::mutex> lock(mutex_);
    return GetHeader()->entry_count;
}

uint32_t SharedCache::GetUsedSpace() const {
    EnsureInitialized();
    if (!initialized_) return 0;

    std::lock_guard<std::mutex> lock(mutex_);
    CacheHeader* header = GetHeader();
    return header->next_offset;
}

uint32_t SharedCache::GetFreeSpace() const {
    EnsureInitialized();
    if (!initialized_) return 0;

    std::lock_guard<std::mutex> lock(mutex_);
    return CACHE_FILE_SIZE - GetUsedSpace();
}

bool SharedCache::IsValid() const {
    EnsureInitialized();
    if (!initialized_) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    CacheHeader* header = GetHeader();
    return header->magic_number == CACHE_MAGIC && header->version == CACHE_VERSION;
}

} // namespace m_cache
