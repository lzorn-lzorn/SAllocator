#pragma once
#include <queue>
#include "Region.hpp"
#include "MemoryPoolConfig.hpp"




struct BinInfo{};

struct Bin{
    Region * cur_region;
    RegionHeap regions;
    BinInfo info;
};
