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

#ifdef _WIN32
    #define OS_EXTRA_CHECK_ALIGNMENT(alignment) do{                                 \
        SYSTEM_INFO sys_info;                                                       \
        GetSystemInfo(&sys_info);                                                   \
        assert((alignment) >= (sys_info.dwAllocationGranularity)                    \
                && "Alignment must be >= system allocation granularity on Windows");\
        printf(                                                                     \
            "Alignment is %zu, and System allocation granularity is %zu",           \
            static_cast<std::size_t>(alignment),                                    \
            static_cast<std::size_t>(sys_info.dwAllocationGranularity)              \
        );                                                                          \
    }while(0)
#else
    #include <unistd.h>
    #define OS_EXTRA_CHECK_ALIGNMENT(alignment) do {                                \
        std::size_t page_size = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));    \
        assert(                                                                     \
            (alignment) >= page_size &&                                             \
            "Alignment must be >= system page size on POSIX systems"                \
        );                                                                          \
        printf("Alignment = %zu, System page size = %zu\n",                         \
            static_cast<size_t>(alignment), page_size);                             \
    }while(0)
#endif 


#ifdef NDEBUG
    #define CHECK_ALIGNMENT(alignment) ((void)0)
#else /* 检查是否为2的幂且大于0 */ 
    #define CHECK_ALIGNMENT(alignment) do {                                         \
        assert(((alignment) > 0) && (((alignment) & ((alignment) - 1)) == 0)        \
            && "Alignment must be a power of 2 and greater than 0");                \
        OS_EXTRA_CHECK_ALIGNMENT(alignment);                                        \
    } while(0)
#endif


namespace OSAllocator {
    struct AllocHeader{
        void * base_ptr;
        size_t total_size;
    };
    /*
     * @function: 封装 OS 底层的内存分配接口
     * @param: size 字节数 
     * @param: alignment 对齐值, 默认是 alignof(std::max_align_t)
     * @note: Release 下不做任何检查, 
     * @note: Debug 要求: alignment 是 2的幂次 && 大于零
     * @note:    Windows 下不能超过 dwAllocationGranularity, 一般是 64KB (VirtualAlloc)
     * @note:    Linux   下不能超过要一个 page_size, 一般是 4KB (mmap)
    */
    void* OS_Alloc(size_t size, size_t alignment = alignof(std::max_align_t));
    void* OS_AllocBigPage(size_t size, size_t alignment = alignof(std::max_align_t));
    void OS_Free(void* ptr, size_t size = 0);
    size_t GetPageSize();

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