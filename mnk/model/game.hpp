#pragma once

#include <cassert>
#include <functional>
#include <optional>
#include <type_traits>

#include "grid.hpp"

namespace mnk::model {

class game {
 public:
  static constexpr int max_players = 2;  // May break class if changed.
  using player_index_t             = uint;

  class board {
   private:
    grid<std::optional<player_index_t>> grid_;
    size_t                              stone_count_{0};

   public:
    using grid = decltype(grid_);

    board(point<int, 2> size) : grid_(size) {}

    bool placeable(const player_index_t& stone [[maybe_unused]],
                   const grid::point&    coords) const {
      bool replacement = grid_[coords].has_value();
      return inside(grid_, coords) && not replacement;
    }

    void place(player_index_t stone, const grid::point& coords) {
      assert(placeable(stone, coords));
      grid_[coords] = stone;
      stone_count_++;
    }

    bool is_full() const noexcept {
      return stone_count_ == grid_.get_cell_count();
    }

    const grid& get_grid() const noexcept { return grid_; }
  };

  struct move {
    player_index_t     stone;
    board::grid::point coords;
  };

  struct result {
    std::optional<id_t>     winner;
    static constexpr result make_tie() { return {}; }
    static constexpr result make_win(id_t winner) { return {winner}; }
    bool                    is_tie() const { return not winner.has_value(); }
    bool                    is_win() const { return winner.has_value(); }
    bool                    get_winner() const {
      assert(is_win());
      return winner.value();
    }
  };

  struct rules {
    // Strategy pattern allows customization of game logic
    template <typename ReturnType>
      requires std::is_convertible_v<ReturnType, bool>
    using inspector =
        std::function<ReturnType(const game& context, const move& inspected)>;

    struct {
      // NOTE: non-customizable game logic is ensured outside here; viz.:
      // * legal moves cannot refer to positions outside the board;
      // * legal moves cannot place stones on top of existing ones,
      // * if the board is full, the game is over;
      // * if both win and tie inspections are true, win takes precedence.
      inspector<bool> legal_move;  // "Does current game-state allow this move?"
      inspector<bool> win;         // "Would this move win the game?"
      inspector<bool> tie;         // "Would this move tie the game?"
    } inspectors;
  };
  static_assert(std::is_aggregate_v<rules>);

  struct prototypes;

 protected:
  board                 board_;
  rules                 rules_;
  std::optional<result> result_;
  uint                  turn_;
  std::optional<move>   last_move_;

 public:
  game(board board, rules rules) : board_(board), rules_(rules) {}
  game(const game& other) = default;
  game(game&& other)      = default;
  ~game()                 = default;

  const auto& get_board() const noexcept { return board_; }
  auto get_current_player_index() const noexcept { return turn_ % max_players; }
  const auto&                get_turn() const noexcept { return turn_; }
  const auto&                get_result() const noexcept { return result_; }
  const std::optional<move>& get_last_move() const noexcept {
    return last_move_;
  }

  bool is_finished() const noexcept { return get_result().has_value(); }

  bool validate(const move& move) const noexcept {
    return board_.placeable(move.stone, move.coords) &&
           rules_.inspectors.legal_move(*this, move);
  };

  void play(const move& move) {
    assert(validate(move));
    board_.place(move.stone, move.coords);
    turn_++;
    last_move_ = move;
    if (rules_.inspectors.win(*this, move)) {
      result_ = result::make_win(move.stone);
    } else if (rules_.inspectors.tie(*this, move) || board_.is_full()) {
      result_ = result::make_tie();
    }
  }
};

}  // namespace mnk::model
