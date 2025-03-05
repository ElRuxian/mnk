#pragma once

#include "node.hpp"
#include <memory>
#include <shared_mutex>

namespace mnkg::model::mcts {

template <class Game>
struct tree {
        using node = class node<Game>;
        std::unique_ptr<node> root;
        std::shared_mutex     mutex;

        tree(Game game) : root{ std::make_unique<node>(game) } {}
};

} // namespace mnkg::model::mcts
