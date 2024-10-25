#pragma once

#include "mnk/model/game.hpp"
#include "mnk/model/line.hpp"

namespace mnk::model::games {

namespace {
auto board = game::board{
    {7, 6}
};
auto rules = game::rules{
    .inspectors{
                .legal_move =
            [](const game& game, const game::move& move) {
              const auto& grid = game.get_board().get_grid();
              const auto& down = point<int, 2>{1, 0};
              return find_sequence_end(grid, move.coords, down) == move.coords;
            }, .win =
            [](const game& game, const game::move& move) {
              const auto& grid = game.get_board().get_grid();
              return find_line(grid, move.coords, 4).has_value();
            }, .tie = [](const game& game,
                const game::move&) { return (game.get_board().is_full()); },
                }
};
}

const auto connect_4 = game(board, rules);

}  // namespace mnk::model::games
