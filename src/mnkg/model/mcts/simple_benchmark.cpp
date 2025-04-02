#include "ai.hpp"
#include "model/mnk/game.hpp"
#include <iostream>

int
main()
{
        using namespace mnkg::model;
        auto game = mnk::game(
            mnk::game::configuration<mnk::game::preset::tictactoe>());
        auto ai = mcts::ai(
            game,
            { .leaf_parallelization
              = std::thread::hardware_concurrency() }); // starts searching
        auto seconds = 0;
        auto iters   = 0;
        while (true) {
                auto iters_ = ai.iterations();
                auto delta  = iters_ - iters;
                iters       = iters_;
                auto sims   = ai.simulations();
                std::cout << "[ " << seconds++ << "s] iters: " << iters
                          << "\td-iters: " << delta << "\tsims: " << sims
                          << std::endl;
                sleep(1);
        }
}
