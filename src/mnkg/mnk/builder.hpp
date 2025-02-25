#pragma once
#include "game.hpp"
#include "play_filter.hpp"

namespace mnkg::mnk {

class game::builder {
public:
        enum class preset {
                tictactoe,
                connect4,
                gomoku,
        };

        template <preset Preset>
        static game
        build()
        {
                if constexpr (Preset == preset::tictactoe) {
                        return game({
                            .board = { .size = { 3, 3 } },
                            .rules = { .line_span = 3 },
                        });
                } else if constexpr (Preset == preset::connect4) {
                        return game({
                            .board = { .size = { 7, 6 } },
                            .rules
                            = { .line_span = 4,
                                .overline  = true,
                                .play_filter
                                = std::make_unique<play_filter::gravity>() },
                        });
                } else if constexpr (Preset == preset::gomoku) {
                        return game({
                            .board = { .size = { 19, 19 } },
                            .rules = { .line_span = 5, .overline = false },
                        });
                }
        }
};

} // namespace mnkg::mnk
