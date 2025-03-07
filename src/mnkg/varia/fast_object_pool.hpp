// This class implements a fixed-size pool of objects that can be allocated and
// deallocated in constant time.
// Speed was valued over safety and functionality, as the pool was made for its
// use in a controlled, performance-critical context: the MCTS algorithm found
// in this project.

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mnkg {

template <typename T>
class fast_object_pool {
public:
        explicit fast_object_pool(std::size_t capacity) : capacity_(capacity)
        {
                assert(capacity_ > 1 && "what are you doing?");

                allocator_ = std::allocator<T>();
                memory_    = allocator_.allocate(capacity_); // may throw

                free_.reserve(capacity_);
                for (std::size_t i = 0; i < capacity_; ++i)
                        free_.push_back(&memory_[i]);
        }

        ~fast_object_pool()
        {
                // WARNING: The user is responsible for deallocating all
                // objects. Otherwise, they are deleted without proper
                // destruction.
                assert(free_.size() == capacity_);
                allocator_.deallocate(memory_, capacity_);
        }

        fast_object_pool(const fast_object_pool &) = delete;
        fast_object_pool(fast_object_pool &&)      = default;

        inline std::size_t // maximum number of allocable objects
        capacity() const
        {
                return capacity_;
        }

        inline std::size_t // number of allocated objects
        size() const
        {
                return capacity_ - free_.size();
        }

        inline bool
        empty() const
        {
                return free_.size() == capacity_;
        }

        inline bool
        full() const
        {
                return size() == capacity_;
        }

        template <typename... Args>
        T *
        allocate(Args &&...args)
        // WARNING: * Not thread-safe.
        //          * If pool is full, returns nullptr, or aborts (debug mode).
        {
                assert(not full() && "pool is full");
                if (free_.empty())
                        return nullptr;
                T *location = free_.back();
                free_.pop_back();
                std::construct_at(location, std::forward<Args>(args)...);
                return location;
        }

        void
        deallocate(T *location)
        // WARNING: * Not thread-safe.
        {
                assert(is_chunk_(location) && "invalid pool-object");
                std::destroy_at(location);
                free_.push_back(location);
        }

private:
        const std::size_t capacity_;
        std::allocator<T> allocator_;
        T                *memory_;
        std::vector<T *>  free_;

        inline bool
        is_chunk_(const T *obj) const
        {
                if (!memory_ || !obj)
                        return false;

                auto index = obj - memory_; // pointer arithmetic
                bool inside
                    = index >= 0 && static_cast<std::size_t>(index) < capacity_;
                bool aligned
                    = (reinterpret_cast<std::uintptr_t>(obj) % alignof(T)) == 0;

                return inside && aligned;
        }

public:
        struct deleter {
                fast_object_pool *pool;
                void
                operator()(T *ptr) const
                {
                        if (!ptr)
                                return;
                        assert(pool && "no pool set");
                        pool->deallocate(ptr);
                }
                // WARNING: no deleter should outlive its assigned pool.
        };
};

} // namespace mnkg
