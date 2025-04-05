## m,n,k-game

This project consists of a game where two players take turns placing a stone on an m-by-n grid. The winner is the first player to align k stones of their own —horizontally, vertically, or diagonally.

An ImGUI interface is provided to configure parameters such as:

- board size (m and n);
- winning line length (k);
- "overline" validity: whether lines longer than k count as a win;
- the visual style of the board; and
- "gravity": if enabled, stones are placed at the lowest available position in the column.

Some presets are available for quick configuration.

Additionally, users can choose whether each player is controlled by a human or an AI.
The AI players use a Monte Carlo Tree Search (MCTS) algorithm, enhanced with leaf-level parallelism via an ASIO thread pool, and node memory pooling through a custom allocator.

The game itself is rendered with Simple and Fast Multimedia Library (SFML). Human players can click on the board to place a stone.

### Building

A CMake build system is provided.

ImGui, ASIO, and SFML are automatically fetched, built, and statically linked.

On Linux, essential system libraries such as OpenGL and the C++ standard library remain dynamically linked. The corresponding shared objects must be available for the executable to run.

On Windows, all dependencies are statically linked to produce a fully standalone executable.

Check [releases](https://github.com/ElRuxian/mnk/releases) for pre-built binaries.
