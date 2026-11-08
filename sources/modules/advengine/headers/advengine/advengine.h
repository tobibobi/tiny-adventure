/**
 * @file advengine.h
 * @brief Public API of advengine, a tiny data-driven text adventure engine.
 *
 * The engine knows nothing about any particular game. Rooms, objects, flags
 * and puzzle rules are loaded from a plain-text world file (see
 * docs/world-format.md). Output is plain ASCII, word-wrapped for an
 * 80-column terminal.
 *
 * Typical use:
 * @code
 *   adv::Game game(std::cout);
 *   try {
 *       game.load("world.txt");
 *   } catch (const adv::LoadError &e) {
 *       std::cerr << e.what() << '\n';
 *       return 1;
 *   }
 *   game.start();
 *   while (!game.is_over()) {
 *       // print a prompt, read a line ...
 *       game.turn(line);
 *   }
 * @endcode
 */
#ifndef ADVENGINE_ADVENGINE_H
#define ADVENGINE_ADVENGINE_H

#include <filesystem>
#include <iosfwd>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace adv
{

/** @brief Thrown by Game::load() when a world file can't be read or is invalid. */
class LoadError : public std::runtime_error
{
  public:
    using std::runtime_error::runtime_error;
};

/** @brief One game: a loaded world plus the player's progress in it. */
class Game
{
  public:
    /** @param out Stream that receives all game text. Must outlive the game. */
    explicit Game(std::ostream &out);
    ~Game();
    Game(const Game &) = delete;
    Game &operator=(const Game &) = delete;
    Game(Game &&) noexcept;
    Game &operator=(Game &&) noexcept;

    /**
     * @brief Load a world file, replacing any previously loaded world.
     * @throws LoadError with "file:line: message" if the file is invalid.
     *         The game is left unchanged in that case.
     */
    void load(const std::filesystem::path &path);

    /** @brief Print the title, the intro and the starting room. */
    void start();

    /**
     * @brief Execute one line of player input.
     * @param line The command as typed, without the trailing newline.
     */
    void turn(std::string_view line);

    /** @return true once the game has ended (won, died or quit). */
    [[nodiscard]] bool is_over() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace adv

#endif // ADVENGINE_ADVENGINE_H
