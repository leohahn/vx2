#include "pool_allocator.hpp"
#include "lt_utils.hpp"

lt_global_variable lt::Logger logger("pool_allocator");

memory::PoolAllocator::PoolAllocator(void *mem, usize pool_size, usize block_size, usize block_alignment)
    : block_size(block_size)
    , m_mem(mem)
    , m_aligned_mem(mem)
    , m_pool_size(pool_size)
{
    LT_Assert(block_size <= pool_size);

    // logger.log("Block size: ", block_size);
    // logger.log("Block alignment: ", block_alignment);

    const usize old_size = m_pool_size;
    if (std::align(block_alignment, block_size, m_aligned_mem, m_pool_size))
    {
        logger.log("Pool allocator aligned address by adjusting ", old_size - m_pool_size, " bytes");

        m_num_blocks = m_pool_size / block_size;

        logger.log("number of blocks: ", m_num_blocks);
        logger.log("wasted space ", BytesToKilobytes(m_pool_size % block_size), "K");

        m_free_list = (void**)m_aligned_mem;
        m_free_list_size = m_num_blocks;
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

void *
memory::PoolAllocator::allocate(usize size)
{
    LT_Assert(size == block_size);

    if (!m_free_list)
    {
        logger.log("Memory not available.");
        return nullptr;
    }

    // Take first element of the list.
    void *ptr = m_free_list;
    // Remove the first element of the list.
    m_free_list = (void**)(*m_free_list);
    m_free_list_size--;

    return ptr;
}

void
memory::PoolAllocator::deallocate(void *ptr)
{
    // TODO: Make an assertion that the passed pointer is owned by this allocator.
    void **head = (void**)ptr;
    *head = m_free_list;
    m_free_list = head;
    m_free_list_size++;
}
