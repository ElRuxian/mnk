#pragma once

#include "model/mcts/tree.hpp"
#include "model/mcts/worker.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>
#include <model/game.hpp>
#include <model/player.hpp>
#include <mutex>
#include <vector>

namespace mnkg::model::mcts {

// clang-format off
template <class Game, typename Action = typename Game::action>
requires
    std::is_base_of_v<model::game::combinatorial<Action>, Game>
class ai { // clang-format on
public:
        using worker = worker<tree<Game> >;

        struct settings {
                size_t worker_count = 1;
        };

        ai(Game game, settings settings = {}) :
                tree_{ game }, settings_{ settings }
        {
                assert(settings.worker_count > 0);
                workers_.reserve(settings.worker_count);
                for (size_t i = 0; i < settings.worker_count; ++i) {
                        workers_.emplace_back(std::make_unique<worker>(tree_));
                }
        }

        Game::action
        evaluate()
        {
                auto        lock       = std::unique_lock(tree_.mutex);
                const auto &candidates = tree_.root->children;
                assert(!candidates.empty());
                auto compare = [](const auto &a, const auto &b) {
                        return a->visits < b->visits;
                };
                const auto &chosen = *std::max_element(
                    candidates.begin(), candidates.end(), compare);
                return chosen->action;
        }

        void
        advance(const Game::action &action)
        {
                auto  lock      = std::unique_lock(tree_.mutex);
                auto &root      = tree_.root;
                auto &next      = root->children;
                auto  is_target = [&action](const auto &node) {
                        return node->action == action;
                };
                auto target = std::find_if(next.begin(), next.end(), is_target);
                bool found  = target != next.end();
                if (found) {
                        root         = std::move(*target);
                        root->parent = nullptr;
                } else {
                        root->action = action;
                        root->game.play(action);
                        root->visits = 0;
                        root->payoff = 0;
                        root->children.clear();
                        root->untried = root->game.playable_actions();
                }
        }

private:
        tree<Game>                            tree_;
        settings                              settings_;
        std::vector<std::unique_ptr<worker> > workers_;
};

} // namespace mnkg::model::mcts
