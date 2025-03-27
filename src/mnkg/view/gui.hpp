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
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Mouse.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <memory>

// NOTE: For now, there is no control layer to mediate view-model communication.
//       It wasn't needed, and direct use of the model provides useful types.
//       An MVC design would facilitate de-monolitization of the codebase, tho.
#include "model/mnk/game.hpp"

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
        constexpr static sf::Vector2u cell_size = { 64, 64 }; // unit: pixels
        const sf::Vector2u            size;                   // unit: cells
};

struct stone {
        constexpr static float size_factor = 0.8; // relative to cell
        uint                   variant;           // simple index
        // Equal stones look the same
};

enum class style {
        tic_tac_toe,
        connect_four,
        go,
};

template <style Style, typename Renderable>
sf::Texture
texture(const Renderable &)
{
        assert(not "Texture implementation");
}

template <>
sf::Texture
texture<style::tic_tac_toe, board>(const board &board)
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
texture<style::connect_four, stone>(const stone &stone)
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
                shape.setFillColor(sf::Color::Red);
                break;
        case 1:
                shape.setFillColor(sf::Color::Blue);
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

class gui {
public:
        struct settings {
                std::string title = "MNK Game";
        };

private:
        std::shared_ptr<mnkg::model::mnk::game> &game_;
        const struct settings                    settings_;
        sf::RenderWindow                         window_;
        mnkg::model::mnk::board::position        selected_;
        struct textures {
                sf::Texture                board;
                std::array<sf::Texture, 2> stone; // two variations
        } textures_;
        constexpr static uint cell_size_ = 64; // side length in pixels
        struct {
                sf::RenderTexture game, overlay;
        } renders_;

public:
        gui(auto &game, auto settings) : game_(game), settings_(settings)
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

                textures_.board = texture<style::tic_tac_toe>(
                    board{ { grid_size[0], grid_size[1] } });

                for (uint i = 0; i < model::mnk::game::player_count(); ++i) {
                        textures_.stone[i]
                            = texture<style::connect_four>(stone(i));
                }

                render_background_();
        }

        void
        run()
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

} // namespace view
} // namespace mnkg
