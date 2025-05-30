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

        enum class preset {
                tictactoe,
                connect4,
                gomoku,
        };

public:
        game(settings &&settings) :
                board_(settings.board.size), rules_(std::move(settings.rules))
        {
        }

        game(const game &other) :
                combinatorial(other), board_(other.board_),
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
                swap(static_cast<combinatorial &>(lhs),
                     static_cast<combinatorial &>(rhs));
                std::swap(lhs.board_, rhs.board_);
                std::swap(lhs.result_, rhs.result_);
        }

        game &
        operator=(game other)
        {
                swap(*this, other);
                return *this;
        }

        const struct settings::rules &
        rules() const noexcept
        {
                return rules_;
        }

        inline const board &
        board() const noexcept
        {
                return board_;
        }

        const result &
        result() const noexcept
        {
                assert(result_.has_value());
                return result_.value();
        }

        class builder;

private:
        mnk::board                 board_;
        std::optional<mnk::result> result_ = std::nullopt;
        struct settings::rules     rules_;

        virtual std::vector<action>
        playable_actions_() const override
        {
                auto playable_actions = std::vector<action>();
                if (is_over_())
                        return playable_actions;
                for (const auto &pos : coords(board())) {
                        if (is_playable_(pos))
                                playable_actions.push_back(pos);
                }
                return playable_actions;
        };

        virtual bool
        is_playable_(const action &position) const override
        {
                if (is_over_())
                        return false;

                const auto &board  = this->board();
                const auto &filter = rules().play_filter;
                const auto &player = current_player();

                return within(board, position) && !board[position].has_value()
                       && (!filter || filter->allowed(*this, player, position));
        }

        virtual std::optional<player::index>
        winner_() const override
        {
                return is_win(*result_)
                           ? std::make_optional(std::get<win>(*result_).player)
                           : std::nullopt;
        }

        virtual void
        play_(const action &position) override
        {
                const auto &player = current_player();
                board_[position]   = player;
                for (auto line : find_lines(board_, position)) {
                        auto len = length<metric::chebyshev>(line) + 1;
                        if (rules_.overline ? len >= rules_.line_span
                                            : len == rules_.line_span) {
                                result_ = { win{ player, line } };
                                return;
                        }
                }
                if ((turn() + 1) == board_.get_cell_count() && !result_)
                        result_ = { tie{} };
                // turn_++; // incremented in combinatorial::play
        }

        virtual bool
        is_over_() const override
        {
                return result_.has_value();
        }

public:
        template <preset Preset>
        static game::settings
        configuration()
        {
                if constexpr (Preset == preset::tictactoe) {
                        return {
                                .board = { .size = { 3, 3 } },
                                .rules = { .line_span = 3 },
                        };
                } else if constexpr (Preset == preset::connect4) {
                        return {
                                .board = { .size = { 7, 6 } },
                                .rules = { .line_span   = 4,
                                           .overline    = true,
                                           .play_filter = std::make_unique<
                                               play_filter::gravity>() },
                        };
                } else if constexpr (Preset == preset::gomoku) {
                        return {
                                .board = { .size = { 19, 19 } },
                                .rules = { .line_span = 5, .overline = false },
                        };
                }
        }
};

} // namespace mnkg::model::mnk

namespace mnkg::model::game {
using mnk = model::mnk::game;
}
