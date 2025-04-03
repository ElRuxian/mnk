#include "control/game.hpp"
#include "model/mnk/game.hpp"
#include "model/mnk/play_filter.hpp"
#include "varia/point.hpp"

#include <SFML/Graphics.hpp>
#include <array>
#include <imgui-SFML.h>
#include <imgui.h>

// TODO: improve architecture:
//        - move ImGUI code to a separate view module?
//        - decouple config menu from control::game?

namespace mnkg::control {

namespace widgets {

void
board_size_selector(std::string name, point<int, 2> *output,
                    point<int, 2> max_selectable,
                    point<int, 2> initial_selection = { 3, 3 },
                    int square_size = 20, int padding = 4)
{
        assert(output);

        auto selector_size
            = point<float, 2>(max_selectable) * (square_size + padding);
        selector_size += make_point<float, 2>(padding);

        point<float, 2> origin = ImGui::GetCursorPos();

        ImGui::BeginChild(
            name.c_str(), selector_size, true, ImGuiWindowFlags_NoScrollbar);

        ImGui::InvisibleButton("##", selector_size);
        if (ImGui::IsItemActive()) {
                point<float, 2> mouse_position = ImGui::GetIO().MousePos;
                mouse_position -= origin + make_point<float, 2>(padding);
                auto stride = square_size + padding;
                *output     = point<int, 2>(mouse_position / stride);
                *output += { 1, 1 }; // coord (0, 0) selects size (1, 1)
                for (auto i = 0; i < 2; ++i)
                        (*output)[i]
                            = std::clamp((*output)[i], 0, max_selectable[i]);
        }

        auto *draw_list = ImGui::GetWindowDrawList();

        for (float y = 0; y < max_selectable[1]; ++y) {
                for (float x = 0; x < max_selectable[0]; ++x) {
                        auto position = point{ x, y };
                        position *= static_cast<float>(square_size + padding);
                        position += origin + make_point<float, 2>(padding);
                        auto size = make_point<float, 2>(square_size);
                        bool higlighted
                            = (x < (*output)[0] && y < (*output)[1]);
                        ImU32 color = higlighted ? IM_COL32(100, 200, 100, 255)
                                                 : IM_COL32(200, 200, 200, 255);
                        draw_list->AddRectFilled(
                            position, position + size, color);
                        draw_list->AddRect(
                            position, position + size, IM_COL32(0, 0, 0, 255));
                }
        }

        ImGui::EndChild();
}
} // namespace widgets

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
                        point<int, 2>     board_size;
                        int               line_length;
                        bool              allow_overline;
                        bool              gravity;
                        view::game::style style;
                } game;

                std::array<control::player, 2> players;

                struct preset {
                        std::string        name;
                        game_configuration config;
                };

                auto preset_options = std::to_array<preset>({
                    { "Tic-Tac-Toe",
                      { .board_size     = { 3, 3 },
                        .line_length    = 3,
                        .allow_overline = false,
                        .gravity        = false } },
                    { "Connect-4",
                      { .board_size     = { 7, 6 },
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

                        widgets::board_size_selector("Board size",
                                                     &game.board_size,
                                                     { 10, 10 },
                                                     game.board_size);
                        ImGui::Text("Line length:");
                        ImGui::SameLine();
                        ImGui::AlignTextToFramePadding();
                        ImGui::SetNextItemWidth(100);
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
                                settings.game.board.size = game.board_size;
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
