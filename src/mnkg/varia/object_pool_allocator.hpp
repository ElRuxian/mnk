// Adapts slab_memory.hpp to be type-oriented (instead of size-oriented)
// and conform to the "Allocator" named requirement.
// See slab_memory.hpp.

#include "slab_memory.hpp"
#include <type_traits>

namespace mnkg {

template <typename T>
class object_pool_allocator {
public:
        using value_type      = T;
        using is_always_equal = std::false_type; // stateful allocator
        // other traits are defaulted (as in std::allocator_traits<object_pool>)

private:
        slab_memory<sizeof(T)> *memory_; // non-owning

public:
        [[nodiscard]] inline T *
        allocate(std::size_t n)
        {
                return static_cast<T *>(
                    memory_->allocate(n * sizeof(T), alignof(T)));
        };

        static inline consteval std::size_t
                                max_size()
        {
                return 1; // the slab memory does not support array allocation
        }

        inline void
        deallocate(T *p, std::size_t n) noexcept
        {
                memory_->deallocate(p, n * sizeof(T));
        }

public:
        bool
        operator==(const object_pool_allocator &) const
            = default;

        bool
        operator!=(const object_pool_allocator &) const
            = default;

        object_pool_allocator(auto *memory) : memory_(memory) {}
        object_pool_allocator(const object_pool_allocator &) noexcept = default;
        object_pool_allocator(object_pool_allocator &&) noexcept      = default;
        ~object_pool_allocator() noexcept                             = default;

        object_pool_allocator &
        operator=(object_pool_allocator &&) noexcept
            = default;
};

} // namespace mnkg
