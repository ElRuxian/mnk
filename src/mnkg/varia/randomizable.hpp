#pragma once

#include <concepts>

namespace mnkg {

template <typename T>
T
make_random()
{
        return T::make_random();
}

template <typename T>
concept randomizable = requires
{
        {
                make_random<T>()
        } -> std::same_as<T>;
};

} // namespace mnkg
