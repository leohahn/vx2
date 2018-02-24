#ifndef __POOL_ALLOCATOR_HPP__
#define __POOL_ALLOCATOR_HPP__

#include <cstdio>
#include <memory>
#include "lt_core.hpp"
#include "lt_utils.hpp"

template<typename T>
struct PoolAllocator
{
    using value_type = T;
    using pointer = value_type*;

    PoolAllocator(void *mem, usize pool_size, usize block_size, usize block_alignment)
        : block_size(block_size)
        , m_mem(mem)
        , m_aligned_mem(mem)
        , m_pool_size(pool_size)
    {
        LT_Assert(block_size <= pool_size);

        // logger.log("Block size: ", block_size);
        // logger.log("Block alignment: ", block_alignment);

        // const usize old_size = m_pool_size;
        if (std::align(block_alignment, block_size, m_aligned_mem, m_pool_size))
        {
            // logger.log("Pool allocator aligned address by adjusting ", old_size - m_pool_size, " bytes");
            LT_Assert(m_pool_size % block_size == 0);

            m_num_blocks = m_pool_size / block_size;
            // logger.log("number of blocks: ", m_num_blocks);

            m_free_list = (void**)m_aligned_mem;

            //
            // Initialize free list with all the blocks.
            // NOTE: This code uses the freed blocks as storage to place the free list.
            //
            void **it = m_free_list;
            for (i32 i = 0; i < m_num_blocks-1; i++)
            {
                *it = ((u8*)(it) + block_size);
                it = (void**)(*it);
            }
        }
        else
        {
            LT_Panic("Failed to align the Pool Allocator.");
        }
    }

    pointer allocate(usize size)
    {
        LT_Assert(size == block_size);

        if (!m_free_list) return nullptr;

        // Take first element of the list.
        void *ptr = m_free_list;
        // Remove the first element of the list.
        m_free_list = (void**)(*m_free_list);

        return static_cast<pointer>(ptr);
    }

    void deallocate(value_type *ptr)
    {
        // TODO: Make an assertion that the passed pointer is owned by this allocator.
        void **head = (void**)ptr;
        *head = m_free_list;
        m_free_list = head;
    }

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
};

#endif // __POOL_ALLOCATOR_HPP__
