#pragma once

#include "../game.hpp"
#include "action.hpp"
#include "board.hpp"
#include "play_filter.hpp"
#include "result.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace mnkg::model::mnk {

class game : public model::game::combinatorial<action> {
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
                board_(settings.board.size), rules_(std::move(settings.rules))
        {
        }

        game(const game &other) :
                board_(other.board_), turn_(other.turn_),
                result_(other.result_),
                rules_({ .line_span   = other.rules_.line_span,
                         .overline    = other.rules_.overline,
                         .play_filter = other.rules_.play_filter
                                            ? other.rules_.play_filter->clone()
                                            : nullptr })
        {
        }

        virtual std::unique_ptr<game::combinatorial>
        clone() const override
        {
                return std::make_unique<game>(*this);
        }

        friend void
        swap(game &lhs, game &rhs)
        {
                using std::swap;
                swap(lhs.board_, rhs.board_);
                swap(lhs.turn_, rhs.turn_);
                swap(lhs.result_, rhs.result_);
        }

        game &
        operator=(game other)
        {
                swap(*this, other);
                return *this;
        }

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

        const result &
        get_result() const noexcept
        {
                assert(result_.has_value());
                return result_.value();
        }

        static constexpr size_t
        get_player_count() noexcept
        {
                return settings::player_count;
        }

        class builder;

private:
        board                  board_;
        size_t                 turn_   = 0;
        std::optional<result>  result_ = std::nullopt;
        struct settings::rules rules_;

        virtual std::vector<action>
        legal_actions_() const override
        {
                auto legal_actions = std::vector<action>();
                if (is_over_())
                        return legal_actions;
                for (const auto &pos : coords(get_board())) {
                        if (is_playable_(pos))
                                legal_actions.push_back(pos);
                }
                return legal_actions;
        };

        virtual payoff_t
        payoff_(player::indice player) const override
        {
                if (!is_over_())
                        return 0;
                const auto &result = get_result();
                if (!is_win(result))
                        return 0;
                auto winner   = std::get<win>(result).player;
                auto max_turn = get_board().get_cell_count();
                auto payoff   = 1 + max_turn - turn_;
                return winner == player ? payoff : -2 * payoff;
        }

        virtual bool
        is_playable_(const action &position) const override
        {

                const auto &board  = get_board();
                const auto &filter = get_rules().play_filter;
                const auto &player = current_player();

                return within(board, position) && !board[position].has_value()
                       && (!filter || filter->allowed(*this, player, position));
        }

        virtual std::optional<player::indice>
        winner_() const override
        {
                return std::get<win>(result_.value()).player;
        }

        virtual void
        play_(const action &position) override
        {
                const auto &player = current_player();
                board_[position]   = player;
                turn_++;
                if (auto line = find_line(board_, position)) {
                        auto len = length<metric::chebyshev>(line.value()) + 1;
                        if (rules_.overline ? len >= rules_.line_span
                                            : len == rules_.line_span)
                                result_ = { win{ player, line.value() } };
                }
                if (turn_ == board_.get_cell_count() && !result_)
                        result_ = { tie{} };
        }

        virtual bool
        is_over_() const override
        {
                return result_.has_value();
        }
};

} // namespace mnkg::model::mnk
