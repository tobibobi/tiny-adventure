/**
 * @file advengine.h
 * @brief Public API of advengine, a tiny data-driven text adventure engine.
 *
 * The engine knows nothing about any particular game. Rooms, objects, flags
 * and puzzle rules are loaded from a plain-text world file (see
 * docs/world-format.md). Output is plain ASCII on stdout, word-wrapped for an
 * 80-column terminal.
 *
 * Typical use:
 * @code
 *   if (adv_load("world.txt") != 0) return 1;
 *   adv_start();
 *   while (!adv_is_over()) {
 *       // print a prompt, read a line ...
 *       adv_turn(line);
 *   }
 * @endcode
 */
#ifndef ADVENGINE_ADVENGINE_H
#define ADVENGINE_ADVENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Load a world file. Call once, before adv_start().
 * @param path Path to the world file.
 * @return 0 on success, -1 on error (a message is printed to stderr).
 */
int adv_load(const char *path);

/** @brief Print the title, the intro and the starting room. */
void adv_start(void);

/**
 * @brief Execute one line of player input.
 * @param line The command as typed, without the trailing newline.
 */
void adv_turn(const char *line);

/** @return Non-zero once the game has ended (won, died or quit). */
int adv_is_over(void);

#ifdef __cplusplus
}
#endif

#endif /* ADVENGINE_ADVENGINE_H */
