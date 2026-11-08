/*
 * output.c - word-wrapped text output.
 */
#include "internal.h"

#include <stdio.h>

static int col;

void adv_reset_column(void)
{
    col = 0;
}

/* Print text, wrapping at ADV_WIDTH. Runs of spaces collapse to one. */
void adv_out(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            putchar('\n');
            col = 0;
            s++;
            continue;
        }
        if (*s == ' ') {
            s++;
            continue;
        }
        const char *e = s;
        while (*e && *e != ' ' && *e != '\n')
            e++;
        int len = (int)(e - s);
        if (col > 0 && col + 1 + len > ADV_WIDTH) {
            putchar('\n');
            col = 0;
        } else if (col > 0) {
            putchar(' ');
            col++;
        }
        fwrite(s, 1, (size_t)len, stdout);
        col += len;
        s = e;
    }
}

/* Print text and end the line. */
void adv_say(const char *s)
{
    adv_out(s);
    putchar('\n');
    col = 0;
}
