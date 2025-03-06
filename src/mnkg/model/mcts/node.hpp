#pragma once

#include <memory>
#include <vector>

namespace mnkg::model::mcts {

template <typename Game>
struct node {

        Game::action                        action;
        Game                                game;
        size_t                              visits;
        float                               payoff;
        node                               *parent;
        std::vector<std::unique_ptr<node> > children;
        std::vector<typename Game::action>  untried;

        node(Game game) : game(game), untried(game.playable_actions()) {}

        node(node &parent, const Game::action &action) :
                parent(&parent), action(action), game(parent.game)
        {
                game.play(action);
                untried = game.playable_actions();
        }

        // cannot be defaulted because of the mutex: it is not moved (it cannot)
        node(node &&other) :
                action{ std::move(other.action) },
                game{ std::move(other.game) }, visits{ other.visits },
                payoff{ other.payoff }, parent{ other.parent },
                children{ std::move(other.children) },
                untried{ std::move(other.untried) }
        {
        }

private:
};

} // namespace mnkg::model::mcts
