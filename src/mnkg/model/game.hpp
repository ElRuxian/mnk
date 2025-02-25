#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "player.hpp"

namespace mnkg::model {

template <typename Action>
class game {
public:
        using action = Action;

private:
        virtual std::vector<Action>
        legal_actions_(player::indice player) const = 0;

        virtual float
        payoff_(player::indice player) const
            = 0;

        virtual bool
        is_playable_(player::indice player, const action &action) const
        {
                using std::ranges::contains;
                return is_legal_player(player)
                       && contains(legal_actions_(player), action);
        }

        virtual void
        play_(player::indice player, const action &action)
            = 0;

        virtual bool
        is_over_() const
        {
                for (player::indice i = 0; i < initial_player_count_; ++i)
                        if (!legal_actions_(i).empty())
                                return false;
                return true;
        }

public:
        virtual ~game() = default;

        game(size_t player_count) : initial_player_count_(player_count)
        {
                assert(initial_player_count_ > 0);
        }

        inline bool
        is_legal_player(player::indice player) const
        {
                return player < initial_player_count_;
        }

        inline float
        payoff(player::indice player) const
        {
                assert(is_legal_player(player));
                return payoff_(player);
        }

        inline std::vector<Action> // factual until next play
        legal_actions(player::indice player) const
        {
                assert(is_legal_player(player));
                auto actions = legal_actions_(player);
                assert(!is_over() || actions.empty());
                return actions;
        }

        bool
        is_playable(player::indice player, const action &action) const
        {
                bool playable = is_playable_(player, action);
                assert(playable == game::is_playable_(player, action));
                return playable;
        }

        inline void
        play(player::indice player, const action &action)
        {
                assert(is_playable(player, action));
                play_(player, action);
        }

        virtual bool
        is_over() const
        {
                bool over = is_over_();
                assert(over == game::is_over_());
                return over;
        }

protected:
        std::size_t initial_player_count_;
};

} // namespace mnkg::model
