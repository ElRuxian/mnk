// Interface of a combinatorial game: a zero-sum, turn-based game with perfect
// information and deterministic gameplay.
//
// Each implementation defines a specific game.
// Each instance represents a specific game state, modifable by playing.
// To play, some action is input. Action type is a template parameter.
// Each play is implicitly made as the current player.
//
// Note that this interface enforces constraints such as a fixed player count
// and turn order. This is intentional, as it allows for more efficient
// algorithms and data structures while still aligning with the project's goals.

#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <type_traits>
#include <vector>

#include "player.hpp"

namespace mnkg::model::game {

template <typename Action>
class combinatorial {
public:
        using action = Action;

        combinatorial()          = default;
        virtual ~combinatorial() = default;

        virtual std::unique_ptr<combinatorial>
        clone() const = 0;

protected:
        size_t turn_ = 0;

        combinatorial(const combinatorial &other) : turn_(other.turn_) {}

        friend void
        swap(combinatorial &lhs, combinatorial &rhs)
        {
                using std::swap;
                swap(lhs.turn_, rhs.turn_);
        }

public:
        size_t
        turn() const
        {
                return turn_;
        }

        consteval static size_t
        player_count()
        {
                // restricted to two-player games
                return 2;
        }

        player::index
        current_player() const
        {
                // restricted to round-robin turn order
                return turn() % player_count();
        }

        player::index
        current_opponent() const
        {
                static_assert(player_count() == 2);
                return (current_player() + 1) % player_count();
        }

        bool
        is_legal(player::index player) const
        {
                static_assert(!std::is_signed_v<player::index>);
                return player < player_count();
        }

        inline std::vector<Action> // factual until next play
        playable_actions() const
        {
                auto actions = playable_actions_();
                assert(!is_over() || actions.empty());
                return actions;
        }

        bool
        is_over() const
        {
                bool over = is_over_();
                assert(over == combinatorial::is_over_());
                return over;
        }

        std::optional<player::index>
        winner() const
        {
                assert(is_over());
                return winner_();
        }

        bool
        is_draw() const
        {
                return is_over() && !winner().has_value();
        }

        inline void
        play(const action &action)
        {
                assert(is_playable(action));
                play_(action);
                turn_++;
        }

        bool
        is_playable(const action &action) const
        {
                bool playable = is_playable_(action);
                assert(playable == combinatorial::is_playable_(action));
                return playable;
        }

private:
        virtual std::vector<Action>
        playable_actions_() const = 0;

        virtual std::optional<player::index> // nullopt for draw
        winner_() const = 0;

        virtual bool
        is_playable_(const action &action) const
        {
                using std::ranges::contains;
                return contains(playable_actions_(), action);
        }

        virtual void
        play_(const action &action)
            = 0;

        virtual bool
        is_over_() const
        {
                return playable_actions_().empty();
        }
};

} // namespace mnkg::model::game
