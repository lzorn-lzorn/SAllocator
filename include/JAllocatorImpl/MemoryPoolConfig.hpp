#pragma once
#include <array>
#include <cstdint>
// 小规模分配 16KB 以下都是小规模分配
constexpr uint32_t small_alloc = 16 << 10; 
// 中等规模分配 16KB~4MB都是中等规模
constexpr uint32_t large_alloc = 4 << 20;
// 备用Chunk块的数量
constexpr uint32_t spare_chunk_num = 4;

// 空闲块的数量
constexpr uint32_t available_region_num = 1024;
// 一个arena持有的bin的数量
constexpr uint32_t bin_num = 16;
// 一个regsion中维护的内存page数量
constexpr uint32_t region_num = 64;

// 内存大小基准
constexpr uint32_t quantum = 4; // 基础大小, 单位字节
constexpr uint32_t tiny = 1; // 最小大小, 单位字节
constexpr uint32_t page = 4 << 10; // 页大小, 单位字节 4KB


using mem_size_t = uint32_t;
// size_class 的划分方式
constexpr uint32_t small_alloc_bound = 130;
constexpr uint32_t large_alloc_bound = 257;
constexpr uint32_t huge_alloc_bound = 347;
// 各个 size_class 的个数 (最值)
constexpr uint32_t small_block_num = 10;
constexpr uint32_t large_block_num = 10;
constexpr uint32_t huge_block_num = 5;
constexpr uint32_t page_byte_size = 4 << 10;    // 4KB
constexpr uint32_t big_page_btye_size = 4 << 20;// 4MB
       

struct MemConfig{
    std::size_t size;
    std::size_t max_num;
    constexpr MemConfig(std::size_t size, std::size_t max_num)
        : size(size), max_num(max_num){}
};

static constexpr std::array<MemConfig, 122> size_class = {{
    // 1-128字节（8192个块）
    {1, 8192}, {2, 8192}, {4, 8192}, {8, 8192},
    {16, 8192}, {24, 8192}, {32, 8192}, {48, 8192},
    {64, 8192}, {80, 8192}, {96, 8192}, {112, 8192}, {128, 8192},
    
    // 160-2048字节（4096个块）
    {160, 4096}, {192, 4096}, {224, 4096}, {256, 4096},
    {288, 4096}, {320, 4096}, {352, 4096}, {384, 4096},
    {416, 4096}, {448, 4096}, {480, 4096}, {512, 4096},
    {576, 4096}, {640, 4096}, {704, 4096}, {768, 4096},
    {832, 4096}, {896, 4096}, {960, 4096}, {1024, 4096},
    {1088, 4096}, {1152, 4096}, {1216, 4096}, {1280, 4096},
    {1344, 4096}, {1408, 4096}, {1472, 4096}, {1536, 4096},
    {1600, 4096}, {1664, 4096}, {1728, 4096}, {1792, 4096},
    {1856, 4096}, {1920, 4096}, {1984, 4096}, {2048, 4096},
    
    // 2304-8192字节（2048个块）
    {2304, 2048}, {2560, 2048}, {2816, 2048}, {3072, 2048},
    {3328, 2048}, {3584, 2048}, {3840, 2048}, {4096, 2048},
    {4352, 2048}, {4608, 2048}, {4864, 2048}, {5120, 2048},
    {5376, 2048}, {5632, 2048}, {5888, 2048}, {6144, 2048},
    {6400, 2048}, {6656, 2048}, {6912, 2048}, {7168, 2048},
    {7424, 2048}, {7680, 2048}, {7936, 2048}, {8192, 2048},
    
    // 8704-16384字节（1024个块）
    {8704, 1024}, {9216, 1024}, {9728, 1024}, {10240, 1024},
    {10752, 1024}, {11264, 1024}, {11776, 1024}, {12288, 1024},
    {12800, 1024}, {13312, 1024}, {14080, 1024}, {14592, 1024},
    {15104, 1024}, {15616, 1024}, {16128, 1024}, {16384, 1024},
    
    // 基于page_byte_size的配置（512个块）
    {2*page_byte_size, 512}, {4*page_byte_size, 512}, 
    {8*page_byte_size, 512}, {12*page_byte_size, 512},
    {16*page_byte_size, 512}, {20*page_byte_size, 512}, 
    {24*page_byte_size, 512}, {28*page_byte_size, 512},
    {32*page_byte_size, 512},
    
    // 基于page_byte_size的配置（256个块）
    {40*page_byte_size, 256}, {48*page_byte_size, 256},
    {56*page_byte_size, 256}, {64*page_byte_size, 256},
    {72*page_byte_size, 256}, {80*page_byte_size, 256},
    {88*page_byte_size, 256}, {96*page_byte_size, 256},
    
    // 基于page_byte_size的配置（128个块）
    {128*page_byte_size, 128}, {160*page_byte_size, 128},
    {190*page_byte_size, 128}, {212*page_byte_size, 128},
    {244*page_byte_size, 128}, {276*page_byte_size, 128},
    
    // 基于big_page_btye_size的配置（64个块）
    {1*big_page_btye_size, 64}, {2*big_page_btye_size, 64},
    {4*big_page_btye_size, 64}, {8*big_page_btye_size, 64},
    
    // 基于big_page_btye_size的配置（32个块）
    {16*big_page_btye_size, 32}, {32*big_page_btye_size, 32}, 
    {64*big_page_btye_size, 32},
    
    // 基于big_page_btye_size的配置（16个块）
    {16*big_page_btye_size, 16}, {32*big_page_btye_size, 16},
    
    // 基于big_page_btye_size的配置（8个块）
    {64*big_page_btye_size, 8}
}};

