# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

`tinyadv`: a tiny, data-driven text adventure engine in plain C99 (no extensions). The engine has no
game-specific code. Rooms, objects, flags and puzzle rules live in a plain-text world file. Output
is plain ASCII, word-wrapped for 80-column terminals.

## Commands

All builds go through CMake into `build/`, driven by `GNUmakefile` (`make help` lists everything):

```sh
make                      # configure (first time) + build
make test                 # build + ctest --output-on-failure
make run                  # play the sample world from build/bin
make format / format-check
make tidy                 # clang-tidy using build/compile_commands.json
make configure            # re-run CMake (needed after changing BUILD_TYPE/CMAKE_ARGS)
make test BUILD_TYPE=Debug CMAKE_ARGS="-DTINYADV_SANITIZE=ON -DTINYADV_WARNINGS_AS_ERRORS=ON"
```

Run a single test: `cd build && ctest -R tinyadv.grue --output-on-failure`.

Variables are applied only at first configure, because `build/CMakeCache.txt` is the make
prerequisite. If you change them later, run `make configure` or `make rebuild`.

## Architecture

- `sources/modules/advengine/`: static library with the whole engine. The public API in
  `headers/advengine/advengine.h` has four functions: `adv_load`, `adv_start`, `adv_turn`,
  `adv_is_over`.
- `sources/apps/tinyadv/`: thin stdin/stdout front end (`src/main.c`), the sample world
  (`data/world.txt`, copied next to the binary after each build) and scripted tests (`test/`).

Engine internals (`advengine/src/`): `world.c` (state + world file loader), `parser.c` (input to
`Cmd`), `game.c` (executes a turn), `output.c` (word wrap), `internal.h` (shared types).

Key design constraints:
- **No allocation during play.** All state lives in one global `struct adv_state`, accessed through
  the macro `G`. Fixed table sizes (`MAXROOM`, `MAXOBJ`, `MAXRULE`, ...) are set at the top of
  `internal.h`. If you change a limit, also update the "Limits" section of `docs/world-format.md`.
- **Two-pass loader.** Pass 1 collects room/object ids, pass 2 resolves references, so the order of
  definitions in a world file doesn't matter. Load errors report file:line, and `adv_load` returns -1.
- **Turn order:** parse → `adv_fallback_parse()` → reject unknown verb/invisible noun (costs no
  turn) → movement → first matching world-file `RULE` (rules override built-ins) → built-in verb →
  end of turn (move counter, darkness: 3 turns in the dark is fatal).
- `adv_fallback_parse()` in `parser.c` is intentionally empty. It is the hook for a future
  embedding/LLM parser. The model may only translate input into existing commands, and game logic
  stays in the engine (see `docs/architecture.md`).

## Tests

Tests are end-to-end only. `test/run_script.cmake` pipes a command file into `tinyadv` with the
sample world and checks that an expected string appears in the output. To add a test, write a
command file in `sources/apps/tinyadv/test/` and register an `add_test` in
`sources/apps/tinyadv/CMakeLists.txt`. If you edit `data/world.txt`, the existing walkthrough
tests can break.

## Docs

`docs/world-format.md` is the reference for the world file language (keywords, rules, built-in
verbs). Update it together with any parser or loader change. `docs/architecture.md` describes the
turn pipeline.

## Style

clang-format (LLVM base, 4-space indent, 100 columns, Linux braces) and clang-tidy configs are in
the repo root. The compiler runs with `-Wall -Wextra -pedantic -Wshadow -Wstrict-prototypes
-Wmissing-prototypes`, so non-static functions need prototypes and empty parameter lists must be
written as `(void)`.
