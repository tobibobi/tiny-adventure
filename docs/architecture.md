# Architecture

> **Status:** this describes the current engine, a C++20 port of the version 1
> text engine. Its architecture is not decided for version 2; see
> [design/rules.md](../design/rules.md) on the `design` branch.

## Components

- **advengine** (`sources/modules/advengine`) is a static library holding the
  whole engine. Its public API is the class `adv::Game` in
  `headers/advengine/advengine.h`: `load`, `start`, `turn` and `is_over`.
- **tinyadv** (`sources/apps/tinyadv`) is a thin terminal front end: it reads
  lines from stdin and hands them to `Game::turn`. A different front end (a
  serial console, a network server) only needs to replace this loop.

## Engine source files

| File | Responsibility |
|---|---|
| `world.cpp` | The game state, lookups and the world file loader |
| `parser.cpp` | Turns a line of input into a command (`Cmd`) |
| `game.cpp` | Executes a command: movement, rules, built-in verbs, scoring, darkness |
| `output.cpp` | Word-wrapped output |
| `internal.h` | Types and declarations shared by the files above |

All state of one game, the loaded world plus the progress of play, lives in a
`World` owned by the `Game`. There is no global state.

The loader reads the file into memory and makes two passes over it: the
first collects room and object ids, so the second can resolve references
regardless of order. Errors are reported with file name and line number by
throwing `adv::LoadError`; a failed load leaves the game unchanged.

## One turn

1. **Parse.** The input is lower-cased and split into words. The first word
   is the verb (after synonyms), unless it is a direction. The rest splits at
   a preposition into a first and second noun phrase, ignoring filler words
   like "the". Each phrase is matched against the `WORDS` of visible objects.
2. **Fallback.** If the verb or a noun is not understood,
   `fallback_parse()` gets a chance to rewrite the command.
3. **Reject.** Commands with an unknown verb or a noun that isn't visible get
   a short explanation and don't use a turn.
4. **Movement.** `go` commands follow an exit, if its flag allows it.
5. **Rules.** The first rule whose verb, objects and conditions match runs its
   effects.
6. **Built-ins.** Otherwise a built-in verb handles it.
7. **End of turn.** The move counter increases, and darkness is checked: three
   turns in a dark room without light are fatal.

## The parser fallback

`fallback_parse()` in `parser.cpp` is deliberately empty. It is the planned
hook for smarter input handling that stays small enough for modest hardware:

1. **Embedding matcher**: a small sentence-embedding model (MiniLM-class,
   tens of MB, via ONNX Runtime) compares the input with the commands that
   are valid in the current room and picks the closest one above a threshold.
2. **Small local language model**: a quantized 1B-class model through
   llama.cpp, with a grammar that only allows valid verbs and object ids as
   output. Used only when the embedding match is unsure.

The world logic always stays in the engine: the model only translates what the
player meant into a command the engine already understands, so puzzles stay
fair and deterministic.
