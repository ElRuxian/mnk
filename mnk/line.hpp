#pragma once

#include <cstddef>
#include <optional>
#include <utility>

#include "grid.hpp"
#include "point.hpp"

namespace mnk {

template <class Point, typename ComponentType = Point::component_type,
          size_t Dimension = Point::dimension>
  requires std::is_same_v<Point, point<ComponentType, Dimension>>
class line {
 private:
  std::pair<Point, Point> endpoints_;

 public:
  line(const Point& start, const Point& end) : endpoints_(start, end) {}

  // Order: start to end.
  const auto& get_endpoints() const { return endpoints_; }

  template <Metric M>
  friend auto length(const line& line) {
    const auto& [start, end] = line.endpoints_;
    return norm<M>(end - start);
  }
};

template <class Grid, typename CellType = Grid::cell_type>
  requires std::is_same_v<Grid, grid<CellType>>
std::optional<line<typename Grid::point>> find_line(
    const Grid& grid, const typename Grid::point& point,
    std::optional<size_t> min_length_ = std::nullopt,
    std::optional<size_t> max_length_ = std::nullopt) {
  using Metric::Chebyshev;
  using point_t = Grid::point;

  size_t min_possible_length = 0;
  size_t max_possible_length = norm<Chebyshev>(grid.get_size());
  size_t min_length          = min_length_.value_or(min_possible_length);
  size_t max_length          = max_length_.value_or(max_possible_length);

  static constexpr auto directions = std::array<point_t, 4>{
      point_t{-1, 1},
      point_t{0,  1},
      point_t{1,  0},
      point_t{1,  1},
  };

  for (auto& direction : directions) {
    line<point_t> line        = {find_sequence_end(grid, point, direction),
                                 find_sequence_end(grid, point, -direction)};
    auto          line_length = length<Chebyshev>(line);
    if (line_length < min_length) continue;
    if (line_length > max_length) continue;
    return line;
  }
  return {};
}

}  // namespace mnk
