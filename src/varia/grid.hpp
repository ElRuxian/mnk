#pragma once
#include <type_traits>
#include <unistd.h>

#include <cassert>
#include <vector>

#include "point.hpp"

namespace mnkg {

template <typename Cell>
class grid {
public:
        using position = point<int, 2>;
        typedef Cell cell;

private:
        position          size_;
        std::vector<Cell> cells_;

        inline constexpr size_t
        index_(const position &coords) const noexcept
        {
                // Row-major order coordinate flattening.
                return coords[0] * get_size()[1] + coords[1];
        }

public:
        grid() = default;

        grid(const position &size, Cell value = Cell()) :
                size_(size), cells_(size_[0] * size_[1], value)
        {
        }

        position
        get_size() const noexcept
        {
                return size_;
        }

        size_t
        get_cell_count() const noexcept
        {
                return cells_.size();
        }

        // Unspecified iteration order.
        decltype(auto) begin(this auto &self) noexcept
        {
                return self.cells_.begin();
        }
        decltype(auto) end(this auto &self) noexcept
        {
                return self.cells_.end();
        }

        inline decltype(auto) operator[](this auto     &&self,
                                         const position &coords)
        {
                assert(within(self, coords));
                return self.cells_[self.index_(coords)];
        }
};

template <typename T, class T_ = std::remove_cvref_t<T> >
concept grid_c = std::same_as<T_, grid<typename T_::cell> >;

template <grid_c Grid>
bool
within(const Grid &grid, const typename Grid::position &position)
{
        using std::views::zip;
        for (auto [coord, boundary] : zip(position, grid.get_size()))
                if (coord < 0 || coord >= boundary)
                        return false;
        return true;
}

template <grid_c Grid>
Grid::position
find_equal_cell_sequence_end(const Grid                    &grid,
                             const typename Grid::position &start,
                             const typename Grid::position &stride)
{
        assert(within(grid, start));
        assert(stride != decltype(stride)::make_origin()
               && "Infinite loop prevention");
        auto end = start;
        for (auto i = start; within(grid, i) && grid[i] == grid[start];
             i += stride)
                end = i;
        return end;
}

template <grid_c Grid>
auto
coords(const Grid &grid)
{
        using namespace std::views;
        const auto &[x_limit, y_limit] = grid.get_size();
        return iota(0, x_limit) | transform([y_limit](auto x) {
                       return iota(0, y_limit) | transform([x](auto y) {
                                      return typename Grid::position{ x, y };
                              });
               })
               | join;
}

} // namespace mnkg
