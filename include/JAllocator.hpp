#pragma once
#include <cstdlib>
#include <type_traits>
template <typename Ty>
class Allocator{
public:
    using value_type = Ty;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment  = std::true_type;
public:
    virtual constexpr Ty* allocate(std::size_t size) = 0;
    virtual void deallocate(Ty* ptr, std::size_t size) = 0;
};

template <typename Ty>
class JAllocator : Allocator<Ty>{
public:
    using base_type = Allocator<Ty>;
    using value_type = Ty;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment  = std::true_type;
public:
    Ty* allocate(size_t num) override {
        value_type* mem_ptr = j_malloc(num * sizeof(value_type));
        return mem_ptr;
    }
    void deallocate(Ty* ptr, size_t size) override {

    }
private: 
    void * j_malloc(size_t size){

        return nullptr;
    }
};