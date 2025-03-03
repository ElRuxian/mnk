#include <csignal>
#include <iostream>
#include <print>

#include "mcts/ai.hpp"
#include "mnk/builder.hpp"
#include "mnk/game.hpp"

using namespace mnkg::model::mnk;

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
                game (*game)();
        };
        using builder = game::builder;
        using enum builder::preset;
        auto game_options = std::to_array<game_option>(
            { { "Tic-Tac-Toe", builder::build<tictactoe> },
              { "Connect Four", builder::build<connect4> },
              { "Gomoku", builder::build<gomoku> } });

        std::optional<game> chosen_game = std::nullopt;
        std::println("SELECT GAME");
        for (size_t i = 0; i < game_options.size(); ++i)
                std::println("{}. {}", i + 1, game_options[i].title);
        size_t chosen_game_indice;
        while (not chosen_game.has_value()) {
                std::cin >> chosen_game_indice;
                if (chosen_game_indice <= game_options.size())
                        chosen_game.emplace(
                            game_options[chosen_game_indice - 1].game());
                else
                        std::println("Invalid choice!");
        }
        auto &game = chosen_game.value();
        auto  mcts = mnkg::model::mcts::mcts<class game>(game);
        while (not game.is_over()) {
                reprint_game(game, game_options[chosen_game_indice - 1].title);

                auto player = game.current_player();

                if (player == 0) {
                        board::position pos;

                        std::cout << "Row: ", std::cin >> pos[0];
                        std::cout << "Column: ", std::cin >> pos[1],
                            std::cout << "\n";

                        if (game.is_playable(player, pos)) {
                                game.play(player, pos);
                                mcts.advance(
                                    { .player = player, .action = pos });

                        } else {
                                std::println("Invalid move!"), sleep(1);
                        }
                } else {
                        constexpr auto time_limit = std::chrono::seconds(3);
                        constexpr auto iterations_limit = 10000000000;
                        mcts.expand(time_limit, iterations_limit);
                        auto move = mcts.evaluate();
                        game.play(move);
                        mcts.advance(move);
                }
        }
        reprint_game(game, game_options[chosen_game_indice - 1].title);
        auto result = game.get_result();
        if (is_tie(result))
                std::println("Tie!");
        else {
                auto data = get<win>(result);
                std::println("Player {} wins!", data.player);
                std::println("Line: {}", data.line);
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
