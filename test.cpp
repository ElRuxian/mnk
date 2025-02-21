#include <csignal>
#include <iostream>

#include "mnk/model/mnk.hpp"

auto game = mnk::model::mnk{};

template <class Stream>
requires std::is_base_of_v<std::basic_ios<char>, Stream> std::ostream &
operator<<(Stream &stream, const decltype(game)::board &board)
{
        const auto &[x, y] = board.get_size();
        for (int i = 0; i < x; ++i) {
                for (int j = 0; j < y; ++j) {
                        if (auto &cell = board[{ i, j }])
                                stream << cell.value();
                        else
                                stream << '-';
                }
                stream << "\n";
        }
        return stream;
}

void
change_screen()
{
        std::cout << "\033[?1049h"; // Switch to the alternate screen buffer
}
void
restore_screen()
{
        std::cout << "\033[?1049l"; // Switch back to the main screen buffer
}

void
signal_handler(int signal)
{
        restore_screen();
        exit(signal); // Exit with the signal code
}

void
reprint_game()
{
        std::cout << "\033[2J";   // Clear the entire screen
        std::cout << "\033[1;1H"; // Move the cursor back to the top-left
        std::cout << "Tic-Tac-Toe!\n";
        std::cout << game.get_board() << std::endl;
}

void
cli_game()
{
        while (not game.is_over()) {
                reprint_game();

                auto player = game.get_player();

                decltype(game)::board::position pos;
                std::cout << "Row: ", std::cin >> pos[0];
                std::cout << "Column: ", std::cin >> pos[1];

                if (game.is_playable(player, pos))
                        game.play(player, pos);
                else
                        std::cout << "Invalid move!" << std::endl, sleep(1);
        }
        auto result = game.get_result();
        bool tie    = std::holds_alternative<decltype(game)::tie>(result);
        if (tie)
                std::cout << "Tie!" << std::endl;
        else {
                auto win = std::get<decltype(game)::win>(result);
                std::cout << "Player " << win.player << " wins!" << '\n'
                          << "Line: " << win.line << std::endl;
        }
}

int
main()
{
        change_screen();
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        cli_game();
        std::cout << "Press any key to exit...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
        restore_screen();
        return EXIT_SUCCESS;
}
