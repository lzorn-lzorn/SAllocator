#pragma once
#include <cstdint>
#include <memory>
#include <cstdlib>
// @function: 给定 void* addr 从而推算出其所属的成员变量
// @usage: Block<64>* block = container_of<Block<64>>(addr, offsetof(Block<64>, mem));
template<typename T, typename U>
static T* container_of(U* ptr, size_t offset){
    return reinterpret_cast<T*>(
        reinterpret_cast<char*>(ptr) - offset
    );
}
// @function: 给定 void* addr 从而推算出其所属的成员变量
// @usage: Block<64>* block = container_of<Block<64>>(addr, &Block<64>::mem);
template<typename T, typename U>
static T* container_of(U* ptr, U T::*member){
    return reinterpret_cast<std::ptrdiff_t>(
        // const volatile 是为了让编译器禁止优化, 从而出现 UB
        // 该指针只是 形式上的类型信息
        &(reinterpret_cast<T const volatile*>(0)->*member)
    );
}


template <std::size_t Size>
struct MemNode{
public:
    static constexpr std::size_t mem_size = Size;
public:
    void * mem;
    const uint64_t id;
public:
    explicit MemNode(uint64_t id) : id(id) {
        mem = (void*)std::malloc(mem_size);
        ResetPtr();
    }
    MemNode(const MemNode&) = delete;
    MemNode(MemNode&&) = delete;
    ~MemNode(){
        if (mem) [[likely]]{
            std::free(mem);
        }
    }

    bool operator<(const MemNode& other) const {
        return reinterpret_cast<uintptr_t>(mem) < reinterpret_cast<uintptr_t>(other.mem);
    }
    uint64_t GetUId() const noexcept {
        return id;
    }
    const void* GetMemPtr() const {
        return mem;
    } 
    bool IsFree() const {
        // 保证之后的读写操作不会被重排到此之前, 防止读到错误数据
        return is_free.load(std::memory_order_acquire);
    }
    void MarkAsUsed() {
        // 保证之前的读写操作不会被重排到此之后, 防止写入时错误覆盖
        is_free.store(false, std::memory_order_release);
    }
    void MarkAsFree() {
        is_free.store(true, std::memory_order_release);
    }
public:
    // 虽然这里开放了 is_free 这变量, 因为 必要要求 Block 是满足 Standard Layout 
    // 此时才能使用  reinterpret_cast 通过 void* addr 推算出 struct 的地址
    // 所以不应该外部手动设置 is_free 
    // 应该使用接口 void MarkAsFree() 和 void MarkAsUsed()
    std::atomic<bool> is_free {true};
};

