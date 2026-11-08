# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

`tinyadv`: a tiny, data-driven text adventure engine in C++20 (no compiler extensions). The engine has no
game-specific code. Rooms, objects, flags and puzzle rules live in a plain-text world file. Output
is plain ASCII, word-wrapped for 80-column terminals.

**Status: in transition, architecture undecided.** Version 2 is planned as a point-and-click
adventure with tabletop RPG rules. The game mechanics are being settled first in
`design/rules.md` (on the `design` branch); the code shape follows from them. The current engine is a straight C++20 port of
the version 1 text engine (the C99 original lives on the `v1` branch), kept as a checkpoint. Many of
its v1 decisions (text parser, LLM fallback hook, text output, darkness/grue, compass exits,
line-based world format) are expected to change. Don't build new features on it until the v2
architecture is decided.

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
  `headers/advengine/advengine.h` is the class `adv::Game` (`load`, `start`, `turn`, `is_over`);
  internals are in namespace `adv::detail` behind a pimpl.
- `sources/apps/tinyadv/`: thin stdin/stdout front end (`src/main.cpp`), the sample world
  (`data/world.txt`, copied next to the binary after each build) and scripted tests (`test/`).

Engine internals (`advengine/src/`): `world.cpp` (state + world file loader), `parser.cpp` (input to
`Command`), `game.cpp` (`Game` and the turn loop), `output.cpp` (word wrap), `internal.h` (shared
types, including `World`, which holds all state of one game).

Key design points of the current engine:
- **Two-pass loader.** The file is read into memory; pass 1 collects room/object ids, pass 2
  resolves references, so the order of definitions in a world file doesn't matter. Load errors
  throw `adv::LoadError` with "file:line: message".
- **Turn order:** parse → `fallback_parse()` → reject unknown verb/invisible noun (costs no
  turn) → movement → first matching world-file `RULE` (rules override built-ins) → built-in verb →
  end of turn (move counter, darkness: 3 turns in the dark is fatal).
- `fallback_parse()` in `parser.cpp` is intentionally empty. It is the hook for a future
  embedding/LLM parser. The model may only translate input into existing commands, and game logic
  stays in the engine (see `docs/architecture.md`).

## Tests

Tests are end-to-end only. `test/run_script.cmake` pipes a command file into `tinyadv` with the
sample world and checks that an expected string appears in the output (with `EXPECT_ERROR`: that
tinyadv fails and the string appears on stderr, as in `tinyadv.load_error`). To add a test, write a
command file in `sources/apps/tinyadv/test/` and register an `add_test` in
`sources/apps/tinyadv/CMakeLists.txt`. If you edit `data/world.txt`, the existing walkthrough
tests can break.

## Docs

`docs/world-format.md` is the reference for the world file language (keywords, rules, built-in
verbs). Update it together with any parser or loader change. `docs/architecture.md` describes the
turn pipeline.

## Style

clang-format (LLVM base, 4-space indent, 100 columns, Linux braces) and clang-tidy configs are in
the repo root. The compiler runs with `-Wall -Wextra -pedantic -Wshadow -Wmissing-declarations
-Wold-style-cast -Wnon-virtual-dtor`, so non-static functions need a prior declaration and casts
must be C++ casts (`static_cast` etc.). Use idiomatic C++20 (standard containers, `std::string`,
exceptions); no C idioms such as `malloc`, `char *` buffers or `setjmp`.
