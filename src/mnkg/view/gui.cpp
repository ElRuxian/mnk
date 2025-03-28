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

namespace mnkg {

// sf::Vector2<T> to point<T, 2> conversion
// TODO: find a better way, in a better place?
template <typename T>
point<T, 2>
make_point(const sf::Vector2<T> &vec)
{
        return point(vec.x, vec.y);
}

namespace view {

struct board {
        constexpr static sf::Vector2u cell_size = { 32, 32 }; // unit: pixels
        const sf::Vector2u            size;                   // unit: cells
};

struct stone {
        constexpr static float size_factor = 0.8; // relative to cell
        uint                   variant;           // simple index
        // Equal stones look the same
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

        auto size = board.size.componentWiseMul(board.cell_size);

        auto texture = sf::RenderTexture(size);

        texture.clear(sf::Color::White);

        sf::RectangleShape line;
        constexpr auto     line_thickness = 2; // arbitrary; looks ok

        for (uint i = 1; i < board.size.x; ++i) {
                auto size = sf::Vector2f(line_thickness, texture.getSize().y);
                line.setSize(size);
                auto position = sf::Vector2<float>(i * board.cell_size.x, 0);
                line.setPosition(position);
                line.setFillColor(sf::Color::Black);
                texture.draw(line);
        }

        for (uint i = 1; i < board.size.y; ++i) {
                auto size = sf::Vector2f(texture.getSize().x, line_thickness);
                line.setSize(size);
                auto position = sf::Vector2<float>(0, i * board.cell_size.y);
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

        auto size = board.size.componentWiseMul(board.cell_size);

        auto texture = sf::RenderTexture(size);

        texture.clear(sf::Color::Blue);

        // TODO: refactor
        //       (this block of code it repeated in texture<tic_tac_toe, stone>)
        constexpr auto max_dimension
            = std::max(board::cell_size.x, board::cell_size.y);
        constexpr auto  radius = max_dimension * stone::size_factor / 2;
        sf::CircleShape shape(radius);

        shape.setFillColor(sf::Color::White);

        for (uint x = 0; x < board.size.x; ++x)
                for (uint y = 0; y < board.size.y; ++y) {
                        auto position = sf::Vector2f(x * board.cell_size.x,
                                                     y * board.cell_size.y);
                        position
                            += sf::Vector2f(board.cell_size.x / 2.f - radius,
                                            board.cell_size.y / 2.f - radius);
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

        auto size = board.size.componentWiseMul(board.cell_size);

        auto texture = sf::RenderTexture(size);

        sf::Color wood_color = { 222, 184, 135 };
        texture.clear(wood_color);

        constexpr auto max_dimension
            = std::max(board::cell_size.x, board::cell_size.y);

        sf::RectangleShape line;
        constexpr float    line_thickness = max_dimension * 0.1f;

        for (uint x = 0; x < board.size.x; ++x) {
                auto size = sf::Vector2f(
                    line_thickness, texture.getSize().y - board.cell_size.y);
                line.setSize(size);
                auto position = sf::Vector2<float>(x * board.cell_size.x, 0);
                position
                    += { board.cell_size.x / 2.f, board.cell_size.y / 2.f };
                line.setPosition(position);
                line.setFillColor(sf::Color::Black);
                texture.draw(line);
        }

        for (uint y = 0; y < board.size.y; ++y) {
                auto size = sf::Vector2f(
                    texture.getSize().x - board.cell_size.x, line_thickness);
                line.setSize(size);
                auto position = sf::Vector2<float>(0, y * board.cell_size.y);
                position
                    += { board.cell_size.x / 2.f, board.cell_size.y / 2.f };
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
            = std::max(board::cell_size.x, board::cell_size.y);
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
            = std::max(board::cell_size.x, board::cell_size.y);
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
            = std::max(board::cell_size.x, board::cell_size.y);
        constexpr float line_thickness = max_dimension * 0.1f;

        texture.clear(sf::Color::Transparent);

        if (stone.variant == 0) { // draw cross (X) sf::RectangleShape line1;
                sf::RectangleShape line1, line2;

                sf::Vector2f center
                    = { board::cell_size.x / 2.0f, board::cell_size.y / 2.0f };
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
        mnkg::model::mnk::board::position        selected_;
        struct textures {
                sf::Texture                board;
                std::array<sf::Texture, 2> stone; // two variations
        } textures_;
        constexpr static uint cell_size_ = 32; // side length in pixels
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
                    = sf::Vector2u{ grid_size[1], grid_size[0] } * cell_size_;
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
                        auto coord = make_point(event.position) / cell_size_;
                        if (coord != selected_) {
                                selected_ = coord;
                                clear_phantom_stones_();
                                if (game_->is_playable(coord))
                                        draw_phantom_stone_(
                                            game_->current_player(), selected_);
                                redraw_window_();
                        };
                };

                const auto on_left = [&](const sf::Event::MouseLeft &event) {
                        clear_phantom_stones_();
                        redraw_window_();
                };

                const auto on_entered
                    = [&](const sf::Event::MouseEntered &event) {
                              if (game_->is_playable(selected_))
                                      draw_phantom_stone_(
                                          game_->current_player(), selected_);
                              redraw_window_();
                      };

                const auto on_click
                    = [&](const sf::Event::MouseButtonPressed &event) {
                              if (event.button == sf::Mouse::Button::Left
                                  && game_->is_playable(selected_)) {
                                      clear_phantom_stones_();
                                      draw_stone_(game_->current_player(),
                                                  selected_);
                                      game_->play(selected_);
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
                        const auto &size = make_point(textures.board.getSize());
                        auto expected = game_->board().get_size() * cell_size_;
                        return size == static_cast<point<uint, 2> >(expected);
                }();

                bool valid_stones = [&]() {
                        for (const auto &stone : textures.stone) {
                                const auto  &size = stone.getSize();
                                sf::Vector2u expected(cell_size_, cell_size_);
                                if (size != expected)
                                        return false;
                        }
                        return true;
                }();

                return valid_board && valid_stones;
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
                auto       position = point<float, 2>(cell_coords) * cell_size_;
                sprite.setPosition({ position[0], position[1] });
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
                auto position = point<float, 2>(cell_coords) * cell_size_;
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

} // namespace view
} // namespace mnkg
