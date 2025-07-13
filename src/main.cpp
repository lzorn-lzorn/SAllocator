#include "../include/SAllocator.hpp"
#include <chrono>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include "../include/JAllocatorImpl/SysApi.h"


using namespace Stellatus;

constexpr size_t N = 10'000'000;
constexpr size_t ALLOC_SIZE = 64;
constexpr size_t LARGE_SIZE = 8ULL * 1024 * 1024 * 1024; // 8GB

template <typename Alloc>
void test_alloc(const std::string& name) {
    std::vector<void*> ptrs;
    ptrs.reserve(N);  // 避免 std::vector 本身频繁扩容干扰测试

    auto t1 = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < N; ++i) {
        void* p = Alloc{}.allocate(ALLOC_SIZE);
        std::memset(p, static_cast<int>(i), ALLOC_SIZE);  // 写入，防止优化
        ptrs.push_back(p);
    }


    for (void* p : ptrs) {
        Alloc{}.deallocate(static_cast<char*>(p), ALLOC_SIZE);
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> d = t2 - t1;
    std::cout << name << ": " << d.count() << " seconds" << std::endl;
}

void test_large_alloc(const std::string& name, auto alloc_fn, auto free_fn) {
    void* ptr = alloc_fn();

    if (!ptr) {
        std::cerr << "allocation failed for " << name << std::endl;
        return;
    }

    auto t1 = std::chrono::high_resolution_clock::now();

    // 强制访问每一页
    for (size_t offset = 0; offset < LARGE_SIZE; offset += 4096) {
        static_cast<volatile char*>(ptr)[offset] = 0xCD;
    }

    auto t2 = std::chrono::high_resolution_clock::now();

    free_fn(ptr);

    std::chrono::duration<double> d = t2 - t1;
    std::cout << name << " (8GB write): " << d.count() << " seconds" << std::endl;
}

template <typename Ty, std::size_t Num>
struct A{
    using value_type = Ty;
    static constexpr std::size_t max_size = Num;
    Ty a;
};

struct Proxy{

    virtual ~Proxy() = default;
};


template <typename Ty, std::size_t Num>
struct AProxy : Proxy{
    using value_type = Ty;
    static constexpr std::size_t max_size = Num;
    AProxy() {
        m_a.a = Ty();
    }
    A<Ty, Num> m_a;
};
using namespace OSAllocator;

bool is_aligned(void* ptr, size_t alignment) {
    return reinterpret_cast<uintptr_t>(ptr) % alignment == 0;
}

void test_allocation(size_t size, size_t alignment) {
    std::cout << "Allocating " << size << " bytes with alignment " << alignment << " ... ";

    void* ptr = OS_Alloc(size, alignment);
    // assert(ptr != nullptr && "OS_Alloc returned nullptr");
    // assert(is_aligned(ptr, alignment) && "Returned pointer is not aligned");

    // 写入测试：将整块内存置为某个值，检查可写性
    std::memset(ptr, 0xAB, size);

    // 读回测试：检查内容是否一致
    unsigned char* byte_ptr = static_cast<unsigned char*>(ptr);
    for (size_t i = 0; i < size; ++i) {
        assert(byte_ptr[i] == 0xAB);
    }

    // 释放
    OS_Free(ptr, size);

    std::cout << "OK\n";
}

int main() {
    std::cout << "========== OS_Alloc / OS_Free Test ==========\n";

    
    std::vector<size_t> alignments = {1, 2, 3, 17, 32, 64, 128, 256, 1024};
    std::vector<size_t> sizes = {
        0,        // edge case
        1,        // small
        100,      // below page size
        4096,     // page-aligned
        8192,     // multi-page
        2 * 1024 * 1024,        // huge page threshold
        4 * 1024 * 1024 + 123,  // above huge page threshold
    };

    for (auto align : alignments) {
        for (auto sz : sizes) {
            try {
                test_allocation(sz, align);
            } catch (const std::exception& e) {
                std::cerr << "FAILED: " << e.what() << "\n";
            }
        }
    }

    std::cout << "========== All Tests Passed ==========\n";
    return 0;
}

