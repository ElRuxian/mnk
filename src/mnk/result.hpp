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

} // namespace mnkg::mnk
