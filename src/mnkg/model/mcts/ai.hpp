#pragma once

#include "varia/allocate_unique.hpp"
#include "varia/object_pool_allocator.hpp"
#include <algorithm>
#include <asio/thread_pool.hpp>
#include <cassert>
#include <cmath>
#include <future>
#include <model/game.hpp>
#include <model/player.hpp>
#include <mutex>
#include <random>
#include <stop_token>
#include <thread>

namespace mnkg::model::mcts {

template <class Game, typename Action = typename Game::action>
requires std::is_base_of_v<model::game::combinatorial<Action>, Game> class ai {
public:
        struct hyperparameters {

                // How many parallel simulations are run per iteration;
                // size of the simulation worker (thread) pool.
                size_t leaf_parallelization = 1;

                // UCT constant
                float exploration = std::numbers::sqrt2;

                // Maximum depth of the search tree.
                // Limits expansion, not simulation.
                std::optional<size_t> max_depth = std::nullopt;

                // Size of the memory pre-allocated for constructing nodes.
                // No further memory is allocated during the search.
                // Note: may indirectly cap tree-depth below max_depth.
                // Note: memory may be wasted if the tree is shallow.
                std::size_t memory_usage = std::pow(1024, 3) * 2; // 2GiB
        };

        ai(Game game, hyperparameters hparams = {}) :
                tree_{ .root = make_node(game) }, hyperparameters_{ hparams },
                node_memory_{ hparams.memory_usage / sizeof(node) },
                worker_pool_{ hparams.leaf_parallelization },
                search_thread_{ [this](std::stop_token stop_token) {
                        while (!stop_token.stop_requested()) {
                                iterate_(tree_);
                        }
                } }
        {
                assert(hparams.leaf_parallelization > 0);
                assert(!hparams.max_depth || *hparams.max_depth > 0);
        }

        ~ai() = default;

        typename Game::action
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
                        // for some fucking reason, this corrupts the deleter
                        // root = std::move(*target);
                        auto mid     = std::move(*target);
                        root         = std::move(mid);
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
        struct node {

                using unique_ptr = std::unique_ptr<
                    node, alloc_deleter<mnkg::object_pool_allocator<node> > >;

                Game::action                       action;
                Game                               game;
                size_t                             visits;
                float                              payoff;
                node                              *parent;
                std::vector<node::unique_ptr>      children;
                std::vector<typename Game::action> untried;

                node(Game game) : game(game), untried(game.playable_actions())
                {
                }

                node(node &parent, const Game::action &action) :
                        parent(&parent), action(action), game(parent.game)
                {
                        game.play(action);
                        untried = game.playable_actions();
                }
        };

        node::unique_ptr
        make_node(auto &&...args)
        {
                auto allocator
                    = mnkg::object_pool_allocator<node>(&node_memory_);
                return allocate_unique<node>(
                    allocator, std::forward<decltype(args)>(args)...);
        }

        struct tree {
                node::unique_ptr root;
                std::mutex       mutex;
        };

        hyperparameters                 hyperparameters_;
        mnkg::slab_memory<sizeof(node)> node_memory_;
        tree                            tree_;
        std::atomic<size_t>             iteration_count_ = { 0 };
        asio::thread_pool               worker_pool_;
        std::jthread                    search_thread_;

        float
        rate_(const node &node)
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

        node &
        select_(const tree &tree)
        {
                auto        *it        = tree.root.get();
                size_t       depth     = 0;
                const size_t max_depth = hyperparameters_.max_depth.value_or(
                    std::numeric_limits<size_t>::max());

                while (depth + 1 < max_depth && !should_select_(*it)) {
                        it = &next_(*it);
                        depth++;
                }

                return *it;
        }

        bool
        should_select_(const node &node)
        {
                bool terminal   = node.game.is_over();
                bool parent     = !node.children.empty();
                bool expandable = !node.untried.empty();
                assert(!(terminal && parent));
                return terminal || expandable;
        }

        inline node &
        next_(const node &node)
        {
                assert(!node.children.empty());
                return *std::max_element(node.children.begin(),
                                         node.children.end(),
                                         [this](const auto &a, const auto &b) {
                                                 return rate_(*a) < rate_(*b);
                                         })
                            ->get();
        }

        node &
        expand_(node &parent)
        {
                assert(not node_memory_.full());

                static thread_local std::mt19937 rng{ std::random_device{}() };

                // Pick random untried action:
                assert(!parent.untried.empty());
                std::uniform_int_distribution<size_t> distribution(
                    0, parent.untried.size() - 1);
                std::swap(parent.untried.back(),
                          parent.untried[distribution(rng)]);
                auto action = parent.untried.back();
                parent.untried.pop_back();

                // Allocate and return corresponding child node:
                parent.children.emplace_back(make_node(parent, action));
                return *parent.children.back();
        }

        inline Game
        random_playout_(Game &&game)
        {
                static thread_local std::mt19937 rng(std::random_device{}());
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
        simulate_(const node &node)
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
        backpropagate_(node &node, float payoff)
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
                if (!node->untried.empty() && !node_memory_.full())
                        node = &expand_(*node);
                backpropagate_(*node, simulate_(*node));
                iteration_count_.fetch_add(1, std::memory_order_relaxed);
        }
};

} // namespace mnkg::model::mcts
