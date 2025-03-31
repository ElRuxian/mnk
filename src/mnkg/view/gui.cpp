#include "gui.hpp"
#include "varia/point.hpp"
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
#include <utility>

namespace mnkg::view {

static constexpr sf::Vector2u cell_viewport_size = { 32u, 32u };

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
        static constexpr uint  variant_count = 2;
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
        // HACK: Bad switch. Must be refactored later.
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
        line.setFillColor(sf::Color::Black);

        line.setSize(sf::Vector2f(line_thickness, texture.getSize().y));
        for (uint i = 1; i < board.grid_size[0]; ++i) {
                line.setPosition(sf::Vector2f(i * cell_viewport_size.y, 0));
                texture.draw(line);
        }

        line.setSize(sf::Vector2f(texture.getSize().x, line_thickness));
        for (uint i = 1; i < board.grid_size[1]; ++i) {
                line.setPosition(sf::Vector2f(0, i * cell_viewport_size.y));
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
        constexpr auto max_dimension
            = std::max(cell_viewport_size.x, cell_viewport_size.y);
        constexpr auto  radius = max_dimension * stone::size_factor / 2;
        sf::CircleShape shape(radius);
        shape.setFillColor(sf::Color::White);
        for (auto x = 0; x < board.grid_size[0]; ++x)
                for (auto y = 0; y < board.grid_size[1]; ++y) {
                        auto position = board.map_grid_to_view(point{ x, y });
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
        auto      texture    = sf::RenderTexture(board.viewport_size());
        sf::Color wood_color = { 222, 184, 135 };
        texture.clear(wood_color);
        constexpr auto max_dimension
            = std::max(cell_viewport_size.x, cell_viewport_size.y);
        sf::RectangleShape line;
        line.setFillColor(sf::Color::Black);
        constexpr float line_thickness = max_dimension * 0.1f;
        auto            grid_size      = texture.getSize() - cell_viewport_size;
        for (uint x = 0; x < board.grid_size[0]; ++x) {
                line.setSize(sf::Vector2f(line_thickness, grid_size.y));
                auto position = sf::Vector2f(x * cell_viewport_size.x, 0);
                position += sf::Vector2f(cell_viewport_size) / 2.f;
                line.setPosition(position);
                texture.draw(line);
        }
        for (uint y = 0; y < board.grid_size[1]; ++y) {
                line.setSize(sf::Vector2f(grid_size.x, line_thickness));
                auto position = sf::Vector2f(0, y * cell_viewport_size.y);
                position += sf::Vector2f(cell_viewport_size) / 2.f;
                line.setPosition(position);
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
        shape.setOrigin({ radius, radius });
        auto center = sf::Vector2f(cell_viewport_size) / 2.f;
        shape.setPosition(center);
        switch (stone.variant) {
        case 0:
                shape.setFillColor(sf::Color::Red);
                break;
        case 1:
                shape.setFillColor(sf::Color::Yellow);
                break;
        default:
                assert(!"unrecognized stone variant");
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
        auto center = sf::Vector2f(cell_viewport_size) / 2.f;
        shape.setPosition(center);
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
        auto            center         = sf::Vector2f(cell_viewport_size) / 2.f;

        texture.clear(sf::Color::Transparent);

        if (stone.variant == 0) { // draw cross (X) sf::RectangleShape line1;
                sf::RectangleShape line;

                float line_length = max_dimension * 0.8f;
                auto  size        = sf::Vector2f(line_length, line_thickness);
                auto  angle       = sf::degrees(45.0f);
                auto  color       = sf::Color::Red;

                line.setSize(size);
                line.setOrigin(size / 2.f);
                line.setPosition(center);
                line.setFillColor(color);

                line.setRotation(angle);
                texture.draw(line);

                line.setRotation(-angle);
                texture.draw(line);

        } else if (stone.variant == 1) { // draw circle (O)
                constexpr auto  radius = max_dimension * stone::size_factor / 2;
                sf::CircleShape shape(radius);
                shape.setOrigin({ radius, radius }); // center
                shape.setPosition(center);
                shape.setFillColor(sf::Color::Transparent);
                shape.setOutlineColor(sf::Color::Blue);
                shape.setOutlineThickness(-line_thickness);
                texture.draw(shape);

        } else {
                assert("Unrecognized stone variant");
                std::unreachable();
        }

        texture.display();
        return texture.getTexture();
};

class gui::implementation {
private:
        sf::RenderWindow         window_;
        callbacks                callbacks_;
        board                    board_;
        point<int, 2>            hovered_cell_;
        std::set<point<int, 2> > selectable_cells_;
        uint                     stone_skin_index_;
        struct textures {
                sf::Texture                                   board;
                std::array<sf::Texture, stone::variant_count> stone;
        } textures_;
        struct {
                sf::RenderTexture game, overlay;
        } renders_;

public:
        implementation(const struct settings &settings) :
                callbacks_(settings.callbacks), board_{ settings.board_size }
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
                for (uint i = 0; i < stone::variant_count; ++i)
                        textures_.stone[i] = texture(stone(i), settings.style);
                assert(valid_(textures_));

                render_background_();
        }

        void
        draw_stone(unsigned int texture_index, point<int, 2> cell_coords)
        {
                assert(texture_index < textures_.stone.size());
                sf::Sprite sprite(textures_.stone[texture_index]);
                sprite.setPosition(board_.map_grid_to_view(cell_coords));
                renders_.game.draw(sprite);
        }

        void
        draw_stone(point<int, 2> cell_coords)
        {
                draw_stone(stone_skin_index_, cell_coords);
        }

        inline void
        set_selectable_cells(std::set<point<int, 2> > cells)
        {
                selectable_cells_ = std::move(cells);
        }

        inline void
        set_stone_skin(uint index)
        {
                stone_skin_index_ = index;
        }

        void
        run()
        {
                const auto on_close
                    = [&](const sf::Event::Closed &) { window_.close(); };

                const auto on_move = [&](const sf::Event::MouseMoved &event) {
                        auto position = point<int, 2>(event.position);
                        auto coord    = board_.map_view_to_grid(position);
                        if (coord != hovered_cell_) {
                                hovered_cell_ = coord;
                                clear_phantom_stones_();
                                if (selectable_(hovered_cell_))
                                        draw_phantom_stone_(hovered_cell_);
                                redraw_window_();
                        };
                };

                const auto on_left = [&](const sf::Event::MouseLeft &event) {
                        clear_phantom_stones_();
                        redraw_window_();
                };

                const auto on_entered
                    = [&](const sf::Event::MouseEntered &event) {
                              if (selectable_(hovered_cell_))
                                      draw_phantom_stone_(hovered_cell_);
                              redraw_window_();
                      };

                const auto on_click =
                    [&](const sf::Event::MouseButtonPressed &event) {
                            if (event.button == sf::Mouse::Button::Left
                                && selectable_(hovered_cell_)) {
                                    clear_phantom_stones_();
                                    redraw_window_();
                                    callbacks_.on_cell_selected(hovered_cell_);
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
                        const auto &expected = board_.viewport_size();
                        return size == expected;
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

        bool
        selectable_(auto coords) const
        {
                return selectable_cells_.contains(coords);
        }

        void
        render_background_()
        {
                renders_.game.draw(sf::Sprite(textures_.board));
        };

        void
        draw_phantom_stone_(unsigned int  texture_index,
                            point<int, 2> cell_coords)
        {
                assert(texture_index < textures_.stone.size());
                sf::Sprite sprite(textures_.stone[texture_index]);
                sf::Color  color;
                color.a *= 0.3f; // 30% opacity
                sprite.setColor(color);
                sprite.setPosition(board_.map_grid_to_view(cell_coords));
                renders_.overlay.draw(sprite);
        }

        void
        draw_phantom_stone_(point<int, 2> cell_coords)
        {
                draw_phantom_stone_(stone_skin_index_, cell_coords);
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

gui::gui(const settings &settings) :
        pimpl_(std::make_unique<implementation>(settings))
{
}

gui::~gui() = default;

inline void
gui::run()
{
        pimpl_->run();
}

inline void
gui::set_selectable_cells(const std::set<point<int, 2> > &coords)
{
        pimpl_->set_selectable_cells(coords);
}

inline void
gui::set_stone_skin(uint index)
{
        pimpl_->set_stone_skin(index);
}

inline void
gui::draw_stone(point<int, 2> cell_coords)
{
        pimpl_->draw_stone(cell_coords);
}

} // namespace mnkg::view
