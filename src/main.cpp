#include <csignal>
#include <iostream>
#include <print>

#include "mnk/game.hpp"
#include "mnk/presets.hpp"
using namespace mnkg::mnk;

template <class Stream>
requires std::is_base_of_v<std::basic_ios<char>, Stream> std::ostream &
operator<<(Stream &stream, const board &board)
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
reprint_game(const auto &game, const auto &title)
{
        std::cout << "\033[2J";   // Clear the entire screen
        std::cout << "\033[1;1H"; // Move the cursor back to the top-left
        std::cout << title << "!\n";
        std::cout << game.get_board() << std::endl;
}

void
cli_game()
{
        struct game_option {
                std::string title;
                game::settings (*settings)();
        };
        auto game_options = std::to_array<game_option>(
            { { "Tic-Tac-Toe", presets::tic_tac_toe },
              { "Connect Four", presets::connect_four },
              { "Gomoku", presets::gomoku } });

        std::optional<game> chosen_game = std::nullopt;
        std::println("SELECT GAME");
        for (size_t i = 0; i < game_options.size(); ++i)
                std::println("{}. {}", i + 1, game_options[i].title);
        size_t chosen_game_indice;
        while (not chosen_game.has_value()) {
                std::cin >> chosen_game_indice;
                if (chosen_game_indice <= game_options.size())
                        chosen_game.emplace(
                            game_options[chosen_game_indice - 1].settings());
                else
                        std::println("Invalid choice!");
        }
        auto &game = chosen_game.value();
        while (not game.is_over()) {

                reprint_game(game, game_options[chosen_game_indice - 1].title);

                auto player = game.get_player();

                board::position pos;

                std::cout << "Row: ", std::cin >> pos[0];
                std::cout << "Column: ", std::cin >> pos[1], std::cout << "\n";

                if (game.is_playable(player, pos))
                        game.play(player, pos);
                else
                        std::println("Invalid move!"), sleep(1);
        }
        reprint_game(game, game_options[chosen_game_indice - 1].title);
        auto result = game.get_result();
        bool tie    = std::holds_alternative<game::tie>(result);
        if (tie)
                std::println("Tie!");
        else {
                auto win = get<game::win>(result);
                std::println("Player {} wins!", win.player);
                std::println("Line: {}", win.line);
        }
}

int
main()
{
        change_screen();
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        cli_game();
        std::cout << "\nPress any key to exit...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
        restore_screen();
        return EXIT_SUCCESS;
}
