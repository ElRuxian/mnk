#pragma once

#include <optional>
#include <utility>

#include "grid.hpp"
#include "point.hpp"

namespace mnk::model {

template <point_c Point>
class line {
private:
        std::pair<Point, Point> endpoints_;

public:
        using point = Point;

        line(const Point &x, const Point &y) : endpoints_(x, y) {}

        const auto &
        endpoints() const
        {
                return endpoints_;
        }
};

template <typename T>
concept line_c = std::same_as<T, line<typename T::point> >;

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
        auto find_end = [&](const auto &stride) {
                auto end = point;
                auto it  = point + stride;
                while (within(grid, it) && grid[it] == grid[point]) {
                        end = it;
                        it += stride;
                }
                return end;
        };

        auto directions = std::array{
                decltype(point){ 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }
        };

        for (auto &&dir : directions) {
                line<decltype(point)> line = { find_end(dir), find_end(-dir) };
                if (length<Metric::Chebyshev>(line) > 0)
                        return line;
        }
        return std::nullopt;
}

} // namespace mnk::model
