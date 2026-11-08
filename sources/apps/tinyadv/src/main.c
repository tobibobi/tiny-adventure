/*
 * main.c - tinyadv: plays an advengine world on stdin/stdout.
 *
 * Usage: tinyadv [world-file]
 *
 * Without an argument it looks for world.txt in the current directory, then
 * in the installed data directory (TINYADV_DATADIR).
 */
#include "advengine/advengine.h"

#include <stdio.h>
#include <string.h>

#ifndef TINYADV_VERSION
#define TINYADV_VERSION "dev"
#endif
#ifndef TINYADV_DATADIR
#define TINYADV_DATADIR "."
#endif

#define LINE_MAX_LEN 512

static const char *default_world(void)
{
    FILE *f = fopen("world.txt", "r");
    if (f) {
        fclose(f);
        return "world.txt";
    }
    return TINYADV_DATADIR "/world.txt";
}

int main(int argc, char **argv)
{
    char line[LINE_MAX_LEN];

    if (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        printf("Usage: %s [world-file]\n", argv[0]);
        return 0;
    }
    if (argc > 1 && (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))) {
        printf("tinyadv %s\n", TINYADV_VERSION);
        return 0;
    }

    if (adv_load(argc > 1 ? argv[1] : default_world()) != 0)
        return 1;

    adv_start();
    while (!adv_is_over()) {
        fputs("\n> ", stdout);
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin)) {
            putchar('\n');
            break;
        }
        line[strcspn(line, "\r\n")] = 0;
        adv_turn(line);
    }
    return 0;
}
