// Implementation of a simple slab memory resource.
// Similar to std::pmr::unsynchronized_pool_resource, but simpler.
// Features:
// - Single memory pool; only manages one block (slab) size.
// - Consteval slab size.
// - No resizing (fixed slab count).
// - No internal fragmentation (disallows small allocations).
// - No thread safety.
// - Few integrity checks.
// Made for its use in the performance-critical MCTS module.
// Speed was prioritized over safety, use with caution.

#include <cstddef>
#include <memory>
#include <memory_resource>
#include <vector>

namespace mnkg {

template <std::size_t SlabSize>
class slab_memory : public std::pmr::memory_resource {
        using slab = std::byte[SlabSize];
        std::unique_ptr<slab[]> slabs_;
        std::vector<slab *>     free_;

        void *
        do_allocate(std::size_t bytes, std::size_t alignment) override
        // WARNING: Not thread-safe.
        {
                if (not valid(bytes, alignment) || full())
                        throw std::bad_alloc();
                auto *slab = free_.back();
                free_.pop_back();
                return slab;
        }

        void
        do_deallocate(void *p, std::size_t bytes,
                      std::size_t alignment) override
        // WARNING: * Not thread-safe.
        //          * Double free is undefined behavior.
        {
                if (!valid(p, bytes, alignment))
                        throw std::bad_alloc();
                free_.push_back(reinterpret_cast<slab *>(p));
        }

        inline bool
        valid(std::size_t bytes, std::size_t alignment) const
        {
                bool fits_perfectly = bytes == SlabSize;
                bool aligned = (alignment <= SlabSize) && (alignment != 0)
                               && ((SlabSize % alignment) == 0);
                return fits_perfectly && aligned;
        }

        inline bool
        valid(void *p, std::size_t bytes, std::size_t alignment) const
        {
                void *begin    = slabs_.get();
                void *end      = slabs_.get() + free_.capacity();
                bool  in_range = (p >= begin) && (p < end);
                auto  offset   = reinterpret_cast<uintptr_t>(p)
                              - reinterpret_cast<uintptr_t>(begin);
                bool slab_start = offset % SlabSize == 0;
                return p && in_range && slab_start && valid(bytes, alignment);
        }

        bool
        do_is_equal(
            const std::pmr::memory_resource &other) const noexcept override
        {
                return this == &other;
        }

public:
        explicit slab_memory(std::size_t slab_count) :
                slabs_(std::make_unique<slab[]>(slab_count))
        {
                free_.reserve(slab_count);
                for (auto i = 0u; i < slab_count; ++i)
                        free_.push_back(slabs_.get() + i);
        }
        slab_memory(const slab_memory &) = delete;
        slab_memory(slab_memory &&)      = delete;
        ~slab_memory()
        {
                if (!empty())
                        throw std::logic_error("slab memory not empty");
        };

        inline auto
        free_slab_count() const noexcept
        {
                return free_.size();
        }

        inline auto
        max_free_slab_count() const noexcept
        {
                return free_.capacity();
        }

        inline bool
        full() const noexcept
        {
                return free_.empty();
        }

        inline bool
        empty() const noexcept
        {
                return free_.size() == free_.capacity();
        }
};

} // namespace mnkg
