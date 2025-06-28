#pragma once
#include <bitset>
#include <queue>
#include "MemoryPoolConfig.hpp"

struct Region{
    int64_t id;
    void * addr;
    size_t free_num;
    std::bitset<region_num> bitmap{0x00};
};

struct RegionCompare{
    bool operator() (const Region* a, const Region* b) const {
        return a->addr > b->addr;
    }
};

using RegionHeap = std::priority_queue<Region*, std::vector<Region*>, RegionCompare>;
