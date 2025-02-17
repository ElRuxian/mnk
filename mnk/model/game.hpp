#pragma once

#include <cassert>
#include <vector>
namespace mnk::model {

template <typename Action>
class game {
public:
        using action        = Action;
        using player_indice = std::size_t;

private:
        virtual std::vector<Action>
        legal_actions_(player_indice player) const = 0;

        virtual float
        payoff_(player_indice player) const
            = 0;

        virtual bool
        is_playable_(player_indice player, const action &action) const
        {
                return is_legal_player(player)
                       && legal_actions_(player).contains(action);
        }

        virtual void
        play_(player_indice player, const action &action)
            = 0;

        virtual bool
        is_over_() const
        {
                for (player_indice i = 0; i < initial_player_count_; ++i)
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
        is_legal_player(player_indice player) const
        {
                return player < initial_player_count_;
        }

        inline float
        payoff(player_indice player) const
        {
                assert(is_legal_player(player));
                return payoff_(player);
        }

        inline std::vector<Action> // factual until next play
        legal_actions(player_indice player) const
        {
                assert(is_legal_player(player));
                auto actions = legal_actions_(player);
                assert(!is_over() || actions.empty());
                return actions;
        }

        bool
        is_playable(player_indice player, const action &action) const
        {
                bool playable = is_playable_(player, action);
                assert(playable == game::is_playable_(player, action));
                return playable;
        }

        inline void
        play(player_indice player, const action &action)
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
}; // namespace mnk::model
