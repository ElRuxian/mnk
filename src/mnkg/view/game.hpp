#pragma once

#include "varia/point.hpp"
#include <memory>

namespace mnkg::view {

class game {
public:
        enum class style {
                tictactoe,
                connect_four,
                go,
        };

        struct callbacks {
                std::function<void(point<int, 2>)> on_cell_selected;
        };

        struct settings {
                std::string    title;
                style          style;
                callbacks      callbacks;
                point<uint, 2> board_size;
        };

public:
        game(const settings &settings);

        ~game();

        void
        run(); // occupies calling thread to manage the window

        void
        set_selectable_cells(const std::vector<point<int, 2> > &coords);

        void
        set_stone_skin(uint index);

        void
        draw_stone(point<int, 2> cell_coords);

        void
        highlight_stone(point<int, 2> cell_coords);

private:
        class implementation;
        std::unique_ptr<implementation> pimpl_;
};

} // namespace mnkg::view
