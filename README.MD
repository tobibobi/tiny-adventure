# tinyadv

A tiny, data-driven text adventure engine in C++20, in the spirit of
Colossal Cave Adventure and Zork.

The engine knows nothing about any particular game. Rooms, objects, flags and
puzzle rules live in a plain-text world file, so a new adventure needs no code
changes. Output is plain ASCII wrapped to 80 columns, which makes it equally
happy in a modern terminal or on a serial line.

```
End of the Road
The cracked road ends here in a turning circle overgrown with weeds. A
weathered wooden shed stands to the east, and a narrow path winds north into
the woods. Set into the ground at your feet is a heavy iron grate.
Exits: north, east, down, in

> open grate
It's locked.
```

## Building

Requirements: a C++20 compiler, CMake 3.16 or newer and GNU make. Ninja is used
automatically if installed. Optional: Doxygen (docs), clang-format and
clang-tidy (code style), dpkg (Debian packages).

All builds go through CMake into `build/`, driven by the `GNUmakefile`:

```sh
make            # configure + build into build/
make run        # build and play the sample world
make test       # build and run the tests
make clean      # remove build/
make help       # list all targets and variables
```

Other targets: `rebuild`, `docs`, `install`, `package`, `format`,
`format-check`, `tidy`. Useful variables: `BUILD_TYPE` (default `Release`),
`PREFIX` (default `/usr/local`) and `CMAKE_ARGS`, for example:

```sh
make test BUILD_TYPE=Debug CMAKE_ARGS=-DTINYADV_SANITIZE=ON
```

## Playing

```sh
build/bin/tinyadv [world-file]
```

Without an argument, `tinyadv` looks for `world.txt` in the current directory
and then in the installed data directory. Type short commands such as `N`,
`TAKE LAMP`, `EXAMINE LEAVES` or `UNLOCK GRATE WITH KEY`. `HELP` lists more.

## Project layout

| Path | Contents |
|---|---|
| `sources/apps/tinyadv/` | The terminal front end (`src/`), the sample world (`data/`) and scripted tests (`test/`) |
| `sources/modules/advengine/` | The engine library: public API in `headers/`, implementation in `src/` |
| `sources/package/` | CPack configuration for installers |
| `docs/` | Architecture notes and the world file format |
| `GNUmakefile` | Build proxy around CMake |
| `CMakeLists.txt` | Top-level CMake project |

## Writing your own world

See [docs/world-format.md](docs/world-format.md). The sample world in
`sources/apps/tinyadv/data/world.txt` shows every feature.

## Roadmap

- Save and restore
- Characters you can talk to
- A smarter parser fallback (embedding matcher or a small local language
  model) behind `fallback_parse()`, see
  [docs/architecture.md](docs/architecture.md)
