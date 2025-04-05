#pragma once
#include "model/mcts/ai.hpp"
#include "model/mnk/game.hpp"
#include "view/game.hpp"
#include <array>

namespace mnkg::control {

enum class player { human, ai };
class game {
public:
        struct settings {
                std::array<player, model::mnk::game::player_count()> players;
                model::mnk::game::settings                           game;
                std::string                                          title;
                view::game::style                                    style;
        };

public:
        game(settings &&settings) :
                model_(std::move(settings.game)),
                gui_({ .title     = settings.title,
                       .style     = settings.style,
                       .callbacks = { .on_cell_selected =
                                          [this](auto coords) {
                                                  on_cell_selected_(coords);
                                          } },
                       .board_size
                       = point<unsigned int, 2>(model_.board().get_size()) }),
                players_(std::move(settings.players))
        {
                using std::ranges::contains;
                bool run_mcts = contains(settings.players, player::ai);
                if (run_mcts) {
                        using mcts       = model::mcts::ai<decltype(model_)>;
                        auto concurrency = std::thread::hardware_concurrency();
                        auto hparams     = mcts::hyperparameters{
                                    .leaf_parallelization = concurrency,
                        };
                        mcts_ = std::make_unique<mcts>(model_, hparams);
                }

                on_new_turn_();
        }

        void
        run()
        {
                gui_.run();
        }

private:
        model::mnk::game                                     model_;
        view::game                                           gui_;
        std::unique_ptr<model::mcts::ai<decltype(model_)> >  mcts_;
        std::array<player, model::mnk::game::player_count()> players_;
        std::vector<model::mnk::action>                      history_;

private:
        void
        on_cell_selected_(auto coords)
        {
                play_(coords);
        }

        void
        play_(auto move)
        {
                gui_.draw_stone(move);
                model_.play(move);
                history_.push_back(move);
                if (mcts_)
                        mcts_->advance(move);
                on_new_turn_();
        }

        void
        on_new_turn_()
        {
                if (model_.is_over())
                        return on_game_over_();
                gui_.set_stone_skin(model_.current_player());
                if (players_[model_.current_player()] == player::human) {
                        gui_.set_selectable_cells(model_.playable_actions());
                } else {
                        gui_.set_selectable_cells({});
                        ai_move_();
                }
        }

        void
        ai_move_()
        {
                assert(players_[game_.current_player()] == player::ai);
                assert(mcts_);
                sleep(1); // let it think
                play_(mcts_->evaluate());
        }

        void
        on_game_over_()
        {
                assert(game_.is_over());
                if (is_win(model_.result())) {
                        auto        win = get<model::mnk::win>(model_.result());
                        const auto &line = win.line;
                        for (auto &cell : covered_cells(model_.board(), line))
                                gui_.highlight_stone(cell);
                        gui_.set_selectable_cells({});
                        return;
                }
        }
};

} // namespace mnkg::control
