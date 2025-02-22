#pragma once

#include "mnk/board.hpp"
#include "mnk/game.hpp"
#include <cassert>

namespace mnkg::mnk::filters {

// Allows actions in proximity to previous actions only
class proximity : public game::play_filter {

        proximity(size_t range = 1) : range_(range) {}

        void
        set_range(size_t range)
        {
                range_ = range;
        }

        bool
        allowed(const game &game, const game::player_indice &player,
                const game::action &action) override
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

private:
        size_t range_;
};

} // namespace mnkg::mnk::filters
