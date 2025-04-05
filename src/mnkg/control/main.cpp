#include "game.hpp"
#include "menu.hpp"

int
main()
{
        auto menu     = mnkg::control::configuation_menu{};
        auto settings = menu.run();
        if (!settings)
                return 0;
        auto game = mnkg::control::game(std::move(*settings));
        game.run();
};
