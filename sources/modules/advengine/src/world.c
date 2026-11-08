/*
 * world.c - world state, lookups and the world file loader.
 */
#include "advengine/advengine.h"
#include "internal.h"

#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct adv_state adv_g;

const char *const adv_dirname[NDIR] = {"north",     "south",     "east",      "west",
                                       "up",        "down",      "northeast", "northwest",
                                       "southeast", "southwest", "in",        "out"};
static const char *const dirabbr[NDIR] = {"n",  "s",  "e",  "w",  "u",  "d",
                                          "ne", "nw", "se", "sw", "in", "out"};

static const char *const builtin_syn[][2] = {
    {"get", "take"},      {"grab", "take"}, {"carry", "take"},      {"pick", "take"},
    {"discard", "drop"},  {"x", "examine"}, {"inspect", "examine"}, {"i", "inventory"},
    {"inv", "inventory"}, {"l", "look"},    {"q", "quit"},          {"z", "wait"},
    {"?", "help"},        {"walk", "go"},   {"run", "go"},          {NULL, NULL}};

static const char *const builtin_verbs[] = {"go",    "look", "examine", "take", "drop", "inventory",
                                            "score", "help", "quit",    "wait", NULL};

/* ------------------------------------------------------------------ */
/* Lookups                                                            */
/* ------------------------------------------------------------------ */

int adv_in_list(const char *const *list, const char *w)
{
    for (; *list; list++)
        if (!strcmp(*list, w))
            return 1;
    return 0;
}

int adv_dir_of(const char *w)
{
    for (int i = 0; i < NDIR; i++)
        if (!strcmp(w, adv_dirname[i]) || !strcmp(w, dirabbr[i]))
            return i;
    return -1;
}

const char *adv_canon(const char *w)
{
    for (int i = 0; i < G.nsyn; i++)
        if (!strcmp(G.syn[i].word, w))
            return G.syn[i].canon;
    for (int i = 0; builtin_syn[i][0]; i++)
        if (!strcmp(builtin_syn[i][0], w))
            return builtin_syn[i][1];
    return w;
}

int adv_verb_known(const char *v)
{
    if (adv_in_list(builtin_verbs, v))
        return 1;
    for (int i = 0; i < G.nrule; i++)
        if (!strcmp(G.rule[i].verb, v))
            return 1;
    return 0;
}

int adv_lit(void)
{
    if (!G.room[G.here].dark)
        return 1;
    for (int o = 0; o < G.nobj; o++) {
        const Obj *ob = &G.obj[o];
        if (ob->lightflag >= 0 && G.flagval[ob->lightflag] &&
            (ob->loc == CARRIED || ob->loc == G.here))
            return 1;
    }
    return 0;
}

int adv_can_see(int o)
{
    return G.obj[o].loc == CARRIED || (G.obj[o].loc == G.here && adv_lit());
}

int adv_has_word(int o, const char *w)
{
    for (int i = 0; i < G.obj[o].nwords; i++)
        if (!strcmp(G.obj[o].words[i], w))
            return 1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Loader helpers                                                     */
/* ------------------------------------------------------------------ */

static jmp_buf load_error;
static const char *wpath;
static int lineno;

static void fail(const char *msg, const char *arg)
{
    fprintf(stderr, "%s:%d: %s%s%s\n", wpath, lineno, msg, arg ? " " : "", arg ? arg : "");
    longjmp(load_error, 1);
}

static void *xalloc(void *p)
{
    if (!p)
        fail("out of memory", NULL);
    return p;
}

static char *xstrdup(const char *s)
{
    return strcpy(xalloc(malloc(strlen(s) + 1)), s);
}

/* Append text to a (possibly NULL) string, separated by a space. */
static char *append(char *old, const char *s)
{
    if (!old)
        return xstrdup(s);
    old = xalloc(realloc(old, strlen(old) + strlen(s) + 2));
    return strcat(strcat(old, " "), s);
}

/* Split off the next whitespace-separated token; *p is left at the rest. */
static char *tok(char **p)
{
    char *s = *p;
    while (*s && isspace((unsigned char)*s))
        s++;
    if (!*s) {
        *p = s;
        return NULL;
    }
    char *t = s;
    while (*s && !isspace((unsigned char)*s))
        s++;
    if (*s)
        *s++ = 0;
    while (*s && isspace((unsigned char)*s))
        s++;
    *p = s;
    return t;
}

static char *need(char **p, const char *what)
{
    char *t = tok(p);
    if (!t)
        fail("missing", what);
    return t;
}

static int find_room(const char *id)
{
    for (int i = 0; i < G.nroom; i++)
        if (!strcmp(G.room[i].id, id))
            return i;
    fail("unknown room", id);
    return -1;
}

static int find_obj(const char *id)
{
    for (int i = 0; i < G.nobj; i++)
        if (!strcmp(G.obj[i].id, id))
            return i;
    fail("unknown object", id);
    return -1;
}

static int find_flag(const char *name)
{
    for (int i = 0; i < G.nflag; i++)
        if (!strcmp(G.flagname[i], name))
            return i;
    if (G.nflag >= MAXFLAG)
        fail("too many flags", NULL);
    G.flagname[G.nflag] = xstrdup(name);
    return G.nflag++;
}

/* A room id, "carried" or "nowhere". */
static int place(const char *w)
{
    if (!strcmp(w, "carried"))
        return CARRIED;
    if (!strcmp(w, "nowhere"))
        return NOWHERE;
    return find_room(w);
}

static Op *new_op(Op *ops, int *n, int type)
{
    if (*n >= MAXOP)
        fail("too many conditions/effects in rule", NULL);
    Op *op = &ops[(*n)++];
    op->type = type;
    op->a = op->b = 0;
    op->s = NULL;
    return op;
}

/* ------------------------------------------------------------------ */
/* Keyword handlers. Each returns 1 if it handled the keyword.        */
/* ------------------------------------------------------------------ */

/* Pass 1: collect room and object ids so forward references work. */
static void declare(const char *kw, char *p)
{
    if (!strcmp(kw, "ROOM")) {
        if (G.nroom >= MAXROOM)
            fail("too many rooms", NULL);
        Room *r = &G.room[G.nroom++];
        r->id = r->name = xstrdup(need(&p, "room id"));
        for (int d = 0; d < NDIR; d++) {
            r->ex[d].to = -1;
            r->ex[d].flag = -1;
        }
    } else if (!strcmp(kw, "OBJECT")) {
        if (G.nobj >= MAXOBJ)
            fail("too many objects", NULL);
        Obj *o = &G.obj[G.nobj++];
        o->id = o->name = xstrdup(need(&p, "object id"));
        o->loc = NOWHERE;
        o->lightflag = -1;
    }
}

static int global_kw(const char *kw, char *p)
{
    if (!strcmp(kw, "TITLE"))
        G.title = xstrdup(p);
    else if (!strcmp(kw, "INTRO"))
        G.intro = append(G.intro, p);
    else if (!strcmp(kw, "START"))
        G.start = find_room(need(&p, "room"));
    else if (!strcmp(kw, "MAXSCORE"))
        G.maxscore = atoi(need(&p, "number"));
    else if (!strcmp(kw, "SYN")) {
        const char *c = need(&p, "word");
        const char *w;
        while ((w = tok(&p))) {
            if (G.nsyn >= MAXSYN)
                fail("too many synonyms", NULL);
            G.syn[G.nsyn].word = xstrdup(w);
            G.syn[G.nsyn++].canon = xstrdup(c);
        }
    } else
        return 0;
    return 1;
}

static void parse_exit(Room *r, char *p)
{
    const char *d = need(&p, "direction");
    int di = adv_dir_of(d);
    if (di < 0)
        fail("bad direction", d);
    Exit *e = &r->ex[di];
    e->to = find_room(need(&p, "destination"));
    const char *k = tok(&p);
    if (k && !strcmp(k, "IF")) {
        e->flag = find_flag(need(&p, "flag"));
        k = tok(&p);
    }
    if (k && !strcmp(k, "ELSE"))
        e->fail = xstrdup(p);
    else if (k)
        fail("expected IF or ELSE, got", k);
}

static int room_kw(Room *r, const char *kw, char *p)
{
    if (!strcmp(kw, "NAME"))
        r->name = xstrdup(p);
    else if (!strcmp(kw, "DESC"))
        r->desc = append(r->desc, p);
    else if (!strcmp(kw, "DARK"))
        r->dark = 1;
    else if (!strcmp(kw, "EXIT"))
        parse_exit(r, p);
    else
        return 0;
    return 1;
}

static int obj_kw(Obj *o, const char *kw, char *p)
{
    if (!strcmp(kw, "NAME"))
        o->name = xstrdup(p);
    else if (!strcmp(kw, "DESC"))
        o->desc = append(o->desc, p);
    else if (!strcmp(kw, "AT"))
        o->loc = place(need(&p, "location"));
    else if (!strcmp(kw, "TAKE"))
        o->portable = 1;
    else if (!strcmp(kw, "LIGHTIF"))
        o->lightflag = find_flag(need(&p, "flag"));
    else if (!strcmp(kw, "WORDS")) {
        const char *w;
        while ((w = tok(&p))) {
            if (o->nwords >= MAXWORD)
                fail("too many words", NULL);
            o->words[o->nwords++] = xstrdup(w);
        }
    } else
        return 0;
    return 1;
}

/* Rule matching (OBJ, WITH) and conditions. */
static int rule_cond_kw(Rule *u, const char *kw, char *p)
{
    if (!strcmp(kw, "OBJ"))
        u->obj = find_obj(need(&p, "object"));
    else if (!strcmp(kw, "WITH")) {
        const char *w = need(&p, "object");
        u->with = strcmp(w, "-") ? find_obj(w) : NONE;
    } else if (!strcmp(kw, "IN"))
        new_op(u->cond, &u->ncond, C_IN)->a = find_room(need(&p, "room"));
    else if (!strcmp(kw, "HAVE"))
        new_op(u->cond, &u->ncond, C_HAVE)->a = find_obj(need(&p, "object"));
    else if (!strcmp(kw, "HERE"))
        new_op(u->cond, &u->ncond, C_HERE)->a = find_obj(need(&p, "object"));
    else if (!strcmp(kw, "FLAG"))
        new_op(u->cond, &u->ncond, C_FLAG)->a = find_flag(need(&p, "flag"));
    else if (!strcmp(kw, "NOT"))
        new_op(u->cond, &u->ncond, C_NOT)->a = find_flag(need(&p, "flag"));
    else
        return 0;
    return 1;
}

static int rule_effect_kw(Rule *u, const char *kw, char *p)
{
    if (!strcmp(kw, "SAY"))
        new_op(u->eff, &u->neff, E_SAY)->s = xstrdup(p);
    else if (!strcmp(kw, "SET"))
        new_op(u->eff, &u->neff, E_SET)->a = find_flag(need(&p, "flag"));
    else if (!strcmp(kw, "CLEAR"))
        new_op(u->eff, &u->neff, E_CLEAR)->a = find_flag(need(&p, "flag"));
    else if (!strcmp(kw, "SCORE"))
        new_op(u->eff, &u->neff, E_SCORE)->a = atoi(need(&p, "points"));
    else if (!strcmp(kw, "GOTO"))
        new_op(u->eff, &u->neff, E_GOTO)->a = find_room(need(&p, "room"));
    else if (!strcmp(kw, "WIN"))
        new_op(u->eff, &u->neff, E_WIN);
    else if (!strcmp(kw, "DIE"))
        new_op(u->eff, &u->neff, E_DIE);
    else if (!strcmp(kw, "LOOK"))
        new_op(u->eff, &u->neff, E_LOOK);
    else if (!strcmp(kw, "MOVE")) {
        Op *op = new_op(u->eff, &u->neff, E_MOVE);
        op->a = find_obj(need(&p, "object"));
        op->b = place(need(&p, "destination"));
    } else
        return 0;
    return 1;
}

/* The block (ROOM, OBJECT or RULE) that following keywords belong to. */
typedef struct {
    Room *room;
    Obj *obj;
    Rule *rule;
} Block;

/* Pass 2: interpret one line. */
static void define(const char *kw, char *p, Block *b)
{
    if (!strcmp(kw, "ROOM")) {
        *b = (Block){&G.room[find_room(need(&p, "room id"))], NULL, NULL};
    } else if (!strcmp(kw, "OBJECT")) {
        *b = (Block){NULL, &G.obj[find_obj(need(&p, "object id"))], NULL};
    } else if (!strcmp(kw, "RULE")) {
        if (G.nrule >= MAXRULE)
            fail("too many rules", NULL);
        Rule *u = &G.rule[G.nrule++];
        u->verb = xstrdup(need(&p, "verb"));
        u->obj = u->with = ANY;
        *b = (Block){NULL, NULL, u};
    } else if (!strcmp(kw, "END")) {
        *b = (Block){NULL, NULL, NULL};
    } else {
        int handled = global_kw(kw, p) || (b->room && room_kw(b->room, kw, p)) ||
                      (b->obj && obj_kw(b->obj, kw, p)) ||
                      (b->rule && (rule_cond_kw(b->rule, kw, p) || rule_effect_kw(b->rule, kw, p)));
        if (!handled)
            fail("unexpected keyword", kw);
    }
}

/* ------------------------------------------------------------------ */

int adv_load(const char *path)
{
    char buf[LINELEN];
    FILE *f = fopen(path, "r");
    if (!f) {
        perror(path);
        return -1;
    }
    wpath = path;
    G.start = -1;

    if (setjmp(load_error)) {
        fclose(f);
        return -1;
    }

    for (int pass = 1; pass <= 2; pass++) {
        Block block = {NULL, NULL, NULL};
        if (fseek(f, 0, SEEK_SET) != 0)
            fail("cannot rewind", NULL);
        lineno = 0;
        while (fgets(buf, sizeof buf, f)) {
            lineno++;
            buf[strcspn(buf, "\r\n")] = 0;
            char *p = buf;
            const char *kw = tok(&p);
            if (!kw || kw[0] == '#')
                continue;
            if (pass == 1)
                declare(kw, p);
            else
                define(kw, p, &block);
        }
    }

    /* Resolve rule verbs through synonyms, so "RULE put" == "RULE drop". */
    for (int i = 0; i < G.nrule; i++) {
        const char *c = adv_canon(G.rule[i].verb);
        if (c != G.rule[i].verb) {
            char *v = xstrdup(c);
            free(G.rule[i].verb);
            G.rule[i].verb = v;
        }
    }
    fclose(f);

    if (G.nroom == 0) {
        fprintf(stderr, "%s: no rooms defined\n", path);
        return -1;
    }
    if (G.start < 0)
        G.start = 0;
    G.here = G.start;
    return 0;
}
