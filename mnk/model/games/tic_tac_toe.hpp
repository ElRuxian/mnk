#pragma once

#include "mnk/model/game.hpp"
#include "mnk/model/line.hpp"

namespace mnk::model::games {

namespace {
auto board = game::board{
    {3, 3}
};
auto rules = game::rules{
    .inspectors{
                .legal_move = [](const game&       game,
                const game::move& move) { return true; },
                .win =
            [](const game& game, const game::move& move) {
              const auto& grid = game.get_board().get_grid();
              return find_line(grid, move.coords, 3).has_value();
            }, .tie = [](const game& game,
                const game::move&) { return (game.get_board().is_full()); },
                }
};
}  // namespace

const auto tic_tac_toe = game(board, rules);

}  // namespace mnk::model::games
