#pragma once
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstddef>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

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