#pragma once

#include <cstddef>
#include <optional>
#include <utility>

#include "grid.hpp"
#include "point.hpp"

namespace mnk::model {

template <Point Point>
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

template <Grid Grid, class Point = Grid::point>
std::optional<line<Point>> find_line(
    const Grid& grid, const Point& point,
    std::optional<size_t> opt_covered_cell_count_minimum = std::nullopt,
    std::optional<size_t> opt_covered_cell_count_maximum = std::nullopt) {
  using Metric::Chebyshev;

  size_t min_length = 0;
  size_t max_length = norm<Chebyshev>(grid.get_size()) - 1;
  // +1 to account for starting point, in addition to offset of endpoints.
  size_t min_cover = opt_covered_cell_count_minimum.value_or(min_length + 1);
  size_t max_cover = opt_covered_cell_count_maximum.value_or(max_length + 1);

  static constexpr auto directions = std::array<Point, 4>{
      Point{0,  1 }, // Horizontal
      Point{-1, 1 },
      Point{-1, 0 }, // Vertical
      Point{-1, -1},
  };

  for (auto& direction : directions) {
    line<Point> line = {find_sequence_end(grid, point, direction),
                        find_sequence_end(grid, point, -direction)};
    // +1 to account for starting point, in addition to offset of endpoints.
    auto line_covered_cell_count = length<Chebyshev>(line) + 1;
    if (line_covered_cell_count < min_cover) continue;
    if (line_covered_cell_count > max_cover) continue;
    return line;
  }
  return {};
}

}  // namespace mnk::model
