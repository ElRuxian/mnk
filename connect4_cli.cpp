#include <csignal>
#include <iostream>

#include "mnk/model/game.hpp"
#include "mnk/model/games/connect_4.hpp"

auto game = mnk::model::games::connect_4;

template <class Stream>
  requires std::is_base_of_v<std::basic_ios<char>, Stream>
std::ostream& operator<<(Stream& stream, const decltype(game)::board& board) {
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
  std::cout << game.get_board() << std::endl;
}

void cli_game() {
  while (not game.get_result().has_value()) {
    reprint_game();
    int col;
    std::cout << "Column: ", std::cin >> col;
    if (col < 0 || col >= game.get_board().get_grid().get_size()[1]) {
      std::cout << "Invalid column!" << std::endl, sleep(1);
      continue;
    }
    auto move = decltype(game)::move{
        game.get_current_player_index(),
        find_sequence_end(game.get_board().get_grid(), {0, col}, {1, 0})};
    if (game.validate(move))
      game.play(move);
    else
      std::cout << "Row is full!" << std::endl, sleep(1);
  }
  auto result = game.get_result().value();
  if (result.is_tie()) {
    std::cout << "Tie!" << std::endl;
  } else {
    std::cout << "Player " << result.get_winner() << " wins!" << std::endl;
  }
}

int main() {
  change_screen();
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);
  cli_game();
  restore_screen();
  return EXIT_SUCCESS;
}
