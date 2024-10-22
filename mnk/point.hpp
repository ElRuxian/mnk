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

namespace mnk {

// Point in some n-dimensional vector space.
template <typename ComponentType, size_t Dimension>
  requires(Dimension > 0)
class point {
 private:
  std::array<ComponentType, Dimension> components_;
  constexpr point() = default;  // Forced use of make_origin() for clarity.

 public:
  static constexpr size_t dimension = Dimension;
  typedef ComponentType   component_type;

  static constexpr point make_origin() { return point{}; }

  template <typename... T>
    requires(sizeof...(T) == Dimension)
  constexpr point(T... args) : components_{args...} {}

  inline decltype(auto) operator[](this auto&& self, size_t index) {
    assert(index < dimension);
    return self.components_[index];
  }

  decltype(auto) constexpr begin(this auto&& self) noexcept {
    return self.components_.begin();
  }
  decltype(auto) constexpr end(this auto&& self) noexcept {
    return self.components_.end();
  }

  friend void swap(point& lhs, point& rhs) {
    using std::swap;
    swap(lhs.components_, rhs.components_);
  }

  bool operator==(point<ComponentType, Dimension> other) const noexcept {
    return components_ == other.components_;
  }

  auto& operator=(point<ComponentType, Dimension> other) {
    swap(*this, other);
    return *this;
  }

  auto& operator+=(const point<ComponentType, Dimension>& other) {
    using std::plus;
    using std::ranges::transform;
    transform(components_, other.components_, components_.begin(), plus<>());
    return *this;
  }

  auto& operator-=(const point<ComponentType, Dimension>& other) {
    using std::minus;
    using std::ranges::transform;
    transform(components_, other.components_, components_.begin(), minus<>());
    return *this;
  }

  auto& operator*=(const ComponentType& scalar) {
    using std::bind_front;
    using std::multiplies;
    using std::ranges::transform;
    transform(components_, components_.begin(),
              bind_front(multiplies<>(), scalar));
    return *this;
  }

  auto& operator/=(const ComponentType& scalar) {
    using std::bind_front;
    using std::multiplies;
    using std::ranges::transform;
    transform(components_, components_.begin(),
              bind_front(multiplies<>(), scalar));
    return *this;
  }
};

template <typename ComponentType, size_t Dimension>
std::ostream& operator<<(std::ostream&                          ostream,
                         const point<ComponentType, Dimension>& point) {
  using std::views::take;
  ostream << '(';
  for (const auto& comp : point | take(Dimension - 1)) {
    ostream << comp << ", ";
  }
  ostream << point[Dimension - 1] << ')';
  return ostream;
}

template <typename ComponentType, size_t Dimension>
point<ComponentType, Dimension> operator-(
    const point<ComponentType, Dimension>& point) {
  auto result = mnk::point<ComponentType, Dimension>::make_origin();
  std::ranges::transform(point, result.begin(), std::negate<>{});
  return result;
}

template <typename ComponentType, size_t Dimension>
auto operator+(const point<ComponentType, Dimension>& lhs,
               const point<ComponentType, Dimension>& rhs) {
  return auto(lhs) += rhs;
}

template <typename ComponentType, size_t Dimension>
auto operator-(const point<ComponentType, Dimension>& lhs,
               const point<ComponentType, Dimension>& rhs) {
  return auto(lhs) -= rhs;
}

template <typename ComponentType, size_t Dimension>
auto operator*(const point<ComponentType, Dimension>& point,
               const ComponentType&                   scalar) {
  return auto(point) *= scalar;
}

template <typename ComponentType, size_t Dimension>
auto operator/(const point<ComponentType, Dimension>& point,
               const ComponentType&                   scalar) {
  return auto(point) /= scalar;
}

enum class Metric { Euclidean, Chebyshev };
// Enum-based dispatch for better use semantics.
template <Metric metric, typename ComponentType, size_t Dimension>
constexpr size_t norm(const point<ComponentType, Dimension>& vec) {
  using enum Metric;
  if constexpr (metric == Euclidean) {
    static_assert(Dimension == 2 && "Implementation limited to 2D vectors");
    return std::hypot(vec[0], vec[1]);
  } else if constexpr (metric == Chebyshev) {
    using std::ranges::max_element;
    return *max_element(vec);
  } else {
    static_assert(!"Incomplete implementation");
  }
}

}  // namespace mnk
