#pragma once

#include <array>
#include <generator>
#include <type_traits>

#include "grid.hpp"
#include "point.hpp"

namespace mnkg {

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

std::ostream &
operator<<(std::ostream &ostream, const line_c auto &line)
{
        return ostream << std::format("{}", range_formatter(line.endpoints()));
}

template <metric Metric>
auto
length(const line_c auto &line)
{
        const auto &[x, y] = line.endpoints();
        return norm<Metric>(y - x);
}

template <grid_c Grid>
std::generator<line<typename Grid::position> >
find_lines(const Grid &grid, const typename Grid::position &point)
{
        using point_t = std::decay_t<decltype(point)>;

        auto find_end = [&](const point_t &stride) -> point_t {
                auto end = point;
                auto it  = point + stride;
                while (within(grid, it) && grid[it] == grid[point]) {
                        end = it;
                        it += stride;
                }
                return end;
        };

        constexpr auto directions = std::to_array<point_t>(
            { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 } });

        for (auto &&dir : directions) {
                line<point_t> line = { find_end(dir), find_end(-dir) };
                if (length<metric::chebyshev>(line) > 0)
                        co_yield line;
        }
}
} // namespace mnkg

template <mnkg::line_c Line, typename CharT>
struct std::formatter<Line, CharT> : std::formatter<CharT> {

        template <typename Context>
        auto
        format(const Line &line, Context &ctx) const
        {
                const auto &endpoints = line.endpoints();

                using endpoints_t = std::decay_t<decltype(endpoints)>;
                using formatter   = std::formatter<endpoints_t, CharT>;

                return formatter::format(endpoints, ctx);
        }
};
