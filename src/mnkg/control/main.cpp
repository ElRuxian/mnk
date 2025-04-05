#include "game.hpp"
#include "menu.hpp"

int
main()
{
        mnkg::control::configuation_menu menu;
        mnkg::control::game::settings    settings = *menu.run();
        mnkg::control::game              game(std::move(settings));
        game.run();
};
