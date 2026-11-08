# World file format

A world file is plain text, one keyword per line. Blank lines and lines
starting with `#` are ignored. Text after a keyword runs to the end of the
line. Ids (rooms, objects, flags) are single words and may be referenced
before they are defined.

## Global keywords

| Keyword | Meaning |
|---|---|
| `TITLE text` | Shown when the game starts |
| `INTRO text` | Introduction; repeat the line to add more text |
| `START room` | Starting room (default: the first room) |
| `MAXSCORE n` | Maximum score, shown by `SCORE` |
| `SYN canonical word word ...` | Words that mean the same as `canonical`, e.g. `SYN drop put place` |

## Rooms

```
ROOM road
NAME End of the Road
DESC The cracked road ends here ...
DESC (more lines are appended)
EXIT east shed
EXIT down chamber IF grate_open ELSE The iron grate is locked shut.
```

| Keyword | Meaning |
|---|---|
| `NAME text` | Room title |
| `DESC text` | Description, repeatable |
| `DARK` | The room needs a light source. Staying three turns in the dark is fatal |
| `EXIT dir room [IF flag] [ELSE message]` | An exit. With `IF`, it is only open while the flag is set, otherwise the `ELSE` message is shown |

Directions: `n s e w u d ne nw se sw in out` or their full names.

## Objects

```
OBJECT lamp
NAME a brass lamp
WORDS lamp lantern brass
DESC An old brass oil lamp.
AT shed
TAKE
LIGHTIF lamp_on
```

| Keyword | Meaning |
|---|---|
| `NAME text` | Name including article, as used in "You can see a brass lamp here." |
| `WORDS noun ...` | Words the player can use for it |
| `DESC text` | Shown by `EXAMINE`, repeatable |
| `AT room` / `carried` / `nowhere` | Starting location (default: nowhere, i.e. hidden) |
| `TAKE` | The object can be picked up. Objects without it are scenery |
| `LIGHTIF flag` | Gives light while the flag is set |

## Rules

A rule fires when the player uses its verb and all its conditions hold.
Rules are tried in file order and the first match wins. Rules are checked
before the built-in verbs, so they can override them, and a rule with no
conditions placed last works as a catch-all message.

```
RULE unlock
OBJ grate
HAVE key
SAY The key turns and the grate swings open.
SET grate_open
SCORE 1
```

Matching:

| Keyword | Meaning |
|---|---|
| `RULE verb` | Starts a rule. Synonyms are resolved, so `RULE put` equals `RULE drop` if `SYN drop put` |
| `OBJ object` | The first object in the command must be this one |
| `WITH object` | The second object ("... with key") must be this one. `WITH -` means no second object allowed |

Conditions:

| Keyword | Holds when |
|---|---|
| `IN room` | The player is in the room |
| `HAVE object` | The player carries the object |
| `HERE object` | The object is carried or visible in the room |
| `FLAG flag` | The flag is set |
| `NOT flag` | The flag is not set |

Effects, run in order:

| Keyword | Effect |
|---|---|
| `SAY text` | Print text. Consecutive `SAY` lines flow into one paragraph |
| `SET flag` / `CLEAR flag` | Set or clear a flag |
| `MOVE object room` | Move an object (`carried` and `nowhere` also work) |
| `SCORE n` | Add points |
| `GOTO room` | Move the player and describe the new room |
| `LOOK` | Describe the current room in full |
| `WIN` / `DIE` | End the game |

`END` closes the current block. It is optional, since the next `ROOM`,
`OBJECT` or `RULE` does the same.

Flags need no declaration and start cleared.

## Built-in verbs

`go`, `look`, `examine`, `take`, `drop`, `inventory`, `score`, `help`,
`quit` and `wait`, plus the usual abbreviations (`n`, `x`, `i`, `l`, ...).
`turn on X` and `turn off X` become the verbs `light` and `extinguish`.

## Limits

There are no fixed limits on the number of rooms, objects, flags, rules,
synonyms, words per object, or conditions and effects per rule.
