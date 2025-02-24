#pragma once

#include "mnk/board.hpp"
#include "mnk/game.hpp"
#include <cassert>
#include <print>

namespace mnkg::mnk::filters {

// Allows actions that "fall in a straight line" only
// Intended for games like Connect Four
class gravity : public game::play_filter {
public:
        gravity(board::position direction = { 1, 0 })
        {
                set_direction(direction);
        }

        void
        set_direction(board::position direction)
        {
                assert(norm<metric::chebyshev>(direction) == 1);
                direction_ = direction;
        }

        board::position
        fall_from(const board &board, board::position pos) const
        {
                auto end = pos;
                auto it  = pos + direction_;
                while (within(board, it) && !board[it].has_value()) {
                        end = it;
                        it += direction_;
                }
                return end;
        }

        bool
        allowed(const game &game, const game::player_indice &player,
                const game::action &action) override
        {
                return action == fall_from(game.get_board(), action);
        }

private:
        board::position direction_;
};

} // namespace mnkg::mnk::filters
