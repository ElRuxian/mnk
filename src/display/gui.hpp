#pragma once

#include "../game.hpp"
#include <memory>

namespace mnkg::mnk::display {

class gui {
public:
        enum class skin {
                paper_grid,
                go_board,
                drop_grid,
        };

        struct settings {
                skin        skin;
                std::string title;
        };

        gui(game &game, settings settings);

        void
        run(); // blocking

private:
        struct impl;
        std::unique_ptr<impl> pimpl;
};

} // namespace mnkg::mnk::display
