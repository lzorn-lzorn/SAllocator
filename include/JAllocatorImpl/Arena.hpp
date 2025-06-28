#pragma once
#include <vector>
#include <atomic>
#include <array>
#include <thread>
#include "JAllocatorImpl/Region.hpp"
#include "JAllocatorImpl/Bin.hpp"
#include "JAllocatorImpl/Chunk.hpp"

struct Arena{
    void* allocate(const size_t size);
    void deallocate(void* ptr, const size_t size);

    std::atomic<size_t> id;
    std::vector<std::thread::id> thread_ids;          // 占用该 Arena 的线程
    std::array<Chunk*, spare_chunk_num> spare_chunks; // 备用的 Chunk 块
    std::array<Region*, available_region_num> avail_runs;   // 空闲内存块
    std::array<Bin, bin_num> bins;                    // bin 的数量
};