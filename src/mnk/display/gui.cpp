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
#include <SFML/System/Vector2.hpp>
#include <cassert>
#include <functional>
#include <memory>
#include <print>

#include "varia/grid.hpp"

namespace mnkg::mnk::display {

enum class effect { obscured, highlighted };

using board = struct {
        const game::board           &model;
        grid<std::optional<effect> > overlay;
};

class renderer {
private:
        virtual void
        background_(sf::RenderTarget &, const board &) const
            = 0;

        virtual void
        stone_(sf::RenderTarget &, game::player_indice) const
            = 0;

        virtual void
        effect_(sf::RenderTarget &, effect) const
            = 0;

        virtual float // 0 to 1, relative to cell size
        cell_pad_factor_() const
            = 0;

        auto
        cells_(sf::RenderTarget &target, const board &board) const
        {
                using namespace std::views;
                auto pad_factor = cell_pad_factor_();
                assert(pad_factor >= 0 && pad_factor <= 1);
                auto viewport_size = static_cast<sf::Vector2i>(
                    target.getView().getSize()); // this cast should be safe
                auto board_size = board.model.get_size();
                auto cell_size
                    = point<int, 2>{ viewport_size.x / board_size[1],
                                     viewport_size.y / board_size[0] };
                return coords(board.model) | transform([&](auto coord) {
                               auto          cell = point<int, 2>(zip_transform(
                                   std::multiplies{}, coord, cell_size));
                               auto          pad  = cell * pad_factor;
                               sf::FloatRect rect(cell[0] + pad[0],
                                                  cell[1] + pad[1],
                                                  cell_size[0] - 2 * pad[0],
                                                  cell_size[1] - 2 * pad[1]);
                               return std::pair{ coord, sf::View(rect) };
                       });
        }

public:
        void
        render(sf::RenderTarget &target, const board &board)
        {
                using namespace std::views;
                auto original_view = target.getView();
                background_(target, board);
                for (auto [coord, view] : cells_(target, board)) {
                        target.setView(view);
                        if (board.model[coord].has_value())
                                stone_(target, board.model[coord].value());
                        if (board.overlay[coord].has_value())
                                effect_(target, board.overlay[coord].value());
                }
                target.setView(original_view);
        }

        virtual ~renderer() = default;

        template <gui::skin Skin>
        class impl;
};

template <>
class renderer::impl<gui::skin::drop_grid> : public renderer {

        virtual void
        background_(sf::RenderTarget &target, const board &board) const override
        {
                target.clear(sf::Color::Blue);
                auto original_view = target.getView();
                for (auto [_, view] : cells_(target, board)) {
                        target.setView(view);
                        circle_(target, sf::Color::Black);
                }
                target.setView(original_view);
        }

        void
        circle_(sf::RenderTarget &target, sf::Color color) const
        {
                sf::CircleShape shape;
                const auto     &view = target.getView();
                shape.setRadius(view.getSize().x / 2);
                shape.setFillColor(color);
                shape.setPosition(view.getCenter() - view.getSize() / 2.f);
                target.draw(shape);
        }

        virtual void
        stone_(sf::RenderTarget   &target,
               game::player_indice player) const override
        {
                switch (player) {
                case 0:
                        circle_(target, sf::Color::Red);
                        break;
                case 1:
                        circle_(target, sf::Color::Yellow);
                        break;
                default:
                        assert(!"limited implementation");
                }
        }

        virtual void
        effect_(sf::RenderTarget &, effect) const override
        {
                // TODO: implement
        }

        virtual float // 0 to 1, relative to cell size
        cell_pad_factor_() const override
        {
                return 0.2;
        }
};

// template <>
// class renderer::impl<gui::skin::paper_grid> : public renderer {
// private:
// public:
//         virtual const sf::Drawable &
//         render(const game::player_indice) override
//         {
//                 using enum cell::effects::flag;
//
//                 if (cell.has_value())
//                         switch (cell.value()) {
//                         case 0:
//                                 render.draw(x);
//                                 break;
//                         case 1:
//                                 render.draw(o);
//                                 break;
//                         default:
//                                 assert(!"limited implementation");
//                         }
//
//                 render.display();
//                 return render.getTexture();
//         }
//
//         static std::array<std::reference_wrapper<const sf::Drawable>, 2>
//         stones; static const sf::VertexArray x; static const sf::CircleShape
//         o;
// };
//
// const sf::VertexArray cell::texture<gui::skin::paper_grid>::x = []() {
//         auto offset = cell::side_length / 10;
//
//         sf::VertexArray vertexes(sf::Lines, 4);
//
//         vertexes[0].position = sf::Vector2f(offset, offset);
//         vertexes[1].position
//             = sf::Vector2f(side_length - offset, side_length - offset);
//         vertexes[2].position = sf::Vector2f(offset, side_length - offset);
//         vertexes[3].position = sf::Vector2f(side_length - offset, offset);
//
//         for (int i = 0; i < vertexes.getVertexCount(); i++)
//                 vertexes[i].color = sf::Color::Black;
//
//         return vertexes;
// }();
//
// const sf::CircleShape cell::texture<gui::skin::paper_grid>::o = []() {
//         auto            offset = side_length / 10;
//         sf::CircleShape circle(side_length / 2 - offset);
//         circle.setFillColor(sf::Color::Transparent);
//         circle.setOutlineThickness(offset);
//         circle.setOutlineColor(sf::Color::Black);
//         circle.setPosition(offset, offset);
//         return circle;
// }();

struct gui::impl {
        game                               &game;
        settings                            settings;
        sf::RenderWindow                    window;
        board                               board;
        std::unique_ptr<renderer>           renderer;
        std::optional<mnk::board::position> hovering;
        constexpr static auto               sidelength = 100;

        impl(class game &game, struct settings settings) :
                game(game), settings(settings), board{ game.get_board(), {} }
        {
                auto size = game.get_board().get_size() * sidelength;
                window.create(sf::VideoMode(size[1], size[0]),
                              settings.title,
                              sf::Style::Titlebar | sf::Style::Close);
                constexpr auto skin = gui::skin::drop_grid; // FIXME: temporary
                renderer            = std::make_unique<renderer::impl<skin> >();
                run();
        }

        void
        draw(auto &window)
        {
                renderer->render(window, board);
                window.display();
        }

        void
        run()
        {
                while (window.isOpen()) {
                        sf::Event event;
                        while (window.pollEvent(event)) {
                                if (event.type == sf::Event::Closed) {
                                        window.close();
                                        break;
                                }
                                if (event.type == sf::Event::MouseMoved) {
                                        auto coord = point<int, 2>(
                                            event.mouseMove.x / sidelength,
                                            event.mouseMove.y / sidelength);
                                        if (within(board.model, coord)) {
                                                board.overlay[hovering.value()]
                                                    = std::nullopt;
                                                hovering = coord;
                                                board.overlay[coord]
                                                    = effect::highlighted;
                                        } else
                                                hovering.reset();
                                }
                                if (event.type == sf::Event::MouseButtonPressed
                                    && event.mouseButton.button
                                           == sf::Mouse::Left) {
                                        game.play(game.get_player(),
                                                  hovering.value());
                                }
                        }
                        draw(window);
                }
        }
};

gui::gui(game &game, settings settings) :
        pimpl(std::make_unique<impl>(game, settings))
{
}

void
gui::run()
{
        pimpl->run();
}

} // namespace mnkg::mnk::display

int
main()
{
        using namespace mnkg;
        auto game = mnk::game();
        auto gui  = mnk::display::gui(
            game, { mnk::display::gui::skin::paper_grid, "Tic-Tac-Toe" });
        gui.run();
}

/*
 * Usar view con coordenadas 0 a 1
 * Se requiere: fondo, piedras, seleccionable y no-seleccionable
 * Pedir al controller un grid que combine piedra + seleccionable
 * Renderizar fondo directo al target. Devolver sprite del resto para efectos
 * Iterar cells, usar stone scale
 * Usar opt<coord> seleccion
 * Referencia al controlador, no al modelo
 * Se requeire mecanismo asincrónico para avisar cuando se pueda jugar
 *
 *
 * */
