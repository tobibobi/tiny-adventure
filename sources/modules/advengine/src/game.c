/*
 * game.c - the turn loop: movement, rules and built-in verbs.
 */
#include "advengine/advengine.h"
#include "internal.h"

#include <stdio.h>
#include <string.h>

static void look(int full)
{
    if (!adv_lit()) {
        adv_say("It is pitch dark. You are likely to be eaten by a grue.");
        return;
    }
    Room *r = &G.room[G.here];
    adv_say(r->name);
    if ((full || !r->seen) && r->desc)
        adv_say(r->desc);
    r->seen = 1;

    for (int o = 0; o < G.nobj; o++)
        if (G.obj[o].loc == G.here && G.obj[o].portable) {
            adv_out("You can see");
            adv_out(G.obj[o].name);
            adv_say("here.");
        }

    char exits[160] = "Exits:";
    int any = 0;
    for (int d = 0; d < NDIR; d++)
        if (r->ex[d].to >= 0) {
            strcat(exits, any ? ", " : " ");
            strcat(exits, adv_dirname[d]);
            any = 1;
        }
    if (any)
        adv_say(exits);
}

static void finish(const char *banner)
{
    char buf[128];
    adv_say("");
    adv_say(banner);
    snprintf(buf, sizeof buf, "You scored %d out of %d points in %d moves.", G.score, G.maxscore,
             G.moves);
    adv_say(buf);
    G.gameover = 1;
}

static void go(int d)
{
    const Exit *e = &G.room[G.here].ex[d];
    if (e->to < 0) {
        adv_say("You can't go that way.");
        return;
    }
    if (e->flag >= 0 && !G.flagval[e->flag]) {
        adv_say(e->fail ? e->fail : "The way is blocked.");
        return;
    }
    G.here = e->to;
    look(0);
}

static int cond_holds(const Op *op)
{
    switch (op->type) {
    case C_IN:
        return G.here == op->a;
    case C_HAVE:
        return G.obj[op->a].loc == CARRIED;
    case C_HERE:
        return adv_can_see(op->a);
    case C_FLAG:
        return G.flagval[op->a];
    case C_NOT:
        return !G.flagval[op->a];
    default:
        return 0;
    }
}

static int rule_matches(const Rule *r, const Cmd *c)
{
    if (strcmp(r->verb, c->verb) != 0)
        return 0;
    if (r->obj != ANY && r->obj != c->o1)
        return 0;
    if (r->with == NONE && c->o2 != -1)
        return 0;
    if (r->with >= 0 && r->with != c->o2)
        return 0;
    for (int i = 0; i < r->ncond; i++)
        if (!cond_holds(&r->cond[i]))
            return 0;
    return 1;
}

static void run_rule(const Rule *r)
{
    for (int i = 0; i < r->neff && !G.gameover; i++) {
        const Op *op = &r->eff[i];
        switch (op->type) {
        case E_SAY: /* consecutive SAY lines flow as one paragraph */
            adv_out(op->s);
            if (i + 1 >= r->neff || r->eff[i + 1].type != E_SAY)
                adv_say("");
            break;
        case E_SET:
            G.flagval[op->a] = 1;
            break;
        case E_CLEAR:
            G.flagval[op->a] = 0;
            break;
        case E_MOVE:
            G.obj[op->a].loc = op->b;
            break;
        case E_SCORE:
            G.score += op->a;
            break;
        case E_GOTO:
            G.here = op->a;
            look(0);
            break;
        case E_LOOK:
            look(1);
            break;
        case E_WIN:
            finish("*** You have won ***");
            break;
        case E_DIE:
            finish("*** You have died ***");
            break;
        default:
            break;
        }
    }
}

static void inventory(void)
{
    int any = 0;
    for (int o = 0; o < G.nobj; o++)
        if (G.obj[o].loc == CARRIED) {
            if (!any)
                adv_say("You are carrying:");
            fputs("  ", stdout);
            adv_say(G.obj[o].name);
            any = 1;
        }
    if (!any)
        adv_say("You are empty-handed.");
}

/* Called after every action that takes game time. */
static void end_turn(void)
{
    G.moves++;
    if (G.gameover)
        return;
    if (adv_lit()) {
        G.dark_turns = 0;
        return;
    }
    G.dark_turns++;
    if (G.dark_turns == 2)
        adv_say("You hear something shuffling closer in the dark...");
    if (G.dark_turns >= 3) {
        adv_say("Something with far too many teeth finds you. You have been eaten by a grue.");
        finish("*** You have died ***");
    }
}

/* Report unknown verbs/nouns. Returns 1 if the command can't be executed. */
static int reject(const Cmd *c)
{
    char buf[160];
    if (!adv_verb_known(c->verb)) {
        snprintf(buf, sizeof buf, "I don't know how to \"%s\".", c->verb);
    } else if (c->o1 == R_UNKNOWN || c->o2 == R_UNKNOWN) {
        snprintf(buf, sizeof buf, "I don't know the word \"%s\".",
                 c->o1 == R_UNKNOWN ? c->n1 : c->n2);
    } else if (c->o1 == R_ABSENT || c->o2 == R_ABSENT) {
        snprintf(buf, sizeof buf, "You don't see any %s here.", c->o1 == R_ABSENT ? c->n1 : c->n2);
    } else {
        return 0;
    }
    adv_say(buf);
    return 1;
}

/* Built-in verbs. Returns 1 if the action took a turn. */
static int builtin(const Cmd *c)
{
    char buf[160];
    const char *v = c->verb;
    Obj *o = c->o1 >= 0 ? &G.obj[c->o1] : NULL;

    if (!strcmp(v, "look")) {
        look(1);
    } else if (!strcmp(v, "inventory")) {
        inventory();
    } else if (!strcmp(v, "wait")) {
        adv_say("Time passes.");
    } else if (!strcmp(v, "examine")) {
        if (!o) {
            adv_say("Examine what?");
            return 0;
        }
        adv_say(o->desc ? o->desc : "You see nothing special.");
    } else if (!strcmp(v, "take")) {
        if (!o) {
            adv_say("Take what?");
            return 0;
        }
        if (o->loc == CARRIED)
            adv_say("You already have it.");
        else if (!o->portable)
            adv_say("You can't take that.");
        else {
            o->loc = CARRIED;
            adv_say("Taken.");
        }
    } else if (!strcmp(v, "drop")) {
        if (!o) {
            adv_say("Drop what?");
            return 0;
        }
        if (o->loc != CARRIED)
            adv_say("You don't have that.");
        else {
            o->loc = G.here;
            adv_say("Dropped.");
        }
    } else if (!strcmp(v, "score")) {
        snprintf(buf, sizeof buf, "You have scored %d out of %d points in %d moves.", G.score,
                 G.maxscore, G.moves);
        adv_say(buf);
        return 0;
    } else if (!strcmp(v, "help")) {
        adv_say("Type short commands, for example: NORTH (or N), TAKE LAMP, EXAMINE LEAVES, "
                "UNLOCK GRATE WITH KEY, INVENTORY (I), LOOK (L), SCORE, QUIT.");
        return 0;
    } else if (!strcmp(v, "quit")) {
        finish("You give up.");
        return 0;
    } else {
        adv_say("That doesn't seem to do anything.");
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* Public API                                                         */
/* ------------------------------------------------------------------ */

void adv_start(void)
{
    adv_reset_column();
    if (G.title) {
        adv_say(G.title);
        adv_say("");
    }
    if (G.intro) {
        adv_say(G.intro);
        adv_say("");
    }
    look(1);
}

int adv_is_over(void)
{
    return G.gameover;
}

void adv_turn(const char *line)
{
    char buf[LINELEN];
    Cmd c;

    adv_reset_column();
    snprintf(buf, sizeof buf, "%s", line);
    if (!adv_parse(buf, &c))
        return;

    /* The classic parser couldn't make sense of it: give the fallback a go. */
    if (!adv_verb_known(c.verb) || c.o1 <= R_ABSENT || c.o2 <= R_ABSENT)
        adv_fallback_parse(line, &c);
    if (reject(&c))
        return;

    if (!strcmp(c.verb, "go")) {
        if (c.dir < 0) {
            adv_say("Go where?");
            return;
        }
        go(c.dir);
        end_turn();
        return;
    }

    /* World rules take priority over built-in verbs. */
    for (int i = 0; i < G.nrule; i++)
        if (rule_matches(&G.rule[i], &c)) {
            run_rule(&G.rule[i]);
            end_turn();
            return;
        }

    if (builtin(&c))
        end_turn();
}
