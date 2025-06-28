#pragma once
#include <cstdint>
#include <memory_resource>
#include <cstdint>

#if defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
#include <windows.h>
#include <process.h>

// 设置主线程栈（链接时设置）
#define SETSTACKSIZE(size) \
    __pragma(comment(linker, "/STACK:" #size))

#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <pthread.h>
#include <sys/resource.h>
#include <unistd.h>

// 设置主线程最大栈（仅对主线程有效）
#define SETSTACKSIZE(size)                       \
    do {                                         \
        struct rlimit rl;                        \
        if (getrlimit(RLIMIT_STACK, &rl) == 0) { \
            rl.rlim_cur = (size_t)(size);        \
            setrlimit(RLIMIT_STACK, &rl);        \
        }                                        \
    } while (0)

#else
#define SETSTACKSIZE(size) do {} while(0)
#endif

constexpr uint32_t stack_size = 8 * 1024 * 1024; // 8MB 栈空间, 多线程情况每个线程8MB
constexpr uint32_t stack_align = 32;

enum class MemoryState
{
    InValid, // note: 非法内存 => 没有被内存池管理的
    Free,    // note: 空闲内存
    Allocated// note: 已被分配内存
};
struct MemoryInfo
{
    MemoryState state; // note: 内存限制状态                           
    uint32_t tid;      // note: 线程id
    void *ptr;         // note: 内存首地址
    size_t size;       // note: 内存块的大小
    size_t align;      // note: 内存块的对齐粒度
    void *caller;      // note: 调用者的指针
    int64_t time;      // note: 申请时间
};

/*
 * @function: 读取配置构建器, 会初始化所有关于内存池的配置
 */
class MemorySystemBuilder
{

};


/*
 * @function: 内存记录器
 */ 
class MemoryDetector
{

};


/*
 * @function: 栈空间的内存池, 所有的小对象都会使用该内存池进行分配
 */ 
struct StackMemory : public std::pmr::monotonic_buffer_resource
{
public:
    // note: 栈空间用完就没有了
    explicit StackMemory(std::pmr::memory_resource* upstream = std::pmr::null_memory_resource())
        : monotonic_buffer_resource(nullptr, 0, upstream){
            SETSTACKSIZE(stack_size); // note: 设置主线程的最大分配栈空间为 8MB
        }

public:
    void* do_allocate(size_t bytes, size_t alignment) override {
        void * ptr = monotonic_buffer_resource::do_allocate(bytes, alignment);

        return ptr;
    }
    void do_deallocate(void* p, size_t bytes, size_t alignment) override {

        monotonic_buffer_resource::do_deallocate(p, bytes, alignment);
    }
private:
    alignas(stack_align) static char buf [stack_size];
};

class Pool
{

};
