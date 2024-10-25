
#include <csignal>
#include <iostream>

#include "mnk/model/game.hpp"
#include "mnk/model/line.hpp"

using namespace mnk::model;

auto board = game::board{
    {7, 6}
};
auto rules = game::rules{
    .inspectors{
                .legal_move =
            [](const game& game, const game::move& move) {
              const auto& grid = game.get_board().get_grid();
              const auto& down = point<int, 2>{1, 0};
              return find_sequence_end(grid, move.coords, down) == move.coords;
            }, .win =
            [](const game& game, const game::move& move) {
              const auto& grid = game.get_board().get_grid();
              return find_line(grid, move.coords, 4).has_value();
            }, .tie = [](const game& game,
                const game::move&) { return (game.get_board().is_full()); },
                }
};

auto thegame = game(board, rules);

template <class Stream>
  requires std::is_base_of_v<std::basic_ios<char>, Stream>
std::ostream& operator<<(Stream& stream, const game::board& board) {
  auto& grid = board.get_grid();
  auto  size = grid.get_size();
  for (int col = 0; col < size[0]; ++col) {
    for (int row = 0; row < size[1]; ++row) {
      if (auto& cell = grid[{col, row}])
        stream << cell.value();
      else
        stream << '-';
    }
    stream << "\n";
  }
  return stream;
}

void change_screen() {
  std::cout << "\033[?1049h";  // Switch to the alternate screen buffer
}
void restore_screen() {
  std::cout << "\033[?1049l";  // Switch back to the main screen buffer
}

void signal_handler(int signal) {
  restore_screen();
  exit(signal);  // Exit with the signal code
}

void reprint_game() {
  std::cout << "\033[2J";    // Clear the entire screen
  std::cout << "\033[1;1H";  // Move the cursor back to the top-left
  std::cout << "Connect Four!\n";
  std::cout << thegame.get_board() << std::endl;
}

void cli_game() {
  while (not thegame.get_result().has_value()) {
    reprint_game();
    int col;
    std::cout << "Column: ", std::cin >> col;
    if (col < 0 || col >= board.get_grid().get_size()[1]) {
      std::cout << "Invalid column!" << std::endl, sleep(1);
      continue;
    }
    auto move = game::move{
        thegame.get_current_player_index(),
        find_sequence_end(thegame.get_board().get_grid(), {0, col}, {1, 0})};
    if (thegame.validate(move))
      thegame.play(move);
    else
      std::cout << "Row is full!" << std::endl, sleep(1);
  }
  auto result = thegame.get_result().value();
  if (result.is_tie()) {
    std::cout << "Tie!" << std::endl;
  } else {
    std::cout << "Player " << result.get_winner() << " wins!" << std::endl;
  }
}

int main(int argc, char* argv[]) {
  change_screen();
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);
  cli_game();
  restore_screen();
  return 0;
}
