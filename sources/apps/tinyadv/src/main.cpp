/*
 * main.cpp - tinyadv: plays an advengine world on stdin/stdout.
 *
 * Usage: tinyadv [world-file]
 *
 * Without an argument it looks for world.txt in the current directory, then
 * in the installed data directory (TINYADV_DATADIR).
 */
#include "advengine/advengine.h"

#include <filesystem>
#include <iostream>
#include <span>
#include <string>
#include <string_view>

#ifndef TINYADV_VERSION
#define TINYADV_VERSION "dev"
#endif
#ifndef TINYADV_DATADIR
#define TINYADV_DATADIR "."
#endif

namespace
{

std::filesystem::path default_world()
{
    if (std::filesystem::exists("world.txt"))
        return "world.txt";
    return std::filesystem::path(TINYADV_DATADIR) / "world.txt";
}

} // namespace

int main(int argc, char **argv)
{
    const std::span<char *> args(argv, static_cast<std::size_t>(argc));
    const std::string_view arg = args.size() > 1 ? args[1] : "";

    if (arg == "-h" || arg == "--help") {
        std::cout << "Usage: " << args[0] << " [world-file]\n";
        return 0;
    }
    if (arg == "-v" || arg == "--version") {
        std::cout << "tinyadv " << TINYADV_VERSION << '\n';
        return 0;
    }

    adv::Game game(std::cout);
    try {
        game.load(args.size() > 1 ? std::filesystem::path(arg) : default_world());
    } catch (const adv::LoadError &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    game.start();
    std::string line;
    while (!game.is_over()) {
        std::cout << "\n> " << std::flush;
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            break;
        }
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        game.turn(line);
    }
    return 0;
}
