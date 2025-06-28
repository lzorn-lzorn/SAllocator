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
    virtual constexpr Ty* allocate(std::size_t size);
    virtual void deallocate(Ty* ptr, std::size_t size);
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
    Ty* allocate(size_t size) override {

        return nullptr;
    }
    void deallocate(Ty* ptr, size_t size) override {

    }
};