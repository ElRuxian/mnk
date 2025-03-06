#pragma once

#include "model/mcts/tree.hpp"
#include <algorithm>
#include <asio/thread_pool.hpp>
#include <cassert>
#include <cmath>
#include <future>
#include <model/game.hpp>
#include <model/player.hpp>
#include <mutex>
#include <numeric>
#include <print>
#include <random>
#include <thread>

namespace mnkg::model::mcts {

// clang-format off
template <class Game, typename Action = typename Game::action>
requires
    std::is_base_of_v<model::game::combinatorial<Action>, Game>
class ai { // clang-format on
public:
        using tree = tree<Game>;

        struct hyperparameters {
                size_t leaf_parallelization = 1; // simulations per iteration
                size_t exploration          = std::sqrt(2); // UCT constant
        };

        ai(Game game, hyperparameters hparams = {}) :
                tree_{ game }, hyperparameters_{ hparams },
                worker_pool_{ hparams.leaf_parallelization },
                rng_{ std::random_device{}() }, search_thread_{ [this] {
                        while (!stopping_.load(std::memory_order_relaxed))
                                iterate_(tree_);
                } }
        {
                assert(hparams.leaf_parallelization > 0);
        }

        ~ai()
        {
                stopping_.store(true);
                search_thread_.join();
        }

        Game::action
        evaluate()
        {
                auto        lock       = std::lock_guard(tree_.mutex);
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
                auto  lock      = std::lock_guard(tree_.mutex);
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

        size_t
        iterations() const
        {
                return iteration_count_.load(std::memory_order_relaxed);
        }

        size_t
        simulations() const
        {
                return iterations() * hyperparameters_.leaf_parallelization;
        }

private:
        tree                tree_;
        hyperparameters     hyperparameters_;
        std::atomic<size_t> iteration_count_ = { 0 };
        asio::thread_pool   worker_pool_;
        std::thread         search_thread_;
        std::atomic<bool>   stopping_ = { false };
        std::mt19937        rng_;

        float
        rate_(const tree::node &node)
        {
                // UCT (Upper Confidence Bound 1 applied to trees)
                assert(node.parent);
                if (node.visits > 0)
                        return node.payoff / node.visits
                               + hyperparameters_.exploration
                                     * std::sqrt(std::log(node.parent->visits)
                                                 / node.visits);
                else
                        return std::numeric_limits<float>::infinity();
        }

        tree::node &
        select_(const tree &tree)
        {
                auto *it = tree.root.get();
                while (!should_select_(*it))
                        it = &next_(*it);
                return *it;
        }

        bool
        should_select_(const tree::node &node)
        {
                bool terminal   = node.game.is_over();
                bool expandable = !node.untried.empty();
                bool rand = std::uniform_int_distribution<int>(0, 1)(rng_);

                return terminal || (expandable && rand);
        }

        inline tree::node &
        next_(const tree::node &node)
        {
                return *std::max_element(node.children.begin(),
                                         node.children.end(),
                                         [this](const auto &a, const auto &b) {
                                                 return rate_(*a) < rate_(*b);
                                         })
                            ->get();
        }

        tree::node &
        expand_(tree::node &parent)
        {
                // pick random untried action
                assert(!parent.untried.empty());
                assert(!parent.game.is_over());
                std::uniform_int_distribution<size_t> distribution(
                    0, parent.untried.size() - 1);
                std::swap(parent.untried.back(),
                          parent.untried[distribution(rng_)]);
                auto action = parent.untried.back();
                parent.untried.pop_back();

                // create and return corresponding child
                parent.children.emplace_back(
                    std::make_unique<typename tree::node>(parent, action));
                return *parent.children.back();
        }

        inline Game // reached game-state; terminal
        random_playout_(Game &&game)
        {
                auto static thread_local rng
                    = std::mt19937(std::random_device{}());
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

        float // delta-payoff from perspective of player who reaches node
        simulate_(const tree::node &node)
        {
                auto delta_payoff_ = [this, &node]() {
                        auto player = node.game.current_opponent();
                        auto winner = random_playout_(Game(node.game)).winner();
                        return winner ? (winner == player ? 1 : -1) : 0;
                };

                bool trivial = node.game.is_over(); // no actual simulation made

                size_t parallelization = hyperparameters_.leaf_parallelization;
                bool   concurrent      = parallelization > 1 && not trivial;

                if (not concurrent)
                        return delta_payoff_();
                // else

                // dispatch simulations to the worker pool:

                std::vector<std::future<float> > results;
                results.reserve(parallelization);

                for (size_t i = 0; i < results.capacity(); ++i) {
                        auto task = std::packaged_task<float()>(delta_payoff_);
                        results.push_back(task.get_future());
                        asio::execution::execute(
                            asio::require(
                                worker_pool_.get_executor(),
                                asio::execution::outstanding_work.tracked),
                            std::move(task));
                }

                auto sum = std::accumulate(
                    results.begin(), results.end(), 0.0f, [](auto a, auto &b) {
                            return a + b.get();
                    });

                auto average = sum / results.size();
                return average;
        }

        void
        backpropagate_(tree::node &node, float payoff)
        {
                // payoff is seen from perspective of player who reaches node

                for (auto *it = &node; it != nullptr; it = it->parent) {
                        it->visits++;
                        it->payoff += payoff;
                        static_assert(Game::player_count() == 2);
                        payoff *= -1; // switch perspective
                }
        }

        void
        iterate_(tree &tree)
        {
                std::lock_guard lock(tree.mutex);
                auto            node = &select_(tree);
                if (!node->untried.empty())
                        node = &expand_(*node);
                backpropagate_(*node, simulate_(*node));
                iteration_count_.fetch_add(1, std::memory_order_relaxed);
        }
};

} // namespace mnkg::model::mcts
