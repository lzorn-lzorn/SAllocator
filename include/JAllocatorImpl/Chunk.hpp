#pragma once

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <type_traits>
#include <functional>
#include <unordered_set>
#include <bitset>
#include <array>
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
    explicit Block(uint64_t id) {
        id = id;
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
private:
    std::atomic<bool> is_free {true};
    void ResetPtr(){
        next = nullptr;
        prev = nullptr;
    }
};

template <std::size_t BlockSize, std::size_t MaxSize>
struct BlockGroup{
public:
    static constexpr std::size_t mem_size = BlockSize;
    static constexpr std::size_t max_size = MaxSize;
private:
    Block<BlockSize>* head {nullptr};
    Block<BlockSize>* tail {nullptr};
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
    Block<BlockSize>::Info Insert(Block<BlockSize>* block){
        return {};
    }
    Block<BlockSize>::Info Remove(Block<BlockSize>* block){
        return {};
    }

    Block<BlockSize>* Get(){
        return {};
    }
};

    

// 负责大规模的内存分配
struct Chunk{
    void * allocate(size_t size){
        return nullptr;
    }
    void deallocate(void * ptr, size_t){

    }
    
};  