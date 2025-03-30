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
#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <optional>
#include <utility>

#include "varia/point.hpp"

namespace mnkg::view {

struct board {
        constexpr static point<uint, 2> cell_size = { 32u, 32u }; // pixels
        const point<uint, 2>            size;                     // cells

        std::optional<point<uint, 2> >
        map(sf::Vector2i position) const
        {
                auto x = position.x / cell_size[0];
                auto y = position.y / cell_size[1];
                if (position.x < 0 || position.y < 0)
                        return std::nullopt;
                if (x >= size[0] || y >= size[1])
                        return std::nullopt;
                return point(static_cast<uint>(x), static_cast<uint>(y));
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

        auto size = transform(std::multiplies{}, board.size, board.cell_size);

        auto texture = sf::RenderTexture(size);

        texture.clear(sf::Color::White);

        sf::RectangleShape line;
        constexpr auto     line_thickness = 2; // arbitrary; looks ok

        for (uint i = 1; i < board.size[0]; ++i) {
                auto size = sf::Vector2f(line_thickness, texture.getSize().y);
                line.setSize(size);
                auto position = sf::Vector2<float>(i * board.cell_size[1], 0);
                line.setPosition(position);
                line.setFillColor(sf::Color::Black);
                texture.draw(line);
        }

        for (uint i = 1; i < board.size[1]; ++i) {
                auto size = sf::Vector2f(texture.getSize().x, line_thickness);
                line.setSize(size);
                auto position = sf::Vector2<float>(0, i * board.cell_size[1]);
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

        auto size = transform(std::multiplies{}, board.size, board.cell_size);

        auto texture = sf::RenderTexture(size);

        texture.clear(sf::Color::Blue);

        // TODO: refactor
        //       (this block of code it repeated in texture<tic_tac_toe, stone>)
        constexpr auto max_dimension
            = std::max(board::cell_size[0], board::cell_size[1]);
        constexpr auto  radius = max_dimension * stone::size_factor / 2;
        sf::CircleShape shape(radius);

        shape.setFillColor(sf::Color::White);

        for (uint x = 0; x < board.size[0]; ++x)
                for (uint y = 0; y < board.size[1]; ++y) {
                        auto position = sf::Vector2f(x * board.cell_size[0],
                                                     y * board.cell_size[1]);
                        position
                            += sf::Vector2f(board.cell_size[0] / 2.f - radius,
                                            board.cell_size[1] / 2.f - radius);
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

        auto size = transform(std::multiplies{}, board.size, board.cell_size);

        auto texture = sf::RenderTexture(size);

        sf::Color wood_color = { 222, 184, 135 };
        texture.clear(wood_color);

        constexpr auto max_dimension
            = std::max(board::cell_size[0], board::cell_size[1]);

        sf::RectangleShape line;
        constexpr float    line_thickness = max_dimension * 0.1f;

        for (uint x = 0; x < board.size[0]; ++x) {
                auto size = sf::Vector2f(
                    line_thickness, texture.getSize().y - board.cell_size[1]);
                line.setSize(size);
                auto position = sf::Vector2<float>(x * board.cell_size[0], 0);
                position
                    += { board.cell_size[0] / 2.f, board.cell_size[1] / 2.f };
                line.setPosition(position);
                line.setFillColor(sf::Color::Black);
                texture.draw(line);
        }

        for (uint y = 0; y < board.size[1]; ++y) {
                auto size = sf::Vector2f(
                    texture.getSize().x - board.cell_size[0], line_thickness);
                line.setSize(size);
                auto position = sf::Vector2<float>(0, y * board.cell_size[1]);
                position
                    += { board.cell_size[0] / 2.f, board.cell_size[1] / 2.f };
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
        auto           texture = sf::RenderTexture(board::cell_size);
        constexpr auto max_dimension
            = std::max(board::cell_size[0], board::cell_size[1]);
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
        auto           texture = sf::RenderTexture(board::cell_size);
        constexpr auto max_radius
            = std::max(board::cell_size[0], board::cell_size[1]);
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
        sf::RenderTexture texture(board::cell_size);

        constexpr float max_dimension
            = std::max(board::cell_size[0], board::cell_size[1]);
        constexpr float line_thickness = max_dimension * 0.1f;

        texture.clear(sf::Color::Transparent);

        if (stone.variant == 0) { // draw cross (X) sf::RectangleShape line1;
                sf::RectangleShape line1, line2;

                auto  center      = point<float, 2>(board::cell_size) / 2;
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
                game_(game), settings_(settings)
        {
                const auto grid_size
                    = point<uint, 2>(game_->board().get_size());
                auto view_size
                    = transform(std::multiplies{}, grid_size, board::cell_size);
                window_.create(sf::VideoMode(view_size),
                               settings.title,
                               sf::Style::Titlebar | sf::Style::Close);
                renders_ = { sf::RenderTexture(view_size),
                             sf::RenderTexture(view_size) };

                // HACK: invert y-axis display; bugfix
                {
                        auto view = window_.getView();
                        auto size = sf::Vector2f(window_.getSize());
                        size.y *= -1.f;
                        view.setSize(size);
                        window_.setView(view);
                }

                textures_.board = texture(
                    board{ { grid_size[0], grid_size[1] } }, settings.style);

                for (uint i = 0; i < model::mnk::game::player_count(); ++i) {
                        textures_.stone[i] = texture(stone(i), settings.style);
                }

                render_background_();
        }

        void
        do_run()
        {
                const auto on_close
                    = [&](const sf::Event::Closed &) { window_.close(); };

                const auto on_move = [&](const sf::Event::MouseMoved &event) {
                        auto coord = transform(std::divides{},
                                               point<int, 2>(event.position),
                                               point<int, 2>(board::cell_size));
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
        is_valid_(const struct textures &textures) const
        {
                bool valid_board = [&]() {
                        const auto &size     = textures.board.getSize();
                        auto        expected = transform(
                            std::multiplies{},
                            point<uint, 2>(game_->board().get_size()),
                            board::cell_size);
                        return size == static_cast<point<uint, 2> >(expected);
                }();

                bool valid_stones = [&]() {
                        for (const auto &stone : textures.stone) {
                                const auto &size     = stone.getSize();
                                const auto &expected = board::cell_size;
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
                auto       position = transform(std::multiplies{},
                                          point<float, 2>(cell_coords),
                                          point<float, 2>(board::cell_size));
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
                auto position = transform(std::multiplies{},
                                          point<float, 2>(cell_coords),
                                          point<float, 2>(board::cell_size));
                sprite.setPosition({ position[0], position[1] });
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
