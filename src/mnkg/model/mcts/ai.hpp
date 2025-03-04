#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <model/game.hpp>
#include <model/player.hpp>
#include <print>
#include <random>
#include <vector>

namespace mnkg::model::mcts {

// clang-format off
template <class Game, typename Action = typename Game::action>
        requires std::is_base_of_v<model::game::combinatorial<Action>, Game>
class mcts { // clang-format on
public:
        mcts(Game game) :
                root_{ std::make_unique<Node>(std::move(
                    Node{ .game = game, .untried = game.legal_actions() })) },
                rng_{ std::random_device{}() }
        {
        }

        Game::action
        evaluate()
        {
                assert(!root_->children.empty());
                return (*std::max_element(root_->children.begin(),
                                          root_->children.end(),
                                          [](const auto &a, const auto &b) {
                                                  return compare_(*a, *b);
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
                        root_->game.play(action);
                        root_->untried = root_->game.legal_actions();
                        root_->children.clear();
                }
        }

private:
        struct Node {
                Game::action                        action;
                Game                                game;
                size_t                              visits = 0;
                float                               payoff = 0;
                Node                               *parent = nullptr;
                std::vector<std::unique_ptr<Node> > children;
                std::vector<typename Game::action>  untried;
        };
        std::unique_ptr<Node> root_;
        std::mt19937          rng_;

        static float
        rate_(const Node &node)
        {
                // UCT (Upper Confidence Bound 1 applied to trees)
                assert(node.parent);
                if (node.visits > 0)
                        return node.payoff / node.visits
                               + std::sqrt(2 * std::log(node.parent->visits)
                                           / node.visits);
                else
                        return std::numeric_limits<float>::infinity();
        }

        static auto
        compare_(const Node &lhs, const Node &rhs)
        {
                return rate_(lhs) < rate_(rhs);
        }

        // TODO: move to game
        std::vector<float> // payoff per player
        payoffs_(const Game &game)
        {
                std::vector<float> payoffs;
                payoffs.reserve(game.get_player_count());
                for (size_t i = 0; i < game.get_player_count(); ++i)
                        payoffs.push_back(game.payoff(i));
                return payoffs;
        }

        Node &
        select_()
        {
                Node *node = root_.get();
                while (node->untried.empty()) {
                        if (node->children.empty())
                                break; // terminal node
                        node = std::max_element(
                                   node->children.begin(),
                                   node->children.end(),
                                   [](const auto &a, const auto &b) {
                                           return compare_(*a, *b);
                                   })
                                   ->get();
                }
                return *node;
        }

        Node &
        expand_(Node &parent)
        {
                assert(!parent.untried.empty());
                auto action = parent.untried.back();
                parent.untried.pop_back();
                auto game{ Game(parent.game) };
                game.play(action);
                auto untried = game.legal_actions();
                std::shuffle(untried.begin(), untried.end(), rng_);
                auto child = std::make_unique<Node>(
                    std::move(Node{ .action  = std::move(action),
                                    .game    = std::move(game),
                                    .parent  = &parent,
                                    .untried = std::move(untried) }));
                parent.children.emplace_back(std::move(child));
                return *parent.children.back();
        }

        Game // over
        simulate_(Node &node)
        {
                // Random playout
                auto game = node.game;
                while (!game.is_over()) {
                        auto actions = game.legal_actions();
                        assert(!actions.empty());
                        std::uniform_int_distribution<size_t> distribution(
                            0, actions.size() - 1);
                        const auto &action = actions[distribution(rng_)];
                        game.play(action);
                };
                return game;
        }

        void
        backpropagate_(Node &node, std::vector<float> payoffs)
        {
                assert(payoffs.size() == node.game.get_player_count());
                for (Node *it = &node; it != nullptr; it = it->parent) {
                        it->visits++;
                        it->payoff += payoffs[it->game.current_player()];
                }
        }

        void
        iterate_()
        {
                auto               &selected = select_();
                Node               *leaf;
                std::optional<Game> terminal;
                if (selected.game.is_over()) {
                        leaf     = &selected;
                        terminal = selected.game;
                } else {
                        leaf     = &expand_(selected);
                        terminal = simulate_(*leaf);
                }
                auto payoffs = payoffs_(*terminal);
                backpropagate_(*leaf, payoffs);
        }
};

} // namespace mnkg::model::mcts
