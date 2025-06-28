
#pragma once
#include <memory_resource>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <format>

#if defined(__linux__)
    #include <sys/syscall.h> // 仅限Linux
    #include <unistd.h>
#elif defined(_WIN32)
    #include <windows.h>
#endif

namespace Mem{
namespace{
    
    uint32_t get_thread_id() {
    #if defined(__linux__)
        return static_cast<uint32_t>(::syscall(SYS_gettid)); // Linux系统调用
    #elif defined(_WIN32)
        return static_cast<uint32_t>(::GetCurrentThreadId()); // Windows API
    #elif defined(__APPLE__)
        uint64_t tid;
        pthread_threadid_np(nullptr, &tid); // macOS/BSD方案
        return static_cast<uint32_t>(tid);
    #else
        // 回退到C++标准库（可能不唯一，但可移植）
        return static_cast<uint32_t>(
            std::hash<std::thread::id>{}(std::this_thread::get_id())
        );
    #endif
}
}
struct PerThreadMallocData{
    uint32_t tid;
    void* ptr;
    size_t size; 
    size_t align; 
    int64_t time;
    PerThreadMallocData() = default;
    PerThreadMallocData(const PerThreadMallocData&) = default;
    PerThreadMallocData(uint32_t tid, void* ptr, size_t size, size_t align, int64_t time)
        : tid(tid), ptr(ptr), size(size), align(align), time(time){}
};
class MemDetector{
public:
    static MemDetector& Instance() {
        static MemDetector instance;
        return instance;
    }

    MemDetector(const MemDetector&) = delete;
    MemDetector(MemDetector&&) = delete;
public:
    void Register(
        uint32_t tid,
        void* ptr, 
        size_t size, 
        size_t align 
    ) {
        auto now = std::chrono::high_resolution_clock::now();
        int64_t time = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        mem_datas.try_emplace(tid, tid, ptr, size, align, time);
    }
    void Print(){
        for(const auto& data: mem_datas){
            print_one(data.second.tid);
        }
    }
private:
    std::pmr::unordered_map<uint32_t, PerThreadMallocData> mem_datas;
private:
    void print_one(uint32_t tid){
        /* 方案1:
        uintptr_t ptr_val = reinterpret_cast<uintptr_t>(mem_datas[tid].ptr);
        size_t size_val = mem_datas[tid].size;
        size_t align_val = mem_datas[tid].align;
        int64_t time_val = mem_datas[tid].time;
    
        std::string formatted = std::vformat(
            "{{\n\ttid={};\n\tmemory_addr={};\n\tsize={};\n\talignment={}\n\tallocate time={}\n}}\n",
            std::make_format_args(tid, ptr_val, size_val, align_val, time_val)
        );
        std::cout << formatted << "\n";
        */
        char buffer[512];  // 预分配缓冲区
        auto end = std::format_to(
            buffer,
            "{{\n\ttid={};\n\tmemory_addr={};\n\tsize={};\n\talignment={}\n\tallocate time={}\n}}\n",
            tid,
            reinterpret_cast<uintptr_t>(mem_datas[tid].ptr),  // 直接传递右值
            mem_datas[tid].size,
            mem_datas[tid].align,
            mem_datas[tid].time
        );
        *end = '\0';  // 添加终止符
        std::cout << buffer;
    }
private:
    MemDetector() = default;
    ~MemDetector() = default;
};

class IMalloc {
public:
    virtual void * Malloc(size_t bytes, size_t alignment = 32) = 0;
    virtual void Free(void* ptr, size_t alignment = 32) noexcept = 0;
    virtual void * Realloc(void * ptr, size_t count, size_t alignment = 32) = 0;
public:
    MemDetector * detector;
};

class SysAllocator
    : public IMalloc,
      public std::pmr::synchronized_pool_resource{

public:
    void *Malloc(size_t bytes, size_t alignment = 32) override{
        // note: RVO 优化
        void * ptr = this->do_allocate(bytes, alignment);
        MemDetector::Instance().Register(
            get_thread_id(),
            ptr,
            bytes,
            alignment
        );
        MemDetector::Instance().Print();
        return this->do_allocate(bytes, alignment);
    }
    void *Realloc(void * ptr, size_t count, size_t alignment = 32) override{
        void * _ptr = ::std::malloc(1);
        return _ptr;
    }
    void Free(void* ptr, size_t alignment = 32) noexcept override{
        std::cout << "SysAllocator:Free" << std::endl;
        this->do_deallocate(ptr, 1, alignment);
    }
private:

    void * do_allocate(std::size_t bytes, std::size_t alignment) override {
        void * ptr = std::pmr::synchronized_pool_resource::do_allocate(bytes, alignment);
        return ptr;
    }
    void do_deallocate(void * ptr, std::size_t bytes, std::size_t alignment) override {
        std::pmr::synchronized_pool_resource::do_deallocate(ptr, bytes, alignment);
    }  

};
}