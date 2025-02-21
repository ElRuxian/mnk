#pragma once

#include <array>
#include <optional>

#include "grid.hpp"
#include "point.hpp"

namespace mnk::model {

template <point_c Point>
class line {
private:
        std::array<Point, 2> endpoints_;

public:
        using point = Point;

        line(const Point &x, const Point &y) : endpoints_{ x, y } {}

        const auto &
        endpoints() const
        {
                return endpoints_;
        }
};

template <typename T, class T_ = std::remove_cvref_t<T> >
concept line_c = std::same_as<T_, line<typename T_::point> >;

template <Metric Metric>
auto
length(const line_c auto &line)
{
        const auto &[x, y] = line.endpoints();
        return norm<Metric>(y - x);
}

template <grid_c Grid>
std::optional<line<typename Grid::position> >
find_line(const Grid &grid, const typename Grid::position &point)
{
        using point_t = std::decay_t<decltype(point)>;

        auto find_end = [&](const auto &stride) {
                auto end = point;
                auto it  = point + stride;
                while (within(grid, it) && grid[it] == grid[point]) {
                        end = it;
                        it += stride;
                }
                return end;
        };

        auto directions = std::to_array<point_t>(
            { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 } });

        for (auto &&dir : directions) {
                line<point_t> line = { find_end(dir), find_end(-dir) };
                if (length<Metric::Chebyshev>(line) > 0)
                        return line;
        }
        return std::nullopt;
}

} // namespace mnk::model
