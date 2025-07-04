#pragma once
#include <iostream>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <mutex>
#include <new>
#include <thread>
#include <vector>

#if __unix__
#include <sys/mman.h>
#include <unistd.h>
#define MEMDETECTOR_EXPORT
#elif _WIN32
#include <windows.h>
#define MEMDETECTOR_EXPORT // __declspec(dllexport)
#else // note: 可以接入macOS
#define MEMDETECTOR_EXPORT
#endif

// note: 检测是否支持 pmr
#if __cplusplus >= 201703L || __cpp_lib_memory_resource
#include <memory_resource>
#endif

// note: 导入 std::pmr
#if __cpp_lib_memory_resource
#define PMR std::pmr
#define PMR_INIT(x) { x }
#define HAS_PMR 1
#else
#define PMR std
#define PMR_INIT(x) 
#define HAS_PMR 0
#endif

#include "intern/Addr2sym.hpp"
#include "intern/AllocAction.hpp"
#include "../Log.h"

// note: 将内容限定在当前编译单元（.cpp 文件）内部
// note: 防止与其他编译单元的符号冲突
namespace {

    uint32_t get_thread_id() {
        #if __unix__
            return gettid();
        #elif _WIN32
            return GetCurrentThreadId();
        #else
            return 0;
        #endif
    }

}

namespace MemDetector{
    struct alignas(64) PerThreadData{
    #if HAS_PMR
        size_t const buf_size = 64 * 1024 * 1024;
        #if __unix__
            void *buf = mmap(nullptr, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONTMOUS, -1, 0);
        #elif _WIN32
            void *buf = VirtualAlloc(nullptr, buf_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        #else
            static char buf[buf_size];
        #endif
        PMR::monotonic_buffer_resource mono{buf, buf_size};
        PMR::unsynchronized_pool_resource pool{&mono};
    #endif
        std::recursive_mutex lock;
        PMR::deque<AllocAction> actions PMR_INIT(&pool);
        bool enable = false;
    };

    struct GlobalData{
        std::mutex lock;
        static inline size_t const kPerThreadsCount = 8;
        PerThreadData per_threads[kPerThreadsCount];
        bool export_plot_on_exit = true; // 用于 plot 待定
    #if HAS_THREADS
        std::thread export_thread;
    #endif 
        std::atomic<bool> stopped { false };

        GlobalData(){
        #if HAS_THREADS
            // note: 这里 if(0) 意味着默认不执行, 改为1既可以
            // note: 创建管道文件 malloc.fifo (mkfifo malloc.fifo)
            // note: 监听管道文件输出 cat malloc.fifo 
            if (0) {
                std::string path = "malloc.fifo";
                export_thread = std::thread([this, path] {
                    get_per_thread(get_thread_id())->enable = false;
                    export_thread_entry(path);
                });
                export_plot_on_exit = false;
            }
        #endif
            for (size_t i = 0; i < kPerThreadsCount; ++i) {
                per_threads[i].enable = true;
            }
        }

        PerThreadData *get_per_thread(uint32_t tid) {
            return per_threads + ((size_t)tid * 17) % kPerThreadsCount;
        }

    #if HAS_THREADS
        void exprot_thead_entry(const string& path){
        #if HAS_PMR
            #if __unix__
                void *buf = mmap(nullptr, 
                    buf_size, 
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, 
                    -1, 
                    0
                );
            #elif _WIN32
                void *buf = VirtualAlloc(
                    nullptr, 
                    buf_size, 
                    MEM_RESERVE | MEM_COMMIT,
                    PAGE_READWRITE
                );
            #else
                static char buf[buf_size];
            #endif 
            PMR::monotonic_buffer_resource mono{buf, bufsz};
            PMR::unsynchronized_pool_resource pool{&mono};
        #endif
            std::ofstream out(path, std::ios::binary);
            PMR::deque<AllocAction> actions PMR_INIT(&pool);
            while(!stopped.load(std::memory_order_threads)){
                for(auto& per_thread: per_threads){
                    std::unique_lock<std::recursive_mutex> guard(per_thread.lock);
                    auto thread_actions = std::move(per_thread.actions);
                    guard.unlock();
                    actions.insert(actions.end(), thread_actions.begin(), thread_actions.end());
                }
                if(!actions.empty()){
                    for (auto& action: actions){
                        out.write((char const *)&action, sizeof(AllocAction));
                    }
                    actions.clear();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            for(auto& per_thread: per_threads){
                std::unique_lock<std::recursive_mutex> guard(per_thread.lock);
                auto thread_actions = std::move(per_thread.actions);
                guard.unlock();
                actions.insert(actions.end(), thread_actions.begin(), thread_actions.end());
            }
            if (!actions.empty()) {
                for (auto &action: actions) {
                    out.write((char const *)&action, sizeof(AllocAction));
                }
                actions.clear();
            }
        }
    #endif
        ~GlobalData() {
            for (size_t i = 0; i < kPerThreadsCount; ++i) {
                per_threads[i].enable = false;
            }
        #if HAS_THREADS
            if (export_thread.joinable()) {
                stopped.store(true, std::memory_order_release);
                export_thread.join();
            }
        #endif
            if (export_plot_on_exit) {
                std::vector<AllocAction> actions;
                for (size_t i = 0; i < kPerThreadsCount; ++i) {
                    auto &their_actions = per_threads[i].actions;
                    actions.insert(actions.end(), their_actions.begin(),
                                their_actions.end());
                }
                // mallocvis_plot_alloc_actions(std::move(actions));
        }
    }

    };

    GlobalData* global = nullptr;

    struct EnableGuard{
        uint32_t tid;
        bool was_enable;
        PerThreadData* per_thread;

        EnableGuard()
            : tid(get_thread_id()),
              per_thread(global ? global->get_per_thread(tid) : nullptr){
            if (!per_thread) {
                was_enable = false;
            } else {
                per_thread->lock.lock();
                was_enable = per_thread->enable;
                per_thread->enable = false;
            }
        }
        explicit operator bool() const {
            return was_enable;
        }
        // note: 该函数是一个 用于记录内存操作信息的函数, 给所有的内存分配器添加这方法, 来保存内存信息
        void on(AllocOp op, 
                void *ptr, 
                size_t size, 
                size_t align,
                void *caller
        ) const {
            if (ptr) {
                auto now = std::chrono::high_resolution_clock::now();
                int64_t time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                now.time_since_epoch()
                            ).count();
                per_thread->actions.push_back(
                    AllocAction{op, tid, ptr, size, align, caller, time}
                );
            }
        }
        ~EnableGuard() {
            if (per_thread) {
                per_thread->enable = was_enable;
                per_thread->lock.unlock();
            }
        }
    };
}

#if __GNUC__
extern "C" void *__libc_malloc(size_t size) noexcept;
extern "C" void __libc_free(void *ptr) noexcept;
extern "C" void *__libc_calloc(size_t nmemb, size_t size) noexcept;
extern "C" void *__libc_realloc(void *ptr, size_t size) noexcept;
extern "C" void *__libc_reallocarray(void *ptr, size_t nmemb,
                                     size_t size) noexcept;
extern "C" void *__libc_valloc(size_t size) noexcept;
extern "C" void *__libc_memalign(size_t align, size_t size) noexcept;
# define REAL_LIBC(name) __libc_##name
# ifndef MAY_OVERRIDE_MALLOC
#  define MAY_OVERRIDE_MALLOC 1
# endif
# ifndef MAY_OVERRIDE_MEMALIGN
#  define MAY_SUPPORT_MEMALIGN 1
# endif
# undef RETURN_ADDRESS
# ifdef __has_builtin
#  if __has_builtin(__builtin_return_address)
#   if __has_builtin(__builtin_extract_return_addr)
#    define RETURN_ADDRESS \
        __builtin_extract_return_addr(__builtin_return_address(0))
#   else
#    define RETURN_ADDRESS __builtin_return_address(0)
#   endif
#  endif
# elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
#  define RETURN_ADDRESS __builtin_return_address(0)
# endif
# ifndef RETURN_ADDRESS
#  define RETURN_ADDRESS ((void *)0)
#  pragma message("Cannot find __builtin_return_address")
# endif
# define CSTDLIB_NOEXCEPT noexcept
#elif _MSC_VER

static void *msvc_malloc(size_t size) noexcept {
    return HeapAlloc(GetProcessHeap(), 0, size);
}

static void *msvc_calloc(size_t nmemb, size_t size) noexcept {
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nmemb * size);
}

static void msvc_free(void *ptr) noexcept {
    HeapFree(GetProcessHeap(), 0, ptr);
}

static void *msvc_realloc(void *ptr, size_t size) noexcept {
    return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
}

static void *msvc_reallocarray(void *ptr, size_t nmemb, size_t size) noexcept {
    return msvc_realloc(ptr, nmemb * size);
}

# define REAL_LIBC(name) msvc_##name
# ifndef MAY_OVERRIDE_MALLOC
#  define MAY_OVERRIDE_MALLOC 0
# endif
# ifndef MAY_OVERRIDE_MEMALIGN
#  define MAY_SUPPORT_MEMALIGN 0
# endif

# include <intrin.h>

# pragma intrinsic(_ReturnAddress)
# define RETURN_ADDRESS _ReturnAddress()
# define CSTDLIB_NOEXCEPT

#else
# define REAL_LIBC(name) name
# ifndef MAY_OVERRIDE_MALLOC
#  define MAY_OVERRIDE_MALLOC 0
# endif
# ifndef MAY_OVERRIDE_MEMALIGN
#  define MAY_SUPPORT_MEMALIGN 0
# endif
# define RETURN_ADDRESS ((void *)1)
# define CSTDLIB_NOEXCEPT
#endif


MEMDETECTOR_EXPORT void operator delete(void *ptr) noexcept {
    MemDetector::EnableGuard ena;
    if (ena) {
        ena.on(AllocOp::Delete, ptr, kNone, kNone, RETURN_ADDRESS);
    }
    REAL_LIBC(free)(ptr);
}

MEMDETECTOR_EXPORT void operator delete[](void *ptr) noexcept {
    MemDetector::EnableGuard ena;
    if (ena) {
        ena.on(AllocOp::DeleteArray, ptr, kNone, kNone, RETURN_ADDRESS);
    }
    REAL_LIBC(free)(ptr);
}

MEMDETECTOR_EXPORT void operator delete(void *ptr,
                                      std::nothrow_t const &) noexcept {
    MemDetector::EnableGuard ena;
    if (ena) {
        ena.on(AllocOp::Delete, ptr, kNone, kNone, RETURN_ADDRESS);
    }
    REAL_LIBC(free)(ptr);
}

MEMDETECTOR_EXPORT void operator delete[](void *ptr,
                                        std::nothrow_t const &) noexcept {
    MemDetector::EnableGuard ena;
    if (ena) {
        ena.on(AllocOp::DeleteArray, ptr, kNone, kNone, RETURN_ADDRESS);
    }
    REAL_LIBC(free)(ptr);
}

MEMDETECTOR_EXPORT void *operator new(size_t size) noexcept(false) {
    MemDetector::EnableGuard ena;
    void *ptr = REAL_LIBC(malloc)(size);
    if (ena) {
        ena.on(AllocOp::New, ptr, size, kNone, RETURN_ADDRESS);
    }
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

MEMDETECTOR_EXPORT void *operator new[](size_t size) noexcept(false) {
    MemDetector::EnableGuard ena;
    void *ptr = REAL_LIBC(malloc)(size);
    if (ena) {
        ena.on(AllocOp::NewArray, ptr, size, kNone, RETURN_ADDRESS);
    }
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

MEMDETECTOR_EXPORT void *operator new(size_t size,
                                    std::nothrow_t const &) noexcept {
    MemDetector::EnableGuard ena;
    void *ptr = REAL_LIBC(malloc)(size);
    if (ena) {
        ena.on(AllocOp::New, ptr, size, kNone, RETURN_ADDRESS);
    }
    return ptr;
}

MEMDETECTOR_EXPORT void *operator new[](size_t size,
                                      std::nothrow_t const &) noexcept {
    MemDetector::EnableGuard ena;
    void *ptr = REAL_LIBC(malloc)(size);
    if (ena) {
        ena.on(AllocOp::NewArray, ptr, size, kNone, RETURN_ADDRESS);
    }
    return ptr;
}

#if (__cplusplus >= 201402L || _MSC_VER >= 1916)
MEMDETECTOR_EXPORT void operator delete(void *ptr, size_t size) noexcept {
    MemDetector::EnableGuard ena;
    if (ena) {
        ena.on(AllocOp::Delete, ptr, size, kNone, RETURN_ADDRESS);
    }
    REAL_LIBC(free)(ptr);
}

MEMDETECTOR_EXPORT void operator delete[](void *ptr, size_t size) noexcept {
    MemDetector::EnableGuard ena;
    if (ena) {
        ena.on(AllocOp::DeleteArray, ptr, size, kNone, RETURN_ADDRESS);
    }
    REAL_LIBC(free)(ptr);
}
#endif

#if (__cplusplus > 201402L || defined(__cpp_aligned_new))
# if MAY_SUPPORT_MEMALIGN
MEMDETECTOR_EXPORT void operator delete(void *ptr,
                                      std::align_val_t align) noexcept {
    MemDetector::EnableGuard ena;
    if (ena) {
        ena.on(AllocOp::Delete, ptr, kNone, (size_t)align, RETURN_ADDRESS);
    }
    REAL_LIBC(free)(ptr);
}

MEMDETECTOR_EXPORT void operator delete[](void *ptr,
                                        std::align_val_t align) noexcept {
    MemDetector::EnableGuard ena;
    if (ena) {
        ena.on(AllocOp::DeleteArray, ptr, kNone, (size_t)align, RETURN_ADDRESS);
    }
    REAL_LIBC(free)(ptr);
}

MEMDETECTOR_EXPORT void operator delete(void *ptr, size_t size,
                                      std::align_val_t align) noexcept {
    MemDetector::EnableGuard ena;
    if (ena) {
        ena.on(AllocOp::Delete, ptr, size, (size_t)align, RETURN_ADDRESS);
    }
    REAL_LIBC(free)(ptr);
}

MEMDETECTOR_EXPORT void operator delete[](void *ptr, size_t size,
                                        std::align_val_t align) noexcept {
    MemDetector::EnableGuard ena;
    if (ena) {
        ena.on(AllocOp::DeleteArray, ptr, size, (size_t)align, RETURN_ADDRESS);
    }
    REAL_LIBC(free)(ptr);
}

MEMDETECTOR_EXPORT void operator delete(void *ptr, std::align_val_t align,
                                      std::nothrow_t const &) noexcept {
    MemDetector::EnableGuard ena;
    if (ena) {
        ena.on(AllocOp::Delete, ptr, kNone, (size_t)align, RETURN_ADDRESS);
    }
    REAL_LIBC(free)(ptr);
}

MEMDETECTOR_EXPORT void operator delete[](void *ptr, std::align_val_t align,
                                        std::nothrow_t const &) noexcept {
    MemDetector::EnableGuard ena;
    if (ena) {
        ena.on(AllocOp::DeleteArray, ptr, kNone, (size_t)align, RETURN_ADDRESS);
    }
    REAL_LIBC(free)(ptr);
}

MEMDETECTOR_EXPORT void *operator new(size_t size,
                                    std::align_val_t align) noexcept(false) {
    MemDetector::EnableGuard ena;
    void *ptr = REAL_LIBC(memalign)((size_t)align, size);
    if (ena) {
        ena.on(AllocOp::New, ptr, size, (size_t)align, RETURN_ADDRESS);
    }
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

MEMDETECTOR_EXPORT void *operator new[](size_t size,
                                      std::align_val_t align) noexcept(false) {
    MemDetector::EnableGuard ena;
    void *ptr = REAL_LIBC(memalign)((size_t)align, size);
    if (ena) {
        ena.on(AllocOp::NewArray, ptr, size, (size_t)align, RETURN_ADDRESS);
    }
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

MEMDETECTOR_EXPORT void *operator new(size_t size, std::align_val_t align,
                                    std::nothrow_t const &) noexcept {
    MemDetector::EnableGuard ena;
    void *ptr = REAL_LIBC(memalign)((size_t)align, size);
    if (ena) {
        ena.on(AllocOp::New, ptr, size, (size_t)align, RETURN_ADDRESS);
    }
    return ptr;
}

MEMDETECTOR_EXPORT void *operator new[](size_t size, std::align_val_t align,
                                      std::nothrow_t const &) noexcept {
    MemDetector::EnableGuard ena;
    void *ptr = REAL_LIBC(memalign)((size_t)align, size);
    if (ena) {
        ena.on(AllocOp::NewArray, ptr, size, (size_t)align, RETURN_ADDRESS);
    }
    return ptr;
}
# endif
#endif

#if MANUAL_GLOBAL_INIT
alignas(GlobalData) static char global_buf[sizeof(GlobalData)];

// # if __has_attribute(__constructor__) && __has_attribute(__destructor__)
// #  define GLOBAL_INIT_PRIORITY 101
// # endif

// __attribute__((__constructor__(GLOBAL_INIT_PRIORITY)))
void mallocvis_init() {
    global = new (&global_buf) GlobalData();
}

// __attribute__((__destructor__(GLOBAL_INIT_PRIORITY)))
void mallocvis_deinit() {
    if (global) {
        global->~GlobalData();
        global = nullptr;
    }
}
#else
static MemDetector::GlobalData global_buf;
static int global_init_helper = (MemDetector::global = &global_buf, 0);
#endif


