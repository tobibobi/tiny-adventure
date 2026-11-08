/*
 * parser.c - turns a line of player input into a Cmd.
 *
 * Classic two-word-plus parser: VERB [noun phrase] [preposition noun phrase].
 * Anything it cannot make sense of is offered to adv_fallback_parse().
 */
#include "internal.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const char *const noise[] = {"the", "a", "an", "some", "at", "to", "up", "of", NULL};
static const char *const preps[] = {"with",   "using", "on",    "onto", "in", "into",
                                    "across", "over",  "under", "from", NULL};

static int is_word_char(char ch)
{
    return isalnum((unsigned char)ch) || ch == '?';
}

/* Split into lower-case words, in place. Returns the number of words. */
static int tokenize(char *s, char **w)
{
    int n = 0;
    for (;;) {
        while (*s && !is_word_char(*s))
            s++;
        if (!*s)
            return n;
        if (n < MAXTOK)
            w[n++] = s;
        for (; is_word_char(*s); s++)
            *s = (char)tolower((unsigned char)*s);
        if (*s)
            *s++ = 0;
    }
}

/*
 * Find the object a noun phrase refers to ("brass lamp" -> lamp).
 * Returns an object index, -1 for an empty phrase, R_ABSENT if the noun is
 * known but not visible, or R_UNKNOWN. The offending word goes to *word.
 */
static int resolve(char **g, int n, char *word)
{
    char unknown[NOUNLEN] = "";
    char absent[NOUNLEN] = "";
    if (n == 0)
        return -1;
    for (int i = 0; i < n; i++) {
        int known = 0;
        for (int o = 0; o < G.nobj; o++) {
            if (!adv_has_word(o, g[i]))
                continue;
            known = 1;
            if (adv_can_see(o))
                return o;
        }
        if (known && !absent[0])
            snprintf(absent, sizeof absent, "%s", g[i]);
        if (!known && !unknown[0])
            snprintf(unknown, sizeof unknown, "%s", g[i]);
    }
    if (absent[0]) {
        snprintf(word, NOUNLEN, "%s", absent);
        return R_ABSENT;
    }
    snprintf(word, NOUNLEN, "%s", unknown);
    return R_UNKNOWN;
}

/* Fill in c->verb (and c->dir for movement). Returns the index of the
 * first word after the verb, or -1 if the command is complete. */
static int parse_verb(char **w, int n, Cmd *c)
{
    int d;

    /* "turn on lamp" / "switch off lamp" */
    if (n >= 2 && (!strcmp(w[0], "turn") || !strcmp(w[0], "switch"))) {
        if (!strcmp(w[1], "on")) {
            snprintf(c->verb, sizeof c->verb, "light");
            return 2;
        }
        if (!strcmp(w[1], "off")) {
            snprintf(c->verb, sizeof c->verb, "extinguish");
            return 2;
        }
    }

    const char *v = adv_canon(w[0]);
    d = adv_dir_of(v);
    if (d >= 0) {
        snprintf(c->verb, sizeof c->verb, "go");
        c->dir = d;
        return -1;
    }
    snprintf(c->verb, sizeof c->verb, "%s", v);
    if (!strcmp(v, "go")) {
        if (n > 1)
            c->dir = adv_dir_of(adv_canon(w[1]));
        return -1;
    }
    return 1;
}

int adv_parse(char *line, Cmd *c)
{
    char *w[MAXTOK];
    int n = tokenize(line, w);

    memset(c, 0, sizeof *c);
    c->dir = -1;
    c->o1 = c->o2 = -1;
    if (n == 0)
        return 0;

    int i = parse_verb(w, n, c);
    if (i < 0)
        return 1;

    /* Split the rest into "noun phrase [preposition noun phrase]". */
    char *g[2][MAXTOK];
    int gn[2] = {0, 0};
    int grp = 0;
    for (; i < n; i++) {
        if (adv_in_list(preps, w[i])) {
            if (gn[0] > 0)
                grp = 1;
            continue;
        }
        if (adv_in_list(noise, w[i]))
            continue;
        g[grp][gn[grp]++] = w[i];
    }
    if (!strcmp(c->verb, "look") && gn[0])
        snprintf(c->verb, sizeof c->verb, "examine");

    c->o1 = resolve(g[0], gn[0], c->n1);
    c->o2 = resolve(g[1], gn[1], c->n2);
    return 1;
}

/*
 * Fallback for input the classic parser can't handle (unknown verb or noun).
 *
 * This is where an embedding matcher or a small local language model plugs
 * in: given the raw line and the current room / visible objects, rewrite *c
 * into a valid verb + objects and return 1. Returning 0 leaves *c untouched
 * and the player gets the classic "I don't understand" style reply.
 */
int adv_fallback_parse(const char *raw, Cmd *c)
{
    (void)raw;
    (void)c;
    return 0;
}
