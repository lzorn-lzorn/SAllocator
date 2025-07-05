#pragma once
#include <tuple>
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
      
// MaxSize 这么大的块, 有MaxNum块
template <std::size_t MaxSize, std::size_t MaxNum>
struct MemConfig{
    constexpr static std::size_t size = MaxSize;
    constexpr static std::size_t max_num =  MaxNum;
};

static constexpr std::tuple<
    MemConfig<1, 8192>, 
    MemConfig<2, 8192>, 
    MemConfig<4, 8192>, 
    MemConfig<8, 8192>,
    MemConfig<16, 8192>,
    MemConfig<24, 8192>,
    MemConfig<32, 8192>,
    MemConfig<48, 8192>,
    MemConfig<64, 8192>,
    MemConfig<80, 8192>,
    MemConfig<96, 8192>,
    MemConfig<16, 8192>,
    MemConfig<24, 8192>,
    MemConfig<32, 8192>,
    MemConfig<48, 8192>,
    MemConfig<64, 8192>,
    MemConfig<80, 8192>,
    MemConfig<96, 8192>,
    MemConfig<112, 8192>,
    MemConfig<128, 8192>,
    MemConfig<160, 4096>,
    MemConfig<192, 4096>,
    MemConfig<224, 4096>,
    MemConfig<256, 4096>,
    MemConfig<288, 4096>,
    MemConfig<320, 4096>,
    MemConfig<352, 4096>,
    MemConfig<384, 4096>,
    MemConfig<416, 4096>,
    MemConfig<448, 4096>,
    MemConfig<480, 4096>,
    MemConfig<512, 4096>,
    MemConfig<576, 4096>,
    MemConfig<640, 4096>,
    MemConfig<704, 4096>,
    MemConfig<768, 4096>,
    MemConfig<832, 4096>,
    MemConfig<896, 4096>,
    MemConfig<960, 4096>,
    MemConfig<1024, 4096>,
    MemConfig<1088, 4096>,
    MemConfig<1152, 4096>,
    MemConfig<1216, 4096>,
    MemConfig<1280, 4096>,
    MemConfig<1344, 4096>,
    MemConfig<1408, 4096>,
    MemConfig<1472, 4096>,
    MemConfig<1536, 4096>,
    MemConfig<1600, 4096>,
    MemConfig<1664, 4096>,
    MemConfig<1728, 4096>,
    MemConfig<1792, 4096>,
    MemConfig<1856, 4096>,
    MemConfig<1920, 4096>,
    MemConfig<1984, 4096>,
    MemConfig<2048, 4096>,
    MemConfig<2304, 2048>,
    MemConfig<2560, 2048>,
    MemConfig<2816, 2048>,
    MemConfig<3072, 2048>,
    MemConfig<3328, 2048>,
    MemConfig<3584, 2048>,
    MemConfig<3840, 2048>,
    MemConfig<4096, 2048>,
    MemConfig<4352, 2048>,
    MemConfig<4608, 2048>,
    MemConfig<4864, 2048>,
    MemConfig<5120, 2048>,
    MemConfig<5376, 2048>,
    MemConfig<5632, 2048>,
    MemConfig<5888, 2048>,
    MemConfig<6144, 2048>,
    MemConfig<6400, 2048>,
    MemConfig<6656, 2048>,
    MemConfig<6912, 2048>,
    MemConfig<7168, 2048>,
    MemConfig<7424, 2048>,
    MemConfig<7680, 2048>,
    MemConfig<7936, 2048>,
    MemConfig<8192, 2048>,
    MemConfig<8704, 1024>,
    MemConfig<9216, 1024>,
    MemConfig<9728, 1024>,
    MemConfig<10240, 1024>,
    MemConfig<10752, 1024>,
    MemConfig<11264, 1024>,
    MemConfig<11776, 1024>,
    MemConfig<12288, 1024>,
    MemConfig<12800, 1024>,
    MemConfig<13312, 1024>,
    MemConfig<14080, 1024>,
    MemConfig<14592, 1024>,
    MemConfig<15104, 1024>,
    MemConfig<15616, 1024>,
    MemConfig<16128, 1024>,
    MemConfig<16384, 1024>,
    MemConfig<2*page_byte_size, 512>,
    MemConfig<4*page_byte_size, 512>,
    MemConfig<8*page_byte_size, 512>,
    MemConfig<12*page_byte_size, 512>,
    MemConfig<16*page_byte_size, 512>,
    MemConfig<20*page_byte_size, 512>,
    MemConfig<24*page_byte_size, 512>,
    MemConfig<28*page_byte_size, 512>,
    MemConfig<32*page_byte_size, 512>,
    MemConfig<40*page_byte_size, 256>,
    MemConfig<48*page_byte_size, 256>,
    MemConfig<56*page_byte_size, 256>,
    MemConfig<64*page_byte_size, 256>,
    MemConfig<72*page_byte_size, 256>,
    MemConfig<80*page_byte_size, 256>,
    MemConfig<88*page_byte_size, 256>,
    MemConfig<96*page_byte_size, 256>,
    MemConfig<128*page_byte_size, 128>,
    MemConfig<160*page_byte_size, 128>,
    MemConfig<190*page_byte_size, 128>,
    MemConfig<212*page_byte_size, 128>,
    MemConfig<244*page_byte_size, 128>,
    MemConfig<276*page_byte_size, 128>,
    MemConfig<1*big_page_btye_size, 64>,
    MemConfig<2*big_page_btye_size, 64>,
    MemConfig<4*big_page_btye_size, 64>,
    MemConfig<8*big_page_btye_size, 64>,
    MemConfig<16*big_page_btye_size, 32>,
    MemConfig<32*big_page_btye_size, 32>,
    MemConfig<64*big_page_btye_size, 32>,
    MemConfig<16*big_page_btye_size, 16>,
    MemConfig<32*big_page_btye_size, 16>,
    MemConfig<64*big_page_btye_size, 8>
> MemPoolConfig;
