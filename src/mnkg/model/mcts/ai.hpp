#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <model/game.hpp>
#include <model/player.hpp>
#include <random>
#include <vector>

namespace mnkg::model::mcts {

// clang-format off
template <class Game, typename Action = typename Game::action>
        requires std::is_base_of_v<model::game::combinatorial<Action>, Game>
class mcts { // clang-format on
public:
        struct settings {
                size_t exploration_factor = 2; // UCT constant
        };

        mcts(Game game, settings settings = {}) :
                root_{ std::make_unique<node>(
                    node{ .game = game, .untried = game.playable_actions() }) },
                rng_{ std::random_device{}() }, settings_{ settings }
        {
        }

        Game::action
        evaluate()
        {
                assert(!root_->children.empty());
                return (*std::max_element(root_->children.begin(),
                                          root_->children.end(),
                                          [](const auto &a, const auto &b) {
                                                  return a->visits < b->visits;
                                          }))
                    ->action;
        }

        void
        expand(std::chrono::seconds time_limit, size_t iterations_limit)
        {
                using namespace std::chrono;
                auto start = steady_clock::now();
                int  i;
                for (i = 0; i < iterations_limit; ++i) {
                        auto now = steady_clock::now();
                        if (duration_cast<seconds>(now - start) >= time_limit)
                                break;
                        iterate_();
                }
        }

        void
        advance(const Game::action &action)
        {
                auto it
                    = std::find_if(root_->children.begin(),
                                   root_->children.end(),
                                   [&action](const auto &node_ptr) {
                                           return node_ptr->action == action;
                                   });
                if (it != root_->children.end()) {
                        root_         = std::move(*it);
                        root_->parent = nullptr;
                } else {
                        root_->action = action;
                        root_->game.play(action);
                        root_->visits = 0;
                        root_->payoff = 0;
                        // root_->parent = nullptr; // already nullptr
                        root_->children.clear();
                        root_->untried = root_->game.playable_actions();
                }
        }

private:
        struct node {
                Game::action                        action; // that lead here
                Game                                game;   // game-state
                size_t                              visits = 0;
                int                                 payoff = 0; // accumulated
                node                               *parent = nullptr;
                std::vector<std::unique_ptr<node> > children;
                std::vector<typename Game::action>  untried;
        };
        std::unique_ptr<node> root_;
        std::mt19937          rng_;
        settings              settings_;

        float
        rate_(const node &node)
        {
                // UCT (Upper Confidence Bound 1 applied to trees)
                assert(node.parent);
                if (node.visits > 0)
                        return static_cast<float>(node.payoff) / node.visits
                               + std::sqrt(settings_.exploration_factor
                                           * std::log(node.parent->visits)
                                           / node.visits);
                else
                        return std::numeric_limits<float>::infinity();
        }

        node &
        select_()
        {
                node *node = root_.get();
                while (node->untried.empty()) {
                        if (node->children.empty())
                                break; // terminal node
                        node->visits++;
                        node->payoff--; // virtual loss
                        node = std::max_element(
                                   node->children.begin(),
                                   node->children.end(),
                                   [this](const auto &a, const auto &b) {
                                           return rate_(*a) < rate_(*b);
                                   })
                                   ->get();
                }
                node->visits++;
                node->payoff--; // virtual loss
                return *node;
        }

        node &
        expand_(node &parent)
        {
                assert(!parent.untried.empty());
                std::uniform_int_distribution<size_t> distribution(
                    0, parent.untried.size() - 1);
                swap(parent.untried.back(), parent.untried[distribution(rng_)]);
                auto action = parent.untried.back();
                parent.untried.pop_back();
                auto game = parent.game;
                game.play(action);
                auto child = std::make_unique<node>(
                    node{ .action  = std::move(action),
                          .game    = std::move(game),
                          .parent  = &parent,
                          .untried = game.playable_actions() });
                parent.children.emplace_back(std::move(child));
                return *parent.children.back();
        }

        Game // terminal state
        simulate_(node &node)
        {
                // Random playout
                auto game = node.game;
                while (!game.is_over()) {
                        auto actions = game.playable_actions();
                        assert(!actions.empty());
                        std::uniform_int_distribution<size_t> distribution(
                            0, actions.size() - 1);
                        const auto &action = actions[distribution(rng_)];
                        game.play(action);
                };
                return game;
        }

        void
        backpropagate_(node &node, Game terminal)
        {
                for (auto *it = &node; it != nullptr; it = it->parent) {
                        it->payoff++; // remove virtual loss
                        if (!terminal.is_draw()) {
                                // set payoff from the perspective of the player
                                // whose turn led to this state
                                bool won = it->game.current_opponent()
                                           == terminal.winner();
                                it->payoff += won ? 1 : -1;
                        }
                }
        }

        void
        iterate_()
        {
                auto &selected = select_();
                if (selected.game.is_over()) {
                        backpropagate_(selected, selected.game);
                } else {
                        auto &leaf = expand_(selected);
                        backpropagate_(leaf, simulate_(leaf));
                }
        }
};

} // namespace mnkg::model::mcts
