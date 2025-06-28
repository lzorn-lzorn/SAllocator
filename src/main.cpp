#include "../include/SAllocator.hpp"
#include <chrono>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>

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

int main() {


    // test_alloc<SAllocator<char>>("SAllocator");
    // test_alloc<std::allocator<char>>("std::allocator");

    // test_large_alloc("Stellutas malloc 8GB",
    //     [] { return tls_arena.allocate(LARGE_SIZE); },
    //     [](void* p) { tls_arena.deallocate(p, LARGE_SIZE); });

    // test_large_alloc("std::malloc 8GB",
    //     [] {
    //         void* p = std::malloc(LARGE_SIZE);
    //         for (size_t offset = 0; offset < LARGE_SIZE; offset += 4096)
    //             static_cast<char*>(p)[offset] = 0xCD; // 强制写入每一页
    //         return p;
    //     },
    //     [](void* p) { std::free(p); });
}
