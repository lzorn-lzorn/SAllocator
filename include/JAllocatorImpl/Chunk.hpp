#pragma once
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <cstddef>
#include <type_traits>
#include <functional>
#include <deque>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <array>
#include <utility>
#include <chrono>
#include <thread>
#include <variant>

#include "MemoryPoolConfig.hpp"
#include "SysApi.h"
#include "Block.hpp"
#include "../static_for.hpp"
template <std::size_t Size>
struct Block;

template <
    std::size_t BlockSize,  
    template <typename> class Container
>
struct BlockGroup{};

using namespace std::chrono;
template <std::size_t BlockSize>
struct BlockGroup<BlockSize, std::deque>{
public:
    using node_type = Block<BlockSize>;
    using node_type_pointer = Block<BlockSize>*; 
    using container = std::deque<MemNode<BlockSize>>;
    static constexpr std::size_t mem_size = BlockSize;
    // note: Info 和内部的 mem_ptr 是全时间绑定的, mem_ptr 被销毁则 Info 被销毁
    struct Info{
        std::thread::id thread_id;
        
        Info(node_type_pointer mem_ptr_) : mem_ptr(mem_ptr_) {
            create_time = steady_clock::now();
        }

        ~Info(){
            enter_duration = OverTimekeeping(create_time);
            mem_ptr = nullptr;
        }
        void mark_enter(){
            enter_time = steady_clock::now();
        }
        void mark_quit(){
            enter_duration = OverTimekeeping(enter_time);
        }
        milliseconds GetEnterTime() const {
            return enter_duration;
        }
        milliseconds GetLifeTime() const {
            return life_duration;
        }
    private:
        milliseconds OverTimekeeping(steady_clock::time_point which_time){
            auto quit_time = steady_clock::now();
            enter_duration += duration_cast<milliseconds>(
                quit_time - which_time
            );
        } 
    private:
        node_type_pointer mem_ptr;

        steady_clock::time_point enter_time;
        steady_clock::time_point create_time;

        milliseconds enter_duration;
        milliseconds life_duration;
    };
private:
    container con;
    std::set<Info> free_nodes;
    std::set<Info> used_nodes;
public:
    explicit BlockGroup(){}

    BlockGroup(const BlockGroup&) = delete;
    BlockGroup(BlockGroup&&) = delete;
    ~BlockGroup() {
        con.clear();
        free_nodes.clear();
        used_nodes.clear();
    }

    // @function 往内存链表中, 插入内存块
    // @return 返回信息
    Block<BlockSize>::Info Insert(node_type_pointer block){
        return {};
    }
    Block<BlockSize>::Info Remove(node_type_pointer block){
        return {};
    }
private:
    static Info& CreateMemInfo(std::thread::id id){
        
    }

};
struct IBlockGroupProxy{
    virtual ~IBlockGroupProxy() = default;
};

template <std::size_t BlockSize, std::size_t MaxSize>
struct BlockGroupProxy : IBlockGroupProxy {
    static constexpr std::size_t mem_size = BlockSize;
    static constexpr std::size_t max_size = MaxSize;
    std::unique_ptr<BlockGroup<BlockSize, std::deque>> group;

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



    

// 负责大规模的内存分配
struct Chunk{
    void * allocate(size_t size){
        return nullptr;
    }
    void deallocate(void * ptr, size_t size){

    }
    
};  