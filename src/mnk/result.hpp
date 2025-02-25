#pragma once

#include "board.hpp"
#include "player.hpp"

#include "varia/grid.hpp"
#include "varia/line.hpp"

namespace mnkg::mnk {

struct win {
        player::indice        player;
        line<board::position> line;
};

struct tie {};

using result = std::variant<win, tie>;

inline bool
is_win(const result &result) noexcept
{
        return std::holds_alternative<win>(result);
}

inline bool
is_tie(const result &result) noexcept
{
        return std::holds_alternative<tie>(result);
}

} // namespace mnkg::mnk
