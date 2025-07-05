#pragma once

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <cstddef>
#include <type_traits>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <array>
#include <utility>
#include <variant>

#include "MemoryPoolConfig.hpp"
#include "SysApi.hpp"
#include "../static_for.hpp"
template <std::size_t Size>
struct Block;

template <std::size_t BlockSize, std::size_t MaxSize>
struct BlockGroup;

template <std::size_t MaxSize>
using block_config = std::pair <std::size_t, 
    std::integral_constant<std::size_t, MaxSize>>;


static std::atomic<uint64_t> id {0};
static uint64_t GenerateBlockId() {    
    return id.fetch_add(1, std::memory_order_acq_rel);
}

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

struct IBlockGroupProxy{
    virtual ~IBlockGroupProxy() = default;
};

template <std::size_t BlockSize, std::size_t MaxSize>
struct BlockGroupProxy : IBlockGroupProxy {
    static constexpr std::size_t mem_size = BlockSize;
    static constexpr std::size_t max_size = MaxSize;
    std::unique_ptr<BlockGroup<BlockSize, MaxSize>> group;

    BlockGroupProxy()
        : group(MakeBlockGroup<BlockSize, MaxSize>()) {}

    void Insert(Block<mem_size>* block){}

    void Remove(void* ptr){
        using block_type = Block<BlockSize>;
        using block_ptr = Block<BlockSize>* ;
        block_ptr mem_ptr = container_of<block_type>(ptr, offsetof(block_type, mem));
        group.Remove(mem_ptr);
    }
};

template <std::size_t Size>
struct Block{
public:
    struct Info {
        void * addr;
        uint64_t id; 
        bool is_free;
    };
public:
    static constexpr std::size_t mem_size = Size;
public:
    void * mem;
    Block<Size>* next;
    Block<Size>* prev;
    const uint64_t id;
public:
    explicit Block(uint64_t id) : id(id) {
        mem = (void*)std::malloc(mem_size);
        ResetPtr();
    }
    Block(const Block&) = delete;
    Block(Block&&) = delete;
    ~Block(){
        if (mem) [[likely]]{
            std::free(mem);
            ResetPtr();
        }
    }
    Info& GetInfo() const {
        return { .addr = mem, .id = this->id, .is_free = is_free.load(std::memory_order_acquire) }; 
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
    void ResetPtr(){
        next = nullptr;
        prev = nullptr;
    }
};

template <std::size_t BlockSize, std::size_t MaxSize>
struct BlockGroup{
public:
    using node_type = Block<BlockSize>;
    using node_type_pointer = Block<BlockSize>*; 
    static constexpr std::size_t mem_size = BlockSize;
    static constexpr std::size_t max_size = MaxSize;
private:
    node_type_pointer head {nullptr};
    node_type_pointer tail {nullptr};
    std::array<node_type_pointer, max_size> index2addr {nullptr};
    uint64_t cur_size;

public:
    BlockGroup(){
        if (!head) [[likely]]{
            auto id = GenerateBlockId();
            head = Block<BlockSize>(id);
            head->MarkAsFree();
            tail = head;
        } 
    }

    BlockGroup(const BlockGroup&) = delete;
    BlockGroup(BlockGroup&&) = delete;
    ~BlockGroup() {
    }

    // @function 往内存链表中, 插入内存块
    // @return 返回信息
    Block<BlockSize>::Info Insert(node_type_pointer block){
        return {};
    }
    Block<BlockSize>::Info Remove(node_type_pointer block){
        return {};
    }

    Block<BlockSize>* GetBlockOf(const uint64_t index){
        return index2addr[index];
    }

    Block<BlockSize>* GetMemOf(const uint64_t index){
        return index2addr[index]->mem;
    }
    uint64_t GetSize() const {
        return cur_size;
    }
};

    

// 负责大规模的内存分配
struct Chunk{
    void * allocate(size_t size){
        return nullptr;
    }
    void deallocate(void * ptr, size_t size){

    }
    
};  