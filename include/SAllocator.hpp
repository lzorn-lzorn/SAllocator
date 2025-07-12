// modern_malloc.hpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>
#include <bit>
#include <mutex>
#include <memory_resource>
#include <atomic>
#include <new>
#include <cassert>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace Stellatus {

constexpr size_t ALIGNMENT = alignof(std::max_align_t);
constexpr size_t MAX_FAST_SIZE = 1024; // 提升 fastbin 支持容量到 1024 bytes
constexpr size_t NUM_FAST_BINS = MAX_FAST_SIZE / ALIGNMENT;
constexpr size_t MMAP_THRESHOLD = 1ULL << 30; // 超过 1GB 使用大块分配
constexpr size_t MAX_ALLOC_SIZE = 8ULL << 30; // 支持最多分配 8GB

inline size_t align_up(size_t size, size_t align = ALIGNMENT) {
    return (size + align - 1) & ~(align - 1);
}

inline void* os_alloc(size_t size) {
#if defined(_WIN32)
    void* ptr = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (ptr) std::memset(ptr, 0, size); // 强制触发物理页分配
    return ptr;
#else
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr != MAP_FAILED) std::memset(ptr, 0, size); // 强制触发物理页分配
    return (ptr == MAP_FAILED) ? nullptr : ptr;
#endif
}

inline void os_free(void* ptr, size_t size) {
#if defined(_WIN32)
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    munmap(ptr, size);
#endif
}

struct Chunk {
    size_t size;
    Chunk* next;

    void* data() { return reinterpret_cast<void*>(this + 1); }
    static Chunk* from_data(void* ptr) {
        return reinterpret_cast<Chunk*>(ptr) - 1;
    }
};

class Arena {
public:
    Arena() = default;

    void* allocate(size_t size) {
        std::scoped_lock lock(mtx);
        if (size > MAX_ALLOC_SIZE) throw std::bad_alloc{};

        if (size <= MAX_FAST_SIZE) {
            size_t idx = size_to_index(size);
            if (fastbins[idx]) {
                Chunk* chunk = fastbins[idx];
                fastbins[idx] = chunk->next;
                return chunk->data();
            }
        }

        size_t total_size = align_up(size + sizeof(Chunk));
        void* raw = nullptr;

        if (size >= MMAP_THRESHOLD) {
            raw = os_alloc(total_size);
            if (!raw) throw std::bad_alloc{};
        } else {
            raw = std::malloc(total_size);
            if (!raw) throw std::bad_alloc{};
            std::memset(raw, 0, total_size); // 强制触发页表分配
        }

        Chunk* chunk = reinterpret_cast<Chunk*>(raw);
        chunk->size = size;
        return chunk->data();
    }

    void deallocate(void* ptr, size_t size) {
        std::scoped_lock lock(mtx);
        if (!ptr) return;

        Chunk* chunk = Chunk::from_data(ptr);

        if (size <= MAX_FAST_SIZE) {
            size_t idx = size_to_index(size);
            chunk->next = fastbins[idx];
            fastbins[idx] = chunk;
        } else {
            size_t total_size = align_up(size + sizeof(Chunk));
            if (size >= MMAP_THRESHOLD) {
                os_free(chunk, total_size);
            } else {
                std::free(chunk);
            }
        }
    }

private:
    std::mutex mtx;
    std::array<Chunk*, NUM_FAST_BINS> fastbins{};

    static size_t size_to_index(size_t size) {
        return (align_up(size) / ALIGNMENT) - 1;
    }
};

inline thread_local Arena tls_arena;

template <typename T>
class SAllocator {
public:
    using value_type = T;

    SAllocator() noexcept = default;
    template <typename U>
    SAllocator(const SAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        return static_cast<T*>(tls_arena.allocate(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t n) {
        tls_arena.deallocate(p, n * sizeof(T));
    }
};

// STL 的要求: https://en.cppreference.com/w/cpp/named_req/Allocator
template <typename T, typename U>
bool operator==(const SAllocator<T>&, const SAllocator<U>&) noexcept { return true; }
template <typename T, typename U>
bool operator!=(const SAllocator<T>&, const SAllocator<U>&) noexcept { return false; }

} 