#ifndef __POOL_ALLOCATOR_HPP__
#define __POOL_ALLOCATOR_HPP__

#include <cstdio>
#include <memory>
#include "lt_core.hpp"
#include "lt_utils.hpp"

namespace memory
{

struct PoolAllocator
{
    PoolAllocator(void *mem, usize pool_size, usize block_size, usize block_alignment);

    void *allocate(usize size);
    void deallocate(void *ptr);

    inline usize num_blocks() const
    {
        return m_num_blocks;
    }

    PoolAllocator& operator=(const PoolAllocator&) = delete;

public:
    const usize block_size;

private:
    void   *m_mem;
    void   *m_aligned_mem;
    usize   m_pool_size;
    i32     m_num_blocks;
    void  **m_free_list;
    usize   m_free_list_size;
};

template<typename T, typename Allocator, typename ...Args> T *
allocate_and_construct(Allocator &allocator, Args&& ...args)
{
    void *ptr = allocator.allocate(sizeof(T));

    if (!ptr) return nullptr;

    return ::new(ptr) T(std::forward<Args>(args)...);
}

template<typename T, typename ...Args> T *
construct(void *ptr, Args&& ...args)
{
    return ::new(ptr) T(std::forward<Args>(args)...);
}

template<typename Allocator, typename T, typename ...Args> void
destroy_and_deallocate(Allocator &allocator, T *t)
{
    t->~T();
    allocator.deallocate(static_cast<void*>(t));
}

};

#endif // __POOL_ALLOCATOR_HPP__
