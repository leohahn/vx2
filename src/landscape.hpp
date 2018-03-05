#ifndef __LANDSCAPE_HPP__
#define __LANDSCAPE_HPP__

#include <atomic>
#include <thread>
#include <functional>
#include "pool_allocator.hpp"

#include "lt_core.hpp"
#include "lt_math.hpp"
#include "pool_allocator.hpp"
#include "semaphore.hpp"
#include "vertex.hpp"

struct TextureAtlas;
struct Camera;
struct ResourceManager;
struct osn_context;
struct ChunkNoise;
struct Memory;

enum BlockType
{
    BlockType_Air,
    BlockType_Terrain,
    BlockType_Count,
};

struct BlocksTextureInfo
{
    enum Layer
    {
        Earth_Sides = 0,
        Earth_Sides_Top = 1,
        Earth_Top = 2,
        Earth_Bottom = 3,

        Snow_Sides = 4,
        Snow_Sides_Top = 5,
        Snow_Top = 6,
        Snow_Bottom = 7,
    };

    BlocksTextureInfo(const char *texture_name, const ResourceManager &manager);
    bool load();
    u32 texture_id() const;

private:
    TextureAtlas *m_texture;
};

typedef i32 ChunkIndex;

static_assert(std::atomic<bool>::is_always_lock_free);
static_assert(std::atomic<i32>::is_always_lock_free);

struct Landscape
{
    constexpr static i32 NUM_CHUNKS_X = 14;
    constexpr static i32 NUM_CHUNKS_Y = 7;
    constexpr static i32 NUM_CHUNKS_Z = 14;
    constexpr static i32 NUM_CHUNKS = NUM_CHUNKS_X*NUM_CHUNKS_Y*NUM_CHUNKS_Z;

    struct Chunk;
private:
    // -----------------------------------------------------------------
    // Queue definition for asynchronously loading chunks
    // -----------------------------------------------------------------
    struct QueueRequest
    {
        // TODO: As soon as multiple threads start modifying this object,
        // introduce a mutex.
        QueueRequest(Chunk *chunk) : chunk(chunk), processed(false) {}

        Chunk *chunk;
        std::atomic<bool> processed;
        std::vector<Vertex_PLN> vertexes;
    };

    struct ChunkQueue
    {
        static constexpr i32 MAX_ENTRIES = 1600; // TODO: figure out the best number here.

        struct Request
        {
            Chunk *chunk;
        };

        void insert(const std::shared_ptr<QueueRequest> &request);
        inline bool is_empty() const { return read_index == write_index; }
        std::shared_ptr<QueueRequest> take_next_request();
        std::mutex mutex;

        // The entries array is a circular FIFO queue.
        std::shared_ptr<QueueRequest> requests[MAX_ENTRIES];
        std::atomic<i32> read_index = 0;
        std::atomic<i32> write_index = 0;
        Semaphore        semaphore;
    };

public:
    struct VAOArray
    {
        struct Entry
        {
            const u32 vao;
            const u32 vbo;
            u32 num_vertices_used;
            bool is_used;
            Entry();
        };

        isize take_free_entry();
        void free_entry(isize index);

        Entry vaos[NUM_CHUNKS];
    };

    struct ChunksMutex
    {
        void lock_high_priority()
        {
            LT_Assert(m_hpt_waiting == false);
            m_hpt_waiting = true;
            m_mutex.lock();
            m_hpt_waiting = false;
        }
        void unlock_high_priority()
        {
            m_hpt_done_signal.notify_one();
            m_mutex.unlock();
        }
        void lock_low_priority()
        {
            std::unique_lock<decltype(m_mutex)> lock(m_mutex);
            while (m_hpt_waiting)
                m_hpt_done_signal.wait(lock);
            lock.release();
        }
        void unlock_low_priority()
        {
            m_hpt_done_signal.notify_one();
            m_mutex.unlock();
        }
    private:
        std::mutex m_mutex;
        std::atomic_bool m_hpt_waiting = false;
        std::condition_variable m_hpt_done_signal;
    };

    struct Chunk
    {
        constexpr static i32 NUM_BLOCKS_PER_AXIS = 16;
        constexpr static i32 NUM_BLOCKS = NUM_BLOCKS_PER_AXIS*NUM_BLOCKS_PER_AXIS*NUM_BLOCKS_PER_AXIS;
        constexpr static i32 BLOCK_SIZE = 1;
        constexpr static i32 SIZE = BLOCK_SIZE * NUM_BLOCKS_PER_AXIS;

        Chunk(Vec3f origin, VAOArray *vao_array, BlockType fill_type = BlockType_Air);
        ~Chunk();
        Chunk(Chunk &chunk) = delete;
        Chunk &operator=(const Chunk &chunk) = delete;

        void create_request();
        void cancel_request();

    public:
        BlockType blocks[NUM_BLOCKS_PER_AXIS][NUM_BLOCKS_PER_AXIS][NUM_BLOCKS_PER_AXIS];
        Vec3f     origin;
        isize     entry_index;
        std::atomic_bool outdated; // Signals that the chunk's buffer must be updated.
        std::shared_ptr<QueueRequest> request;
    private:
        VAOArray *m_vao_array;
    };

    using ChunkPtr = std::unique_ptr<Chunk,std::function<void(Chunk*)>>;
    // using ChunkPtr = std::unique_ptr<Chunk>;

    constexpr static i32 TOTAL_BLOCKS_X = NUM_CHUNKS_X * Chunk::NUM_BLOCKS_PER_AXIS;
    constexpr static i32 TOTAL_BLOCKS_Y = NUM_CHUNKS_Y * Chunk::NUM_BLOCKS_PER_AXIS;
    constexpr static i32 TOTAL_BLOCKS_Z = NUM_CHUNKS_Z * Chunk::NUM_BLOCKS_PER_AXIS;

    constexpr static i32 SIZE_X = TOTAL_BLOCKS_X * Chunk::BLOCK_SIZE;
    constexpr static i32 SIZE_Y = TOTAL_BLOCKS_Y * Chunk::BLOCK_SIZE;
    constexpr static i32 SIZE_Z = TOTAL_BLOCKS_Z * Chunk::BLOCK_SIZE;


    Landscape(Memory &memory, i32 seed, f64 amplitude, f64 frequency,
              i32 num_octaves, f64 lacunarity, f64 gain);
    ~Landscape();

    // A landscape cannot be copied.
    Landscape(const Landscape&) = delete;
    Landscape(const Landscape&&) = delete;
    Landscape &operator=(const Landscape&) = delete;
    Landscape &operator=(const Landscape&&) = delete;

    std::vector<Vertex_PLN> update_chunk_buffer(Chunk *chunk);
    bool block_exists(i32 abs_block_xi, i32 abs_block_yi, i32 abs_block_zi);
    void update(const Camera &camera);
    void generate();

    inline Vec3f center() const
    {
        return origin + 0.5f*Vec3f(SIZE_X, SIZE_Y, SIZE_Z);
    }

private:
    void chunk_deleter(Chunk *chunk);

public:
    ChunksMutex chunks_mutex;

    // Chunks matrix.
    ChunkPtr chunk_ptrs[NUM_CHUNKS_X][NUM_CHUNKS_Y][NUM_CHUNKS_Z];

    // Where the (left, bottom, back) corner of the landscape starts.
    Vec3f   origin;

    // Structure that contains all of the VBOs and VAOs necessary to render
    // the chunks.
    VAOArray vao_array;

private:
    const i32           m_seed;
    const f64           m_amplitude;
    const f64           m_frequency;
    const i32           m_num_octaves;
    const f64           m_lacunarity;
    const f64           m_gain;

    // TODO: Instead of hardcoding the number of threads, decide this at runtime
    // which number is the best.
    static constexpr i32 M_NUM_THREADS = 4;
    std::thread          m_threads[M_NUM_THREADS];
    std::atomic<bool>    m_threads_should_run;

    osn_context  *m_simplex_ctx;

    memory::PoolAllocator m_chunks_allocator;

    void initialize_chunks();
    void initialize_threads();
    Vec3f get_chunk_origin(i32 cx, i32 cy, i32 cz);
    ChunkNoise *fill_noise_map_for_chunk_column(f32 origin_x, f32 origin_z);
    void do_chunk_generation_work(Chunk *chunk);
    void run_worker_thread();
    void stop_threads();
    void pass_chunk_buffer_to_gpu(const VAOArray::Entry &entry, const std::vector<Vertex_PLN> &buf);

    // // Queue that contains the chunks that need to be loaded by the threads.
    ChunkQueue m_chunks_to_process_queue;
    ChunkQueue m_chunks_processed_queue;
};

#endif // __LANDSCAPE_HPP__
