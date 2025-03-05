#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <limits>
#include <memory>
#include <model/game.hpp>
#include <model/player.hpp>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

namespace mnkg::model::mcts {

// clang-format off
template <class Game, typename Action = typename Game::action>
requires
    std::is_base_of_v<model::game::combinatorial<Action>, Game> 
class mcts { // clang-format on
public:
        struct settings {
                size_t exploration_factor  = 2; // UCT constant
                size_t search_thread_count = 1;
        };

        mcts(Game game, settings settings = {}) :
                root_{ std::make_unique<node>(
                    node{ .game = game, .untried = game.playable_actions() }) },
                settings_{ settings }
        {
                for (size_t i = 0; i < settings_.search_thread_count; ++i) {
                        search_.workers.threads.emplace_back(&mcts::worker_,
                                                             this);
                }
        }

        ~mcts()
        {
                stop_searching();
                for (auto &t : search_.workers.threads) {
                        if (t.joinable()) {
                                t.join();
                        }
                }
        }

        Game::action
        evaluate()
        {
                std::lock_guard<std::mutex> lock(tree_mutex_);
                assert(!root_->children.empty());
                auto best
                    = std::max_element(root_->children.begin(),
                                       root_->children.end(),
                                       [](const auto &a, const auto &b) {
                                               return a->visits < b->visits;
                                       });
                return (*best)->action;
        }

        void
        start_searching()
        {
                search_.running.store(true);
                search_.running.notify_all();
        }

        void
        stop_searching()
        {
                search_.running.store(false);
                search_.running.notify_all();
                while (search_.workers.working.load() > 0) {
                        search_.workers.working.wait(
                            search_.workers.working.load());
                }
        }

        void
        advance(const Game::action &action)
        {
                bool was_searching = search_.running.load();
                stop_searching();
                {
                        std::lock_guard<std::mutex> lock(tree_mutex_);
                        auto it = std::find_if(root_->children.begin(),
                                               root_->children.end(),
                                               [&action](const auto &node_ptr) {
                                                       return node_ptr->action
                                                              == action;
                                               });
                        if (it != root_->children.end()) {
                                root_         = std::move(*it);
                                root_->parent = nullptr;
                        } else {
                                root_->action = action;
                                root_->game.play(action);
                                root_->visits = 0;
                                root_->payoff = 0;
                                root_->children.clear();
                                root_->untried = root_->game.playable_actions();
                        }
                }
                if (was_searching)
                        start_searching();
        }

private:
        struct node {
                Game::action action; // action that led here
                Game         game;   // game state
                size_t       visits = 0;
                int          payoff = 0; // accumulated value
                node        *parent = nullptr;
                std::vector<std::unique_ptr<node> > children;
                std::vector<typename Game::action>  untried;
        };

        std::unique_ptr<node> root_;
        settings              settings_;
        struct {
                struct {
                        std::vector<std::thread> threads;
                        std::atomic<size_t>      working{ 0 };
                } workers;
                std::atomic<bool> running{ false };
        } search_;

        mutable std::mutex tree_mutex_;

        static std::mt19937 &
        rng_()
        {
                thread_local std::mt19937 rng{ std::random_device{}() };
                return rng;
        }

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
        select_(node &from)
        {
                node *current = &from;
                while (current->untried.empty() && !current->children.empty()) {
                        current->visits++;
                        current->payoff--; // apply virtual loss
                        auto best_it = std::max_element(
                            current->children.begin(),
                            current->children.end(),
                            [this](const auto &a, const auto &b) {
                                    return rate_(*a) < rate_(*b);
                            });
                        current = best_it->get();
                }
                current->visits++;
                current->payoff--; // virtual loss for the final node
                return *current;
        }

        node &
        expand_(node &parent)
        {
                assert(!parent.untried.empty());
                auto                                 &rng = rng_();
                std::uniform_int_distribution<size_t> distribution(
                    0, parent.untried.size() - 1);
                std::swap(parent.untried.back(),
                          parent.untried[distribution(rng)]);
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

        Game
        simulate_(node &node)
        {
                auto  game = node.game;
                auto &rng  = rng_();
                while (!game.is_over()) {
                        auto actions = game.playable_actions();
                        assert(!actions.empty());
                        std::uniform_int_distribution<size_t> distribution(
                            0, actions.size() - 1);
                        const auto &action = actions[distribution(rng)];
                        game.play(action);
                }
                return game;
        }

        void
        backpropagate_(node &node, Game terminal)
        {
                for (auto *it = &node; it != nullptr; it = it->parent) {
                        it->payoff++; // remove virtual loss
                        if (!terminal.is_draw()) {
                                bool won = it->game.current_opponent()
                                           == terminal.winner();
                                it->payoff += (won ? 1 : -1);
                        }
                }
        }

        void
        iterate_(node &root)
        {
                node *selected;
                {
                        std::lock_guard<std::mutex> lock(tree_mutex_);
                        selected = &select_(root);
                        if (selected->game.is_over()) {
                                backpropagate_(*selected, selected->game);
                                return;
                        }
                        selected = &expand_(*selected);
                }
                Game simulation_result = simulate_(*selected);
                {
                        std::lock_guard<std::mutex> lock(tree_mutex_);
                        backpropagate_(*selected, simulation_result);
                }
        }

        void
        worker_()
        {
                while (true) {
                        search_.running.wait(false);
                        search_.workers.working.fetch_add(1);
                        while (search_.running.load()) {
                                iterate_(*root_);
                        }
                        search_.workers.working.fetch_sub(1);
                        search_.workers.working.notify_all();
                }
        }
};

} // namespace mnkg::model::mcts

