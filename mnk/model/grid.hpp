#pragma once
#include <unistd.h>

#include <cassert>
#include <vector>

#include "point.hpp"

namespace mnk::model {

// Designed specifically for this project; not for general use.
template <typename CellType>
class grid {
 public:
  using point = point<int, 2>;
  typedef CellType cell_type;

 private:
  point                 size_;
  std::vector<CellType> cells_;

  inline constexpr size_t index(const point& coords) const noexcept {
    // Row-major order coordinate flattening.
    return coords[0] * get_size()[1] + coords[1];
  }

 public:
  grid() = default;

  grid(const point& size, CellType value = CellType())
      : size_(size), cells_(size_[0] * size_[1], value) {}

  point  get_size() const noexcept { return size_; }
  size_t get_cell_count() const noexcept { return cells_.size(); }

  // These declarations do not intend to guarantee any specific iteration order.
  decltype(auto) begin(this auto& self) noexcept { return self.cells_.begin(); }
  decltype(auto) end(this auto& self) noexcept { return self.cells_.end(); }

  inline decltype(auto) operator[](this auto&& self, const point& coords) {
    assert(inside(self, coords));
    return self.cells_[self.index(coords)];
  }
};

template <typename CellType>
bool inside(const grid<CellType>& grid, const point<int, 2>& coords) {
  using std::views::zip;
  for (auto [coord, boundary] : zip(coords, grid.get_size())) {
    if (coord < 0 || coord >= boundary) return false;
  }
  return true;
}

template <typename CellType>
point<int, 2> find_sequence_end(const grid<CellType>& grid,
                                const point<int, 2>&  start,
                                const point<int, 2>&  stride) {
  assert(inside(grid, start));
  point<int, 2> end = start;
  for (auto i = start; inside(grid, i) && grid[i] == grid[start]; i += stride)
    end = i;
  return end;
}

}  // namespace mnk::model
