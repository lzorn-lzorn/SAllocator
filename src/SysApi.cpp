#include "../include/JAllocatorImpl/SysApi.h"
#include <cstdint>

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

    static size_t NextPowerOfTwo(size_t x) {
        // 位运算展开填满所有低位的 1，再加 1 得到下一个 2 的幂。
        if (x == 0) return 1;
        x--;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
    #if SIZE_MAX > UINT32_MAX
        x |= x >> 32; // 64-bit平台支持
    #endif
        return x + 1;
    }


    void * OS_Alloc(size_t size, size_t alignment){
        if (size == 0) [[unlikely]] return nullptr;
        // assert(alignment >= sizeof(void*) && "alignment must be at least sizeof(void*)");

        alignment = alignment > sizeof(void*) ? alignment : sizeof(void*);

        // 确保按照 2的幂次 对齐
        // note: 以下是一个 2 的幂次经典技巧:
        // note: (x>0)^(x&(x-1)) == 0 <-iff-> x 是 2 的幂
        if ((alignment & (alignment - 1)) != 0) [[unlikely]] {
            // 默认对齐不: 小于输入值的最小的 2 的幂, 即向上取整到最近的 2 的幂
            // alignment = NextPowerOfTwo(alignment);
            // C++20 本身有 NextPowerOfTwo 的实现 -- bit_ceil
            // alignment = std::bit_ceil(alignment);
            alignment = 16;
        }

        #ifdef _WIN32
            SYSTEM_INFO sys_info;
            GetSystemInfo(&sys_info);
            alignment = alignment > sys_info.dwAllocationGranularity ? 
                alignment : sys_info.dwAllocationGranularity;
        #endif

        // 大页分配阈值为 2MB
        size_t page_size = GetPageSize();
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
                error = GetLastError();
                std::cerr << "VirtualAlloc failed with error: " << error << "\n";
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
                error = errno;
                std::cerr << "mmap failed with error: " << strerror(error) << "\n";
            #endif
        }

        // 小内存分配路径
        if (size < page_size) {
        #ifdef _WIN32
            return _aligned_malloc(size, alignment);
        #else
            return std::aligned_alloc(
                alignment, 
                ((size + alignment - 1) / alignment) * alignment
            );
        #endif
        }

        // 正常页面分配:
        // size_t header_size = sizeof(AllocHeader);
        size_t header_size = (sizeof(AllocHeader) + alignment - 1) & ~(alignment - 1);
        size_t total_size = size + alignment + header_size;
        void *base_ptr = nullptr;
        bool is_fail = false;
        #ifdef _WIN32
            base_ptr = VirtualAlloc(
                nullptr,
                total_size, 
                MEM_COMMIT | MEM_RESERVE, 
                PAGE_READWRITE
            );
            is_fail = !base_ptr;
            // size_t align__ = 16;
        #else
            base_ptr = mmap(
                nullptr,
                total_size,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS,
                -1, 
                0
            );
            is_fail = (base_ptr == MAP_FAILED);
            // size_t align__ = GetPageSize();
        #endif
        // 处理对齐
        if (is_fail){
            //std::cerr << "正常内存页面分配失败\n";
            throw std::bad_alloc();
        }
        //std::cerr << "正常内存页面分配成功\n";
        // 指针对齐
        // note: 将原始地址 raw_addr 向上对齐到 alignment
        // note: aliged_addr = math.floor((raw_addr + alignment - 1)/alignment) * alignment
        // note: 等价于: (raw_addr + alignment - 1) & ~(alignment - 1)
        // note: Eg: raw_addr = 1000, alignment = 64
        // note: 1000 + 63 = 1063, 二进制为 100001100111
        // note: alignment - 1 = 63 = 000000111111
        // note: ~63 = 0xFFFFFFC0
        // note: 所以 1063 & 0xFFFFFFC0 = 1024，即对齐到下一个 64 倍数
        uintptr_t raw_addr = reinterpret_cast<uintptr_t>(base_ptr);
        uintptr_t aligned_addr = (raw_addr + alignment - 1) & ~(alignment - 1);
        void* aligned_ptr = reinterpret_cast<void*>(aligned_addr);

        uintptr_t header_addr = aligned_addr - sizeof(AllocHeader);
        if (header_addr < raw_addr) {
            // 如果 header 会低于 base_ptr，调整对齐方式
            aligned_addr = (raw_addr + alignment + sizeof(AllocHeader) - 1) & ~(alignment - 1);
            aligned_ptr = reinterpret_cast<void*>(aligned_addr);
            header_addr = aligned_addr - sizeof(AllocHeader);
        }
        // 记录头部信息
        auto* header = reinterpret_cast<AllocHeader*>(aligned_addr - header_size);
        header->base_ptr = base_ptr;
        header->total_size = total_size;
  
        return aligned_ptr;
    }

    void OS_Free(void *ptr, size_t size){
        if (!ptr) return;

        size_t page_size = GetPageSize();

        if (size < page_size) {
        #ifdef _WIN32
            _aligned_free(ptr);
        #else
            std::free(ptr);
        #endif
            return;
        } 

        auto* header = reinterpret_cast<AllocHeader*>(
            reinterpret_cast<uintptr_t>(ptr) - sizeof(AllocHeader)
        );
        void* base_ptr = header->base_ptr;
        size_t total_size = header->total_size;
        #ifdef _WIN32
            VirtualFree(base_ptr, 0, MEM_RELEASE);
        #else
            munmap(base_ptr, total_size);
        #endif
        
    }
}