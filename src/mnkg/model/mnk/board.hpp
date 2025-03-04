#pragma once

#include "model/player.hpp"
#include "varia/grid.hpp"
#include <optional>

namespace mnkg::model::mnk {

using board = grid<std::optional<player::index> >;

}
