#include "gui.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Mouse.hpp>
#include <array>
#include <cassert>
#include <memory>
#include <utility>

#include "varia/point.hpp"

namespace mnkg::view {

static constexpr sf::Vector2u cell_viewport_size = { 32u, 32u };

enum class space {
        grid,     // board cells
        viewport, // pixels
};

struct board {
        const point<uint, 2> grid_size;

        constexpr sf::Vector2u
                  viewport_size() const
        {
                return transform(
                    std::multiplies{}, { cell_viewport_size }, grid_size);
        }

        constexpr inline sf::Vector2f
        map_grid_to_view(auto cell_coord) const
        {
                return transform(std::multiplies{},
                                 point<float, 2>{ cell_coord },
                                 point<float, 2>{ point(cell_viewport_size) });
        }

        constexpr inline point<int, 2>
        map_view_to_grid(auto position) const
        {
                return transform(std::divides{},
                                 point<int, 2>{ position },
                                 point<int, 2>{ point(cell_viewport_size) });
        }
};

struct stone {
        constexpr static float size_factor = 0.8; // relative to cell
        uint                   variant;           // simple index
        // Equal stones (same variant) look the same.
};

template <style Style, typename Renderable>
sf::Texture
texture(const Renderable &)
{
        assert(not "Texture implementation");
}

template <typename Renderable>
sf::Texture
texture(const Renderable &renderable, style style)
{
        // HACK: Shit ass switch. Must be refactored later.
        //       "Make it work first".
        switch (style) {
        case style::tictactoe:
                return texture<style::tictactoe>(renderable);
        case style::connect_four:
                return texture<style::connect_four>(renderable);
        case style::go:
                return texture<style::go>(renderable);
        }
}

template <>
sf::Texture
texture<style::tictactoe, board>(const board &board)
{

        auto texture = sf::RenderTexture(board.viewport_size());

        texture.clear(sf::Color::White);

        sf::RectangleShape line;
        constexpr auto     line_thickness = 2; // arbitrary; looks ok

        for (uint i = 1; i < board.grid_size[0]; ++i) {
                auto size = sf::Vector2f(line_thickness, texture.getSize().y);
                line.setSize(size);
                auto position = sf::Vector2<float>(i * cell_viewport_size.y, 0);
                line.setPosition(position);
                line.setFillColor(sf::Color::Black);
                texture.draw(line);
        }

        for (uint i = 1; i < board.grid_size[1]; ++i) {
                auto size = sf::Vector2f(texture.getSize().x, line_thickness);
                line.setSize(size);
                auto position = sf::Vector2<float>(0, i * cell_viewport_size.y);
                line.setPosition(position);
                line.setFillColor(sf::Color::Black);
                texture.draw(line);
        }

        texture.display();
        return texture.getTexture();
};

template <>
sf::Texture
texture<style::connect_four, board>(const board &board)
{

        auto texture = sf::RenderTexture(board.viewport_size());

        texture.clear(sf::Color::Blue);

        // TODO: refactor
        //       (this block of code it repeated in texture<tic_tac_toe, stone>)
        constexpr auto max_dimension
            = std::max(cell_viewport_size.x, cell_viewport_size.y);
        constexpr auto  radius = max_dimension * stone::size_factor / 2;
        sf::CircleShape shape(radius);

        shape.setFillColor(sf::Color::White);

        for (auto x = 0; x < board.grid_size[0]; ++x)
                for (auto y = 0; y < board.grid_size[1]; ++y) {
                        auto coord    = point<int, 2>{ x, y };
                        auto position = board.map_grid_to_view(coord);
                        position += sf::Vector2f(cell_viewport_size) / 2.f;
                        position -= sf::Vector2f(radius, radius);
                        shape.setPosition(position);
                        texture.draw(shape);
                }

        texture.display();
        return texture.getTexture();
};

template <>
sf::Texture
texture<style::go, board>(const board &board)
{

        auto texture = sf::RenderTexture(board.viewport_size());

        sf::Color wood_color = { 222, 184, 135 };
        texture.clear(wood_color);

        constexpr auto max_dimension
            = std::max(cell_viewport_size.x, cell_viewport_size.y);

        sf::RectangleShape line;
        constexpr float    line_thickness = max_dimension * 0.1f;

        for (uint x = 0; x < board.grid_size[0]; ++x) {
                auto size = sf::Vector2f(
                    line_thickness, texture.getSize().y - cell_viewport_size.y);
                line.setSize(size);
                auto position = sf::Vector2<float>(x * cell_viewport_size.x, 0);
                position += { cell_viewport_size.x / 2.f,
                              cell_viewport_size.y / 2.f };
                line.setPosition(position);
                line.setFillColor(sf::Color::Black);
                texture.draw(line);
        }

        for (uint y = 0; y < board.grid_size[1]; ++y) {
                auto size = sf::Vector2f(
                    texture.getSize().x - cell_viewport_size.x, line_thickness);
                line.setSize(size);
                auto position = sf::Vector2<float>(0, y * cell_viewport_size.y);
                position += { cell_viewport_size.x / 2.f,
                              cell_viewport_size.y / 2.f };
                line.setPosition(position);
                line.setFillColor(sf::Color::Black);
                texture.draw(line);
        }

        texture.display();
        return texture.getTexture();
};

template <>
sf::Texture
texture<style::connect_four, stone>(const stone &stone)
{
        auto           texture = sf::RenderTexture(cell_viewport_size);
        constexpr auto max_dimension
            = std::max(cell_viewport_size.x, cell_viewport_size.y);
        constexpr auto  radius = max_dimension * stone::size_factor / 2;
        sf::CircleShape shape(radius);
        shape.setOrigin({ radius, radius }); // center
        auto center = static_cast<float>(max_dimension) / 2;
        shape.setPosition({ center, center });
        switch (stone.variant) {
        case 0:
                shape.setFillColor(sf::Color::Red);
                break;
        case 1:
                shape.setFillColor(sf::Color::Yellow);
                break;
        default:
                assert("Unrecognized stone variant");
                std::unreachable(); // test the code!
        }
        texture.clear(sf::Color::Transparent);
        texture.draw(shape);
        texture.display();
        return texture.getTexture();
};

template <>
sf::Texture
texture<style::go, stone>(const stone &stone)
{
        auto           texture = sf::RenderTexture(cell_viewport_size);
        constexpr auto max_radius
            = std::max(cell_viewport_size.x, cell_viewport_size.y);
        constexpr auto  radius = max_radius * stone::size_factor / 2;
        sf::CircleShape shape(radius);
        shape.setOrigin({ radius, radius }); // center
        auto center = static_cast<float>(max_radius) / 2;
        shape.setPosition({ center, center });
        switch (stone.variant) {
        case 0:
                shape.setFillColor(sf::Color::White);
                break;
        case 1:
                shape.setFillColor(sf::Color::Black);
                break;
        default:
                assert("Unrecognized stone variant");
                std::unreachable(); // test the code!
        }
        texture.clear(sf::Color::Transparent);
        texture.draw(shape);
        texture.display();
        return texture.getTexture();
};

template <>
sf::Texture
texture<style::tictactoe, stone>(const stone &stone)
{
        sf::RenderTexture texture(cell_viewport_size);

        constexpr float max_dimension
            = std::max(cell_viewport_size.x, cell_viewport_size.y);
        constexpr float line_thickness = max_dimension * 0.1f;

        texture.clear(sf::Color::Transparent);

        if (stone.variant == 0) { // draw cross (X) sf::RectangleShape line1;
                sf::RectangleShape line1, line2;

                auto  center      = point<float, 2>(cell_viewport_size) / 2;
                float line_length = max_dimension * 0.8f;

                auto color = sf::Color::Red;

                // Configure the first diagonal line
                line1.setSize({ line_length, line_thickness });
                line1.setOrigin({ line_length / 2.0f, line_thickness / 2.0f });
                line1.setPosition(center);
                line1.setRotation(sf::degrees(45.0f));
                line1.setFillColor(color);
                sf::Angle angle;

                // Configure the second diagonal line
                line2.setSize({ line_length, line_thickness });
                line2.setOrigin({ line_length / 2.0f, line_thickness / 2.0f });
                line2.setPosition(center);
                line2.setRotation(sf::degrees(-45.0f));
                line2.setFillColor(color);

                texture.draw(line1);
                texture.draw(line2);
        } else if (stone.variant == 1) { // draw circle (O)
                constexpr float radius
                    = (max_dimension * stone::size_factor - line_thickness)
                      / 2.0f;
                sf::CircleShape circle(radius);
                circle.setOrigin({ radius - line_thickness / 2.f,
                                   radius - line_thickness / 2.f }); // center
                auto center = static_cast<float>(max_dimension) / 2;
                circle.setPosition({ center, center });
                circle.setFillColor(sf::Color::Transparent);
                circle.setOutlineColor(sf::Color::Blue);
                circle.setOutlineThickness(line_thickness);
                texture.draw(circle);

        } else {
                assert("Unrecognized stone variant");
                std::unreachable();
        }

        texture.display();
        return texture.getTexture();
};

class gui::implementation {
private:
        std::shared_ptr<mnkg::model::mnk::game> &game_;
        const struct settings                    settings_;
        sf::RenderWindow                         window_;
        board                                    board_;
        point<int, 2>                            selected_cell_;
        struct textures {
                sf::Texture                board;
                std::array<sf::Texture, 2> stone; // two variations
        } textures_;
        struct {
                sf::RenderTexture game, overlay;
        } renders_;

public:
        implementation(auto &game, auto settings) :
                game_(game), settings_(settings),
                board_{ point<uint, 2>(game_->board().get_size()) }
        {
                auto viewport_size = board_.viewport_size();
                window_.create(sf::VideoMode(viewport_size),
                               settings.title,
                               sf::Style::Titlebar | sf::Style::Close);
                renders_ = { sf::RenderTexture(viewport_size),
                             sf::RenderTexture(viewport_size) };

                // HACK: invert y-axis display; bugfix
                {
                        auto view = window_.getView();
                        auto size = sf::Vector2f(window_.getSize());
                        size.y *= -1.f;
                        view.setSize(size);
                        window_.setView(view);
                }

                textures_.board = texture(board_, settings.style);

                for (uint i = 0; i < model::mnk::game::player_count(); ++i)
                        textures_.stone[i] = texture(stone(i), settings.style);

                assert(valid_(textures_));

                render_background_();
        }

        void
        do_run()
        {
                const auto on_close
                    = [&](const sf::Event::Closed &) { window_.close(); };

                const auto on_move = [&](const sf::Event::MouseMoved &event) {
                        auto position = point<int, 2>(event.position);
                        auto coord    = board_.map_view_to_grid(position);
                        if (coord != selected_cell_) {
                                selected_cell_ = coord;
                                clear_phantom_stones_();
                                if (game_->is_playable(selected_cell_))
                                        draw_phantom_stone_(
                                            game_->current_player(),
                                            selected_cell_);
                                redraw_window_();
                        };
                };

                const auto on_left = [&](const sf::Event::MouseLeft &event) {
                        clear_phantom_stones_();
                        redraw_window_();
                };

                const auto on_entered =
                    [&](const sf::Event::MouseEntered &event) {
                            if (game_->is_playable(selected_cell_))
                                    draw_phantom_stone_(game_->current_player(),
                                                        selected_cell_);
                            redraw_window_();
                    };

                const auto on_click
                    = [&](const sf::Event::MouseButtonPressed &event) {
                              if (event.button == sf::Mouse::Button::Left
                                  && game_->is_playable(selected_cell_)) {
                                      clear_phantom_stones_();
                                      draw_stone_(game_->current_player(),
                                                  selected_cell_);
                                      game_->play(selected_cell_);
                                      redraw_window_();
                              }
                      };

                while (window_.isOpen())
                        window_.handleEvents(
                            on_close, on_move, on_left, on_entered, on_click);
        }

private:
        bool
        valid_(const struct textures &textures) const
        {
                bool valid_board = [&]() {
                        const auto &size     = textures.board.getSize();
                        auto        expected = board_.viewport_size();
                        return size == static_cast<point<uint, 2> >(expected);
                }();

                bool valid_stones = [&]() {
                        for (const auto &stone : textures.stone) {
                                const auto &size     = stone.getSize();
                                const auto &expected = cell_viewport_size;
                                if (size != expected)
                                        return false;
                        }
                        return true;
                }();

                return valid_board && valid_stones;
        }

        void
        select_cell_(auto coords)
        {
                selected_cell_ = coords;
                clear_phantom_stones_();
                if (game_->is_playable(coords))
                        draw_phantom_stone_(game_->current_player(),
                                            selected_cell_);
                redraw_window_();
        }

        void
        render_background_()
        {
                renders_.game.draw(sf::Sprite(textures_.board));
        };

        void
        draw_stone_(unsigned int texture_index, point<int, 2> cell_coords)
        {
                sf::Sprite sprite(textures_.stone[texture_index]);
                auto       position = board_.map_grid_to_view(cell_coords);
                sprite.setPosition(position);
                renders_.game.draw(sprite);
        }

        void
        draw_phantom_stone_(unsigned int  texture_index,
                            point<int, 2> cell_coords)
        {
                sf::Sprite     sprite(textures_.stone[texture_index]);
                constexpr auto opacity_factor = 0.3f;
                sf::Color      color;
                color.a *= opacity_factor; // alpha channel
                sprite.setColor(color);
                auto position = board_.map_grid_to_view(cell_coords);
                sprite.setPosition(position);
                renders_.overlay.draw(sprite);
        }

        void
        clear_phantom_stones_()
        {
                renders_.overlay.clear(sf::Color::Transparent);
        }

        void
        redraw_window_()
        {
                window_.draw(sf::Sprite(renders_.game.getTexture()));
                window_.draw(sf::Sprite(renders_.overlay.getTexture()));
                window_.display();
        }
};

gui::gui(std::shared_ptr<mnkg::model::mnk::game> &game, settings settings) :
        pimpl_(std::make_unique<implementation>(game, settings))
{
}

gui::~gui() = default;

void
gui::run()
{
        pimpl_->do_run();
}

} // namespace mnkg::view
