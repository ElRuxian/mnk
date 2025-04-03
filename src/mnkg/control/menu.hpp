#include "control/game.hpp"
#include "model/mnk/game.hpp"
#include "model/mnk/play_filter.hpp"

#include <SFML/Graphics.hpp>
#include <array>
#include <imgui-SFML.h>
#include <imgui.h>

// TODO: improve architecture:
//        - move ImGUI code to a separate view module?
//        - decouple config menu from control::game?

namespace mnkg::control {

class configuation_menu {

public:
        struct output {
                int  board_width;
                int  board_height;
                int  line_length;
                bool allow_overline;
                bool gravity;
        };

private:
        sf::RenderWindow window_;

public:
        configuation_menu() :
                window_(sf::VideoMode({ 800, 600 }), "MNKG Menu",
                        sf::Style::Close)
        {
                if (!ImGui::SFML::Init(window_))
                        throw std::runtime_error(
                            "Failed to initialize ImGui-SFML.");
        }

        std::optional<control::game::settings>
        run()
        {
                struct player {
                        control::player type;
                        std::string     name;
                };

                auto player_options = std::to_array<player>({
                    { .type = control::player::human, .name = "Human" },
                    { .type = control::player::ai, .name = "AI" },
                });

                struct game_configuration {
                        int  board_width, board_height;
                        int  line_length;
                        bool allow_overline;
                        bool gravity;
                } game;

                std::array<control::player, 2> players;

                struct preset {
                        std::string        name;
                        game_configuration config;
                };

                auto preset_options = std::to_array<preset>({
                    { "Tic-Tac-Toe",
                      { .board_width    = 3,
                        .board_height   = 3,
                        .line_length    = 3,
                        .allow_overline = false,
                        .gravity        = false } },
                    { "Connect-4",
                      { .board_width    = 7,
                        .board_height   = 6,
                        .line_length    = 4,
                        .allow_overline = true,
                        .gravity        = true } },
                });

                game    = preset_options[0].config;
                players = { control::player::human, control::player::ai };

                sf::Clock clock;
                while (window_.isOpen()) {

                        while (auto event = window_.pollEvent()) {
                                ImGui::SFML::ProcessEvent(window_, *event);
                                if (event->is<sf::Event::Closed>())
                                        window_.close();
                        }

                        ImGui::SFML::Update(window_, clock.restart());

                        ImGui::SetNextWindowPos(ImVec2(0, 0));
                        ImGui::SetNextWindowSize(
                            ImVec2(window_.getSize().x, window_.getSize().y));
                        ImGui::Begin("MNKG Menu",
                                     nullptr,
                                     ImGuiWindowFlags_NoTitleBar
                                         | ImGuiWindowFlags_NoResize
                                         | ImGuiWindowFlags_NoMove);
                        // float window_width   = window_.getSize().x;
                        // float window_height  = window_.getSize().y;
                        // float content_width  = 300.0f;
                        // float content_height = 200.0f;
                        // ImGui::SetCursorPos(
                        //     ImVec2((window_width - content_width) / 2,
                        //            (window_height - content_height) / 2));

                        ImGui::Text("Board size:");
                        ImGui::PushItemWidth(100);
                        ImGui::SameLine();
                        if (ImGui::InputInt(
                                "##board_width", &game.board_width, 1, 10))
                                game.board_width
                                    = std::clamp(game.board_width, 1, 10);
                        ImGui::SameLine();
                        ImGui::Text(":");
                        ImGui::SameLine();
                        if (ImGui::InputInt(
                                "##board_height", &game.board_height, 1, 10))
                                game.board_height
                                    = std::clamp(game.board_height, 1, 10);

                        ImGui::Text("Line length");
                        ImGui::SameLine();
                        if (ImGui::InputInt(
                                "##line_length", &game.line_length, 1, 10))
                                game.line_length
                                    = std::clamp(game.line_length, 1, 10);

                        ImGui::Checkbox("Allow overline", &game.allow_overline);

                        ImGui::Checkbox("Gravity", &game.gravity);

                        if (ImGui::Button("Preset.."))
                                ImGui::OpenPopup("preset_popup");
                        if (ImGui::BeginPopup("preset_popup")) {
                                for (const auto &preset : preset_options) {
                                        const auto &name = preset.name.c_str();
                                        if (ImGui::Selectable(name))
                                                game = preset.config;
                                }
                                ImGui::EndPopup();
                        }

                        ImGui::SameLine();

                        if (ImGui::Button("Start game")) {
                                control::game::settings settings;
                                settings.game.board.size
                                    = { game.board_width, game.board_height };
                                settings.game.rules
                                    = { .line_span
                                        = static_cast<uint>(game.line_length),
                                        .overline = game.allow_overline,
                                        .play_filter
                                        = game.gravity
                                              ? std::make_unique<
                                                    model::mnk::play_filter::
                                                        gravity>()
                                              : nullptr };
                                settings.style   = view::game::style::go;
                                settings.players = players;
                                settings.title   = "MNKG Game";
                                ImGui::SFML::Shutdown();
                                return settings;
                        };

                        ImGui::End();

                        window_.clear(sf::Color(50, 50, 50));
                        ImGui::SFML::Render(window_);
                        window_.display();
                }

                ImGui::SFML::Shutdown();
                return {};
        }
};

} // namespace mnkg::control
