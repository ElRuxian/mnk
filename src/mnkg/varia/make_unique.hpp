// Custom std::make_unique upgrade: takes the memory_resource to be used

#include <memory>

namespace mnkg {

template <class Memory, class T, class... Args>
auto
make_unique_from(Memory &resource, Args &&...args)
{
        auto deleter = [&resource](T *p) { resource.deallocate(p, 1); };
}

} // namespace mnkg
