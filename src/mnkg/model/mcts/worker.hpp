#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <thread>

namespace mnkg::model::mcts {

template <typename Tree>
class worker {
public:
        worker(Tree &tree) { start_(tree); }

        ~worker() { stop_(); }

        size_t
        iterations() const
        {
                return iteration_count_.load(std::memory_order_relaxed);
        }

private:
        std::mt19937        rng_{ std::random_device{}() };
        std::atomic<size_t> iteration_count_{ 0 };
        std::atomic<bool>   stopping_ = { false };
        std::thread         thread_;

        void
        start_(Tree &tree)
        {
                thread_ = std::thread(&worker::work_, this, std::ref(tree));
        }

        void
        stop_()
        {
                stopping_.store(true);
                if (thread_.joinable())
                        thread_.join();
        }

        void
        work_(Tree &tree)
        {
                while (!stopping_.load(std::memory_order_relaxed)) {
                        std::shared_lock lock(tree.mutex);
                        iterate_(tree);
                        iteration_count_.fetch_add(1,
                                                   std::memory_order_relaxed);
                }
                stopping_.store(false);
        }

        float
        rate_(const Tree::node &node)
        {
                // UCT (Upper Confidence Bound 1 applied to trees)
                assert(node.parent);
                std::shared_lock node_lock(node.mutex), parent_lock(node.mutex);
                constexpr size_t exploration = 2; // hiperparameter defaulted
                if (node.visits > 0)
                        return static_cast<float>(node.payoff) / node.visits
                               + std::sqrt(exploration
                                           * std::log(node.parent->visits)
                                           / node.visits);
                else
                        return std::numeric_limits<float>::infinity();
        }

        Tree::node &
        select_(const Tree &tree)
        {
                auto *current = tree.root.get();
                while (!selectable_(*current)) {
                        visit_(*current);
                        current = &next_(*current);
                }
                visit_(*current);
                return *current;
        }

        inline bool
        selectable_(const Tree::node &node)
        {
                std::shared_lock lock(node.mutex);
                return !node.untried.empty() || !node.children.empty();
        }

        inline void
        visit_(Tree::node &node)
        {
                auto lock = std::unique_lock(node.mutex);
                node.visits++;
                node.payoff--; // virtual loss
        }

        inline Tree::node &
        next_(const Tree::node &node)
        {
                std::shared_lock lock(node.mutex);
                return *std::max_element(node.children.begin(),
                                         node.children.end(),
                                         [this](const auto &a, const auto &b) {
                                                 return rate_(*a) < rate_(*b);
                                         })
                            ->get();
        }

        Tree::node &
        expand_(Tree::node &parent)
        {
                auto lock = std::unique_lock(parent.mutex);

                // pick random untried action
                assert(!parent.untried.empty());
                std::uniform_int_distribution<size_t> distribution(
                    0, parent.untried.size() - 1);
                std::swap(parent.untried.back(),
                          parent.untried[distribution(rng_)]);
                auto action = parent.untried.back();
                parent.untried.pop_back();

                // create and return corresponding child
                parent.children.emplace_back(
                    std::make_unique<typename Tree::node>(parent, action));
                return *parent.children.back();
        }

        auto // winner
        playout_(Tree::node &node)
        {
                auto game = node.game;
                while (!game.is_over()) {
                        auto actions = game.playable_actions();
                        assert(!actions.empty());
                        std::uniform_int_distribution<size_t> distribution(
                            0, actions.size() - 1);
                        const auto &action = actions[distribution(rng_)];
                        game.play(action);
                }
                return game.winner();
        }

        void
        backpropagate_(Tree::node &node, auto winner)
        {
                for (auto *it = &node; it != nullptr; it = it->parent) {
                        it->payoff++; // remove virtual loss
                        if (winner) {
                                auto player = it->game.current_opponent();
                                bool won    = player == winner;
                                it->payoff += (won ? 1 : -1);
                        }
                }
        }

        void
        iterate_(Tree &tree)
        {
                auto selected = &select_(tree);
                if (selected->game.is_over()) {
                        backpropagate_(*selected, selected->game.winner());
                } else {
                        selected    = &expand_(*selected);
                        auto winner = playout_(*selected);
                        backpropagate_(*selected, winner);
                }
        }
};
} // namespace mnkg::model::mcts
