#pragma once

#include "model/mnk/game.hpp" // TODO: MVC pattern
#include <memory>

namespace mnkg::view {

enum class style {
        tictactoe,
        connect_four,
        go,
};

struct settings {
        std::string title = "MNK Game";
        style       style = style::tictactoe;
};

class gui {
public:
        gui(std::shared_ptr<mnkg::model::mnk::game> &game,
            settings                                 settings = {});

        ~gui();

        void
        run();

private:
        class implementation;
        std::unique_ptr<implementation> pimpl_;
};

} // namespace mnkg::view
