#pragma once

#include "mnk/filters/gravity.hpp"
#include "mnk/game.hpp"

namespace mnkg::mnk::presets {

inline game::settings
tic_tac_toe()
{
        return {
                .board_size  = { 3, 3 },
                .line_span   = 3,
                .overline    = false,
                .play_filter = nullptr,
        };
}

inline game::settings
connect_four()
{
        return {
                .board_size = { 6, 7 },
                .line_span  = 4,
                .overline   = true,
                .play_filter
                = std::make_unique<filters::gravity>(board::position{ 1, 0 }),
        };
}

inline game::settings
gomoku()
{
        return {
                .board_size  = { 19, 19 },
                .line_span   = 5,
                .overline    = false,
                .play_filter = nullptr,
        };
}

} // namespace mnkg::mnk::presets
