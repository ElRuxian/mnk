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

                memory_ = std::make_unique<T[]>(capacity_);

                assert(memory_ && "memory allocation failed");
                for (std::size_t i = 0; i < capacity_; ++i)
                        free_.push_back(&memory_[i]);
        }

        ~fast_object_pool()
        {
                // WARNING: The user is responsible for deallocating all
                // objects. Otherwise, they are deleted without proper
                // destruction.
                assert(free_.size() == capacity_);
        }

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
                return free_.empty();
        }

        template <typename... Args>
        T *
        allocate(Args &&...args)
        // WARNING: * Not thread-safe.
        //          * If pool is full, returns nullptr, or aborts (debug mode).
        {
                assert(free_.size() > 0);
                if (free_.empty())
                        return nullptr;
                T *obj = free_.back();
                free_.pop_back();
                new (obj) T(std::forward<Args>(args)...);
                return obj;
        }

        void
        deallocate(T *obj)
        // WARNING: * Not thread-safe.
        {
                assert(is_chunk_(obj) && "invalid pool-object");
                obj->~T();
                free_.push_back(obj);
        }

private:
        std::unique_ptr<T[]> memory_; // Use unique_ptr for memory management
        const std::size_t    capacity_;
        std::vector<T *>     free_; // Track free objects as raw pointers

        inline bool
        is_chunk_(const T *obj) const
        {
                auto offset   = obj - &memory_[0];
                bool inside   = offset >= 0 && offset < capacity_;
                bool alligned = offset % sizeof(T) == 0;
                return inside && alligned;
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
