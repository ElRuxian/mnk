#pragma once

#include "board.hpp"
#include "model/player.hpp"

#include "varia/grid.hpp"
#include "varia/line.hpp"

namespace mnkg::model::mnk {

struct win {
        player::index         player;
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

} // namespace mnkg::model::mnk
