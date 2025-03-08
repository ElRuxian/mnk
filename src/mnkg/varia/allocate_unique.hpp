// Taken from https://stackoverflow.com/a/23132307
// Style changes were made.

#include <memory>

template <typename Allocator>
class alloc_deleter {
public:
        alloc_deleter(const Allocator &a) : allocator_(a) {}

        typedef typename std::allocator_traits<Allocator>::pointer pointer;

        void
        operator()(pointer p) const
        {
                Allocator allocator(allocator_);
                std::allocator_traits<Allocator>::destroy(allocator,
                                                          std::addressof(*p));
                std::allocator_traits<Allocator>::deallocate(allocator, p, 1);
        }

private:
        Allocator allocator_;
};

template <typename T, typename Allocator, typename... Args>
auto
allocate_unique(const Allocator &alloc, Args &&...args)
{
        using traits = std::allocator_traits<Allocator>;
        static_assert(
            std::is_same<typename traits::value_type, std::remove_cv_t<T> >{}(),
            "Allocator has the wrong value_type");

        Allocator allocator(alloc);
        auto      ptr = traits::allocate(allocator, 1);
        try {
                traits::construct(allocator,
                                  std::addressof(*ptr),
                                  std::forward<Args>(args)...);
                using deleter = alloc_deleter<Allocator>;
                return std::unique_ptr<T, deleter>(ptr, deleter(allocator));
        } catch (...) {
                traits::deallocate(allocator, ptr, 1);
                throw;
        }
}
