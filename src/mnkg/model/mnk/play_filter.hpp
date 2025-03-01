#pragma once
#include "action.hpp"
#include "model/player.hpp"
#include <memory>

namespace mnkg::model::mnk {

class game; // forward declaration

namespace play_filter {

class base {
private:
        virtual bool
        allowed_(const game &game, const player::indice &player,
                 const action &action)
            = 0;

public:
        bool
        allowed(const game &game, const player::indice &player,
                const action &action)
        {
                return allowed_(game, player, action);
        }

        virtual std::unique_ptr<base>
        clone() const = 0;

        virtual ~base() = default;
};

// Filters out actions that don't "fall in a straight line"
// Intended for games like Connect Four
class gravity : public play_filter::base {
public:
        gravity(board::position direction = { 1, 0 }) : direction_(direction)
        {
                assert(norm<metric::chebyshev>(direction) == 1);
        }

        gravity(const gravity &other) = default;

        virtual std::unique_ptr<base>
        clone() const override
        {
                return std::make_unique<gravity>(*this);
        };

private:
        board::position direction_;

        bool
        allowed_(const game &game, const player::indice &player,
                 const action &action) override;
};

// Filters out actions that are not close to previous actions
class proximity : public play_filter::base {
public:
        proximity(size_t range = 1) : range_(range) {}

        proximity(const proximity &other) = default;

        virtual std::unique_ptr<base>
        clone() const override
        {
                return std::make_unique<proximity>(*this);
        };

private:
        size_t range_;

        bool
        allowed_(const game &game, const player::indice &player,
                 const action &action) override;
};

class composite; // TODO

} // namespace play_filter
} // namespace mnkg::model::mnk
