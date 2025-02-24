// Custom implemenation driven by project-specific requirements.
// Minimal and simple. Doesn't aim for functionality beyond this project;
// e.g: neglected performance when handling large vectors.
// Seek linear algebra libraries for more comprehensive solutions.

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <ostream>
#include <ranges>
#include <type_traits>

#include "range_formatter.hpp"

namespace mnkg {

template <typename Component, size_t Dimension> // clang-format off
        requires(std::is_arithmetic_v<Component> && Dimension > 0)
class point { // clang-format on
private:
        std::array<Component, Dimension> components_;

public:
        using component                 = Component;
        static constexpr auto dimension = Dimension;

        constexpr
        point()
            = default;

        // clang-format off

        template <typename... T>
                requires(sizeof...(T) == Dimension)
        constexpr
        point(T... args) :
                components_{ args... }
        {
        }

        // clang-format on

        template <typename Range>
        explicit point(Range &&range)
        {
                assert(std::size(range) == Dimension);
                std::ranges::copy(range, components_.begin());
        }

        constexpr
        point(const point &other) noexcept
            = default;

        constexpr
        point(point &&other) noexcept
            = default;

        ~point() noexcept = default;

        inline decltype(auto) operator[](this auto &&self, size_t index)
        {
                assert(index < self.dimension);
                return self.components_[index];
        }

        decltype(auto) constexpr begin(this auto &&self) noexcept
        {
                return self.components_.begin();
        }
        decltype(auto) constexpr end(this auto &&self) noexcept
        {
                return self.components_.end();
        }

        friend void
        swap(point &lhs, point &rhs)
        {
                using std::swap;
                swap(lhs.components_, rhs.components_);
        }

        bool
        operator==(const point &other) const noexcept
        {
                return components_ == other.components_;
        }

        auto &
        operator=(point other)
        {
                // Copy-and-swap idiom
                swap(*this, other);
                return *this;
        }

#define BINARY_ASSIGNMENT_OPERATOR(op, operand_type, transformation)           \
        point &operator op(const operand_type & operand)                       \
        {                                                                      \
                using std::ranges::transform;                                  \
                if constexpr (std::same_as<operand_type, point>) {             \
                        transform(components_,                                 \
                                  operand.components_,                         \
                                  components_.begin(),                         \
                                  transformation<>());                         \
                        return *this;                                          \
                } else if constexpr (std::same_as<operand_type, Component>) {  \
                        transform(components_,                                 \
                                  components_.begin(),                         \
                                  bind_front(transformation<>(), operand));    \
                        return *this;                                          \
                }                                                              \
        }
        BINARY_ASSIGNMENT_OPERATOR(+=, point, std::plus)
        BINARY_ASSIGNMENT_OPERATOR(-=, point, std::minus)
        BINARY_ASSIGNMENT_OPERATOR(*=, Component, std::multiplies)
        BINARY_ASSIGNMENT_OPERATOR(/=, Component, std::divides)
#undef BINARY_ASSIGNMENT_OPERATOR
};

template <typename T, class T_ = std::remove_cvref_t<T> >
concept point_c
    = std::is_same_v<T_, point<typename T_::component, T_::dimension> >;

template <std::size_t I, mnkg::point_c Point>
auto &
get(Point &point)
{
        static_assert(I < Point::dimension, "Index out of bounds for Point");
        return point[I];
}

template <std::size_t I, mnkg::point_c Point>
const auto &
get(const Point &point)
{
        static_assert(I < Point::dimension, "Index out of bounds for Point");
        return point[I];
}

auto
operator-(const point_c auto &point)
{
        using point_t = std::remove_cvref_t<decltype(point)>;
        using namespace std::ranges;

        return point | views::transform(std::negate<>{}) | to<point_t>();
}

#define BINARY_OPERATOR(op)                                                    \
        template <point_c LHS, typename RHS>                                   \
        [[nodiscard]] constexpr auto operator op(const LHS &lhs,               \
                                                 const RHS &rhs)               \
        {                                                                      \
                return auto(lhs) op## = rhs;                                   \
        }
// point-point:
BINARY_OPERATOR(+)
BINARY_OPERATOR(-)
// point-scalar:
BINARY_OPERATOR(*)
BINARY_OPERATOR(/)
#undef BINARY_OPERATOR

std::ostream &
operator<<(std::ostream &ostream, const point_c auto &point)
{
        return ostream << std::format("{}", range_formatter(point));
}

enum class metric { euclidean, chebyshev };
template <metric Tag = metric::euclidean>
constexpr auto
norm(const point_c auto &point)
{
        if constexpr (Tag == metric::euclidean) {
                static_assert(point.dimension == 2
                              && "current implementation limited to 2D");
                return std::hypot(point[0], point[1]);
        } else if constexpr (Tag == metric::chebyshev) {
                using std::ranges::max_element;
                using std::views::transform;
                auto abs_values = transform(
                    point, [](const auto &value) { return std::abs(value); });
                return *max_element(abs_values);
        } else {
                static_assert(!"incomplete implementation");
        }
}

template <typename Range>
point(Range &&range) -> point<typename std::ranges::range_value_t<Range>,
                              decltype(std::ranges::size(range))::value>;

} // namespace mnkg

namespace std {

template <mnkg::point_c Point>
struct tuple_size<Point> : integral_constant<std::size_t, Point::dimension> {};

template <std::size_t I, mnkg::point_c Point>
struct tuple_element<I, Point> {
        using type = typename Point::component;
};

} // namespace std
