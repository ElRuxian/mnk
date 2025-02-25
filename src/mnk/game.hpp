#pragma once

#include "../game.hpp"
#include "action.hpp"
#include "board.hpp"
#include "play_filter.hpp"
#include "result.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace mnkg::mnk {

class game : public ::mnkg::game<action> {
public:
        struct settings {
                static const size_t player_count = 2;
                struct board {
                        point<int, 2> size = { 3, 3 };
                } board;
                struct rules {
                        size_t line_span = 3;    // aligned moves needed to win
                        bool   overline  = true; // allow extra moves to win
                        std::unique_ptr<play_filter::base> play_filter;
                } rules = {};
        };

        game(settings &&settings) :
                ::mnkg::game<action>(settings.player_count),
                board_(settings.board.size), rules_(std::move(settings.rules))
        {
        }

        game() : game(settings{}) {}

        const struct settings::rules &
        get_rules() const noexcept
        {
                return rules_;
        }

        inline const board &
        get_board() const noexcept
        {
                return board_;
        }

        player::indice
        get_player() const noexcept
        {
                return turn_ % initial_player_count_;
        }

        size_t
        get_turn() const noexcept
        {
                return turn_;
        }

        const result &
        get_result() const noexcept
        {
                return result_.value();
        }

private:
        board                  board_;
        size_t                 turn_   = 0;
        std::optional<result>  result_ = std::nullopt;
        struct settings::rules rules_;

        virtual std::vector<action>
        legal_actions_(player::indice player) const override
        {
                auto legal_actions = std::vector<action>();
                if (is_over_())
                        return legal_actions;
                for (const auto &pos : coords(get_board())) {
                        if (is_playable_(player, pos))
                                legal_actions.push_back(pos);
                }
                return legal_actions;
        };

        virtual float
        payoff_(player::indice player) const override
        {
                if (!is_over_())
                        return 0;
                const auto &result = get_result();
                bool        won    = std::holds_alternative<win>(result);
                if (!won)
                        return 0;
                auto winner = std::get<win>(result).player;
                return winner == player ? 1 : -1;
        }

        virtual bool
        is_playable_(player::indice player,
                     const action  &position) const override
        {

                const auto &board  = get_board();
                const auto &filter = get_rules().play_filter;
                bool        valid  = within(board, position);
                bool        free   = !board[position].has_value();
                bool        allowed
                    = filter && filter->allowed(*this, player, position);
                return valid && free && allowed;
        }

        virtual void
        play_(player::indice player, const action &position) override
        {

                board_[position] = player;
                turn_++;
                std::optional<mnk::result> result;
                if (auto line = find_line(board_, position)) {
                        auto len = length<metric::chebyshev>(line.value()) + 1;
                        if (rules_.overline ? len >= rules_.line_span
                                            : len == rules_.line_span)
                                result_ = { win{ player, line.value() } };
                }
                if (turn_ == board_.get_cell_count() && !result)
                        result = { tie{} };
        }

        virtual bool
        is_over_() const override
        {
                return result_.has_value();
        }
};

} // namespace mnkg::mnk
