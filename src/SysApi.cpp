
#include <corecrt_malloc.h>
#include <errhandlingapi.h>
#include <memoryapi.h>
#include <new>
#include <stdexcept>
#include <sysinfoapi.h>
#include <windows.h>
#include <winnt.h>
#include "../include/JAllocatorImpl/SysApi.h"

namespace {
    int GetNativeProtection(OSAllocator::Protection prot){
        using P = OSAllocator::Protection;

        switch(prot) {
        case P::READ_ONLY:            return MEM_NATIVE_PROT_READ_ONLY;
        case P::READ_WRITE:          return MEM_NATIVE_PROT_READ_WRITE;
        case P::READ_EXECUTE:        return MEM_NATIVE_PROT_READ_EXECUTE;
        case P::READ_WRITE_EXECUTE:  return MEM_NATIVE_PROT_READ_WRITE_EXECUTE;
        case P::NO_ACCESS:           return MEM_NATIVE_PROT_NO_ACCESS;
        default:
            throw std::invalid_argument("Unknown protection flag");
        }
    }
}

namespace OSAllocator {
    static size_t error = -1;
    size_t GetPageSize(){
        static size_t page_size = 0;
        if (page_size == 0){
            #ifdef _WIN32
                SYSTEM_INFO sys_info;
                GetSystemInfo(&sys_info);
                page_size = sys_info.dwPageSize;
            #else   
                page_size = sysconf(_SC_PAGESIZE);
            #endif
        }
        return page_size;
    }
    void * OS_Alloc(size_t size, size_t alignment){
        if (size == 0) return nullptr;

        // 确保按照 2的幂次 对齐
        if ((alignment & (alignment - 1)) != 0){
            alignment = 16; // 默认对齐16字节
        }

        // 大页分配阈值为 2MB
        constexpr size_t HUGE_PAGE_THRESHOLD = 2 * 1024 * 1024;
        // 大页面分配(开启大页分配往往需要系统权限)
        // Linux: 通常需要 root 或 /proc/sys/vm/nr_hugepages 设置
        // Windows: 需要权限 SeLockMemoryPrivilege
        if (size >= HUGE_PAGE_THRESHOLD){
            #ifdef _WIN32
                void *ptr = VirtualAlloc(
                    nullptr,  // 指定地址或 NUL
                    size,     // 要分配的内存大小(以字节为单位
                    MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, // 分配类型
                /*  flAllocationType
                    note: MEM_RESERVE: 保留地址空间, 但不分配物理内存(虚拟映射)
                    note: MEM_COMMIT:  实际分配物理内存并初始化为 0
                    note: MEM_RESET:   表示该区域即将被重新初始化
                    note: MEM_LARGE_PAGES: 使用大页(通常为 2MB 或更大),
                */
                    PAGE_READWRITE // 保护类型
                /* flProtect 
                    note: PAGE_READONLY : 只读
                    note: PAGE_READWRITE: 可读写
                    note: PAGE_EXECUTE_READ: 可读执行
                    note: PAGE_EXECUTE_READWRITE: 可读写执行
                    note: PAGE_NOACCESS: 无权限, 用于异常触发
                */
                );
                if (ptr) return ptr;
                else error = GetLastError();
            #elif defined(__linux__)
                void *ptr = mmap(
                    nullptr, // 通常为 NULL, 系统自动选择
                    size,    // 要映射的大小(页对齐)
                    PROT_READ | PROT_WRITE, // 内存保护属性
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, // 控制映射行为
                /*
                    note: MAP_PRIVATE: 写时复制(不会影响文件)
                    note: MAP_SHARED: 修改反映到文件
                    note: MAP_ANONYMOUS: 匿名映射(fd 必须是 -1)
                    note: MAP_HUGETLB: 使用大页(HugeTLB), 
                    note: MAP_FIXED: 强制映射到指定地址（危险，易引起 SEGFAULT）
                */
                    -1,      // 映射的文件描述符(匿名映射时设为 -1)
                    0        // 映射文件的起始偏移
                );
                if (ptr != MAP_FAILED) return ptr;
                else{
                    perror("Big Page Alloc Failed!");
                    error = errno;
                    std::cerr << strerror(error);
                }
            #endif
        }

        #ifdef _WIN32
            void *ptr = VirtualAlloc(
                nullptr,
                size, 
                MEM_COMMIT | MEM_RESERVE, 
                PAGE_READWRITE
            );
            bool is_fail = !ptr;
            size_t align__ = 16;
        #else
            void *ptr = mmap(
                nullptr,
                size,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS,
                -1, 
                0
            );
            bool is_fail = (ptr == MAP_FAILED);
            size_t align__ = GetPageSize();
        #endif
        // 处理对齐
        if (is_fail)
            throw std::bad_alloc();

        if (alignment > align__){
            void *align_ptr = _aligned_malloc(size, alignment);
            if(align_ptr){
                #ifdef _WIN32
                    VirtualFree(ptr, 0, MEM_RELEASE);
                #else
                    munmap(ptr, size);
                #endif
                return align_ptr;
            }
        }
        return ptr;
    }

    void OS_Free(void *ptr, size_t size){
        if (!ptr) return;

        #ifdef _WIN32
            if(size > 0 && size % GetPageSize() != 0){
                _aligned_free(ptr);
            }else{
                VirtualFree(ptr, 0, MEM_RELEASE);
            }
        #else
            if (size > 0 && size % GetPageSize() != 0){
                free(ptr);
            }else{
                munmap(ptr, size);
            }
        #endif
    }
}