#pragma once
#include <cstddef>
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
    size_t offset = reinterpret_cast<std::ptrdiff_t>(
        &(reinterpret_cast<T const volatile*>(0)->*member)
    );
    return contain_of<T, U>(ptr, offset);
}


template <std::size_t Size>
struct MemNode{
public:
    static constexpr std::size_t mem_size = Size;
public:
    void * mem;
    const uint64_t id;
    bool is_free;
public:
    explicit MemNode(uint64_t id) : id(id) {
        mem = (void*)std::malloc(mem_size);
        ResetPtr();
    }
    MemNode(const MemNode&) = delete;
    MemNode(MemNode&&) = delete;
    ~MemNode(){
        if (mem) [[likely]] {
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

};

