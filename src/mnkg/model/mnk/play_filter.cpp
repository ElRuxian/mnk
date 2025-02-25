#pragma once

#include "play_filter.hpp"
#include "game.hpp"

namespace mnkg::model::mnk::play_filter {

board::position
fall_from(const board &board, board::position pos, board::position dir)
{
        auto it = pos + dir, end = pos;
        while (within(board, it) && !board[it].has_value()) {
                end = it;
                it += dir;
        }
        return end;
}

bool
gravity::allowed_(const game &game, const player::indice &player,
                  const action &action)
{
        return action == fall_from(game.get_board(), action, direction_);
}

bool
proximity::allowed_(const game &game, const player::indice &player,
                    const action &action)
{
        assert(range_ == 1 && "limited implementation");
        // TODO: DRY: directions also found in `find_line`
        constexpr auto directions = std::to_array<board::position>(
            { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 } });
        const auto &pos   = action;
        const auto &board = game.get_board();
        for (const auto &dir : directions)
                if (board[pos + dir].has_value())
                        return true;
        return false;
}

} // namespace mnkg::model::mnk::play_filter
