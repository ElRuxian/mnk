#pragma once

#include "../game.hpp"
#include "action.hpp"
#include "board.hpp"
#include "result.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace mnkg::mnk {

class game : public ::mnkg::game<action> {
public:
        class play_filter {
        public:
                virtual ~play_filter() = default;

                virtual bool
                allowed(const game &, const player::indice &, const action &)
                    = 0;
        };

private:
        board                 board_;
        size_t                turn_   = 0;
        std::optional<result> result_ = std::nullopt;

        const size_t line_span_; // how many aligned moves are needed to win
        const bool   overline_;  // win by aligning more than line_span_ moves

        const std::unique_ptr<play_filter> play_filter_;

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
                if (!result_.has_value())
                        return 0;
                auto result = result_.value();
                bool is_win = std::holds_alternative<win>(result);
                if (!is_win)
                        return 0;
                auto winner = std::get<win>(result).player;
                return winner == player ? 1 : -1;
        }

        virtual bool
        is_playable_(player::indice player,
                     const action  &position) const override
        {
                bool empty
                    = within(board_, position) && !board_[position].has_value();

                auto const filter = play_filter_.get();

                bool allowed
                    = !filter || filter->allowed(*this, player, position);

                return empty && allowed;
        }

        virtual void
        play_(player::indice player, const action &position) override
        {
                board_[position] = player;
                turn_++;
                if (auto line = find_line(board_, position)) {
                        auto len = length<metric::chebyshev>(line.value()) + 1;
                        if (overline_ ? len >= line_span_ : len == line_span_)
                                result_ = { win{ player, line.value() } };
                }
                if (turn_ == board_.get_cell_count() && !result_.has_value())
                        result_ = { tie{} };
        }

        virtual bool
        is_over_() const override
        {
                return result_.has_value();
        }

public:
        struct settings {
                static const size_t          player_count = 2;
                point<int, 2>                board_size   = { 3, 3 };
                size_t                       line_span    = 3;
                bool                         overline     = true;
                std::unique_ptr<play_filter> play_filter  = nullptr;
        };

        game(settings &&settings) :
                ::mnkg::game<action>(settings.player_count),
                board_(settings.board_size), line_span_(settings.line_span),
                overline_(settings.overline),
                play_filter_(std::move(settings.play_filter))
        {
                assert(line_span_ > 0);
                player::indice current_player;
                assert(line_span_
                       <= norm<metric::chebyshev>(board_.get_size()));
        }

        game() : game(settings{}) {}

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
};

} // namespace mnkg::mnk
