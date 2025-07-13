#pragma once
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <stdexcept>
#include <new>
#include <bit>
#include <iostream>
#include <algorithm>
#if defined(_WIN32)
    #include <windows.h>
    #include <winnt.h>
    #include <sysinfoapi.h>
    #include <corecrt_malloc.h>
    #include <errhandlingapi.h>
    #include <memoryapi.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
    #ifdef __linux__
        #include <sys/sysinfo.h>
        #include <errno.h>
    #endif
#endif


#ifdef _WIN32
    #define MEM_NATIVE_PROT_READ_ONLY         PAGE_READONLY
    #define MEM_NATIVE_PROT_READ_WRITE        PAGE_READWRITE
    #define MEM_NATIVE_PROT_READ_EXECUTE      PAGE_EXECUTE_READ
    #define MEM_NATIVE_PROT_READ_WRITE_EXECUTE PAGE_EXECUTE_READWRITE
    #define MEM_NATIVE_PROT_NO_ACCESS         PAGE_NOACCESS
#else
    #include <sys/mman.h>  // for PROT_READ/WRITE/EXEC/NONE

    #define MEM_NATIVE_PROT_READ_ONLY         PROT_READ
    #define MEM_NATIVE_PROT_READ_WRITE        (PROT_READ | PROT_WRITE)
    #define MEM_NATIVE_PROT_READ_EXECUTE      (PROT_READ | PROT_EXEC)
    #define MEM_NATIVE_PROT_READ_WRITE_EXECUTE (PROT_READ | PROT_WRITE | PROT_EXEC)
    #define MEM_NATIVE_PROT_NO_ACCESS         PROT_NONE
#endif

#ifdef NDEBUG
    #define CHECK_ALIGNMENT(x) (true)
#else
    #define CHECK_ALIGNMENT(x)                                      \
        ([](std::size_t __chk_align) constexpr {                    \
            static_assert(__chk_align > 0, "Alignment must be > 0");\
            static_assert((__chk_align & (__chk_align - 1)) == 0,   \
                          "Alignment must be a power of 2");        \
            return true;                                            \
        }(x))
#endif


namespace OSAllocator {
    // 内存保护标志
    enum class Protection {
        READ_ONLY,
        READ_WRITE,
        READ_EXECUTE,
        READ_WRITE_EXECUTE,
        NO_ACCESS
    };

    struct AllocHeader{
        void * base_ptr;
        size_t total_size;
    };

    // 原有基础API
    void* OS_Alloc(size_t size, size_t alignment = 16);
    void OS_Free(void* ptr, size_t size = 0);
    size_t GetPageSize();
    
    // 新增高级功能API
    void* OS_AllocLargePages(size_t size); // 大页分配
    bool OS_SetProtection(void* ptr, size_t size, Protection prot); // 内存保护
    bool OS_LockMemory(void* ptr, size_t size); // 锁定内存(防止交换)
    bool OS_UnlockMemory(void* ptr, size_t size); // 解锁内存
}

// inline void* OS_Alloc(size_t size) {
//     #if defined(_WIN32)
//         void* ptr = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
//         if (ptr) std::memset(ptr, 0, size); // 强制触发物理页分配
//         return ptr;
//     #else
//         void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
//         if (ptr != MAP_FAILED) std::memset(ptr, 0, size); // 强制触发物理页分配
//         return (ptr == MAP_FAILED) ? nullptr : ptr;
//     #endif
// }
    
// inline void OS_Free(void* ptr, size_t size) {
// #if defined(_WIN32)
//     VirtualFree(ptr, 0, MEM_RELEASE);
// #else
//     munmap(ptr, size);
// #endif
// }