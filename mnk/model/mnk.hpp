#pragma once

#include "game.hpp"
#include "grid.hpp"
#include "mnk/model/line.hpp"
#include "mnk/model/point.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace mnk::model {

class mnk : public game<typename grid<std::optional<size_t> >::position> {
public:
        using board = grid<std::optional<size_t> >;

        struct win {
                player_indice         player;
                line<board::position> line;
        };

        struct tie {};

        using result = std::variant<win, tie>;

private:
        board                 board_;
        size_t                turn_   = 0;
        std::optional<result> result_ = std::nullopt;

        const size_t line_span_;
        const bool   overline_;

        class play_filter {
        public:
                virtual ~play_filter() = default;

                virtual bool
                allowed(const mnk &, const player_indice &, const action &)
                    = 0;
        };
        const std::unique_ptr<play_filter> play_filter_;

        virtual std::vector<action>
        legal_actions_(player_indice player_indice) const override
        {
                auto legal_actions = std::vector<action>();
                if (is_over_())
                        return legal_actions;
                const auto &[x, y] = board_.get_size();
                for (auto i = 0; i < x; ++i)
                        for (auto j = 0; j < y; ++j) {
                                action action = { i, j };
                                if (is_playable_(player_indice, action))
                                        legal_actions.push_back(action);
                        }
                return legal_actions;
        };

        virtual float
        payoff_(player_indice player) const override
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
        is_playable_(player_indice player,
                     const action &position) const override
        {
                bool empty
                    = within(board_, position) && !board_[position].has_value();

                auto const filter = settings_.play_filter.get();

                bool allowed
                    = !filter || filter->allowed(*this, player, position);

                return empty && allowed;
        }

        virtual void
        play_(player_indice player, const action &position) override
        {
                board_[position] = player;
                turn_++;
                if (auto line = find_line(board_, position)) {
                        auto len = length<Metric::Chebyshev>(line.value()) + 1;
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
                board::position              board_size   = { 3, 3 };
                size_t                       line_span    = 3;
                bool                         overline     = true;
                std::unique_ptr<play_filter> play_filter  = nullptr;
        } settings_;

        mnk(settings &&settings) :
                game(settings.player_count), board_(settings.board_size),
                line_span_(settings.line_span), overline_(settings.overline),
                play_filter_(std::move(settings.play_filter))
        {
                assert(line_span_ > 0);
                assert(line_span_
                       <= norm<Metric::Chebyshev>(board_.get_size()));
        }

        mnk() : mnk(settings{}) {}

        inline const board &
        get_board() const noexcept
        {
                return board_;
        }

        player_indice
        get_player() const noexcept
        {
                return turn_ % initial_player_count_;
        }

        const result &
        get_result() const noexcept
        {
                return result_.value();
        }
};

} // namespace mnk::model
