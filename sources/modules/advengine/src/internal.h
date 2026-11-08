/*
 * internal.h - definitions shared between the advengine source files.
 * Not part of the public API; only included from inside the module.
 */
#ifndef ADVENGINE_INTERNAL_H
#define ADVENGINE_INTERNAL_H

#define ADV_WIDTH 78 /* wrap column for 80-column terminals */
#define MAXROOM   64
#define MAXOBJ    64
#define MAXFLAG   64
#define MAXRULE   128
#define MAXSYN    96
#define MAXWORD   8 /* nouns per object */
#define MAXOP     8 /* conditions / effects per rule */
#define MAXTOK    16
#define LINELEN   512
#define NOUNLEN   32
#define NDIR      12

#define NOWHERE   (-1) /* object locations */
#define CARRIED   (-2)
#define ANY       (-1) /* rule OBJ/WITH: don't care */
#define NONE      (-3) /* rule WITH -: no second object allowed */
#define R_ABSENT  (-4) /* noun resolution results */
#define R_UNKNOWN (-5)

typedef struct {
    int to;   /* destination room, -1 = no exit */
    int flag; /* required flag, -1 = always open */
    char *fail;
} Exit;

typedef struct {
    char *id, *name, *desc;
    int dark, seen;
    Exit ex[NDIR];
} Room;

typedef struct {
    char *id, *name, *desc;
    char *words[MAXWORD];
    int nwords, loc, portable, lightflag;
} Obj;

enum { C_IN, C_HAVE, C_HERE, C_FLAG, C_NOT };
enum { E_SAY, E_SET, E_CLEAR, E_MOVE, E_SCORE, E_WIN, E_DIE, E_GOTO, E_LOOK };

typedef struct {
    int type, a, b;
    char *s;
} Op;

typedef struct {
    char *verb;
    int obj, with;
    Op cond[MAXOP];
    int ncond;
    Op eff[MAXOP];
    int neff;
} Rule;

/** One parsed player command. */
typedef struct {
    char verb[NOUNLEN];
    int dir, o1, o2;
    char n1[NOUNLEN], n2[NOUNLEN]; /* noun words, for error messages */
} Cmd;

typedef struct {
    char *word, *canon;
} Synonym;

/** The complete world and game state. */
struct adv_state {
    Room room[MAXROOM];
    int nroom;
    Obj obj[MAXOBJ];
    int nobj;
    char *flagname[MAXFLAG];
    int flagval[MAXFLAG];
    int nflag;
    Rule rule[MAXRULE];
    int nrule;
    Synonym syn[MAXSYN];
    int nsyn;
    char *title, *intro;
    int start, here, score, maxscore, moves, dark_turns, gameover;
};

extern struct adv_state adv_g;
#define G adv_g

extern const char *const adv_dirname[NDIR];

/* output.c */
void adv_out(const char *s);
void adv_say(const char *s);
void adv_reset_column(void);

/* world.c */
int adv_in_list(const char *const *list, const char *w);
int adv_dir_of(const char *w);
const char *adv_canon(const char *w);
int adv_verb_known(const char *v);
int adv_lit(void);
int adv_can_see(int o);
int adv_has_word(int o, const char *w);

/* parser.c */
int adv_parse(char *line, Cmd *c);
int adv_fallback_parse(const char *raw, Cmd *c);

#endif /* ADVENGINE_INTERNAL_H */
