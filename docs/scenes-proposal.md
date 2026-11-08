# Proposal: scenes, options and checks

Status: draft for review. Nothing here is implemented.

## Goal

Make tinyadv play more like a tabletop role-playing game: the player
describes what they do in their own words, choices have consequences, and
dice decide whether a risky action works. At the same time keep the engine
small, deterministic and runnable without a language model.

## Approach

Instead of trying to understand any sentence about anything, the engine only
has to understand what the player can do **in the current scene**. Each scene
lists its options explicitly, and the author makes that list large: many
options, each with several outcomes. Free-form input is matched to one of the
options, or to none of them.

This narrows the language problem from "understand and respond to anything" to
"which of these N options did the player mean?". That is a classification
task, which a keyword matcher, a sentence-embedding model or a very small
language model can do well, and the model can never invent story or change
game state.

Rooms stay as they are. Exploring, taking and dropping objects keep working.
Scenes are added on top, as the place where encounters happen: a guard at a
gate, a merchant, a trapped corridor.

## World file additions

### Player character

```
PLAYER
STAT str 10
STAT dex 14
STAT cha 12
HP 10
```

`STAT name value` defines an ability score. Names are free-form; the world
decides which abilities exist. The modifier for a check is
`(value - 10) / 2`, rounded down, as in common tabletop systems.

### Scenes

```
SCENE gate
AT city_road
IF NOT gate_passed
DESC A bored guard leans on his spear by the city gate.
BLOCKS north

OPTION bribe
MEANS bribe the guard, offer him coins, pay him off, slip him some money
HAVE coins
CHECK cha 10
PASS SAY He pockets the coins and looks the other way.
PASS MOVE coins nowhere
PASS SET gate_passed
PASS GOTO courtyard
FAIL SAY "Bribing a city guard? You can explain that to the captain."
FAIL GOTO cell

OPTION sneak
MEANS sneak past, slip by, wait until he looks away, hide in the crowd
CHECK dex 14
PASS SAY You slip through behind a passing cart.
PASS SET gate_passed
PASS GOTO courtyard
FAIL SAY The guard grabs your collar. "Not so fast."

OPTION talk
MEANS talk to the guard, greet him, ask about the city, chat
SAY "Gate's closed to strangers after dark. Rules are rules."

OFFPATH The guard eyes you. "State your business or move along."
```

| Keyword | Meaning |
|---|---|
| `SCENE id` | Starts a scene |
| `AT room` | The scene is active while the player is in this room |
| `IF ...` | Extra conditions for the scene to be active, using the rule conditions (`FLAG`, `NOT`, `HAVE`, ...) |
| `DESC text` | Shown after the room description while the scene is active, repeatable |
| `BLOCKS dir ...` | Exits that can't be used while the scene is active |
| `OPTION id` | Starts an option within the scene |
| `MEANS phrase, phrase, ...` | Example phrasings the matcher compares input with, repeatable |
| `OFFPATH text` | In-character reply when input matches no option and no built-in verb, repeatable |

Within an option:

| Keyword | Meaning |
|---|---|
| Rule conditions (`HAVE`, `FLAG`, `NOT`, ...) | The option is only available while these hold |
| `CHECK stat dc` | Roll d20 + modifier against the difficulty `dc`. At most one per option |
| `PASS effect` | Effect run when the check succeeds |
| `FAIL effect` | Effect run when the check fails |
| Plain effects (`SAY`, `SET`, ...) | Run always; for an option without `CHECK` these are its outcome |

A scene ends when its conditions stop holding, typically because an outcome
sets a flag or moves the player. There is no separate "end scene" effect.

### New effects

| Keyword | Effect |
|---|---|
| `DAMAGE n` | Lose `n` hit points; at 0 the game ends as with `DIE` |
| `HEAL n` | Regain `n` hit points, up to the maximum |

## Turn order with scenes

The existing pipeline in [architecture.md](architecture.md) gets one new step
before movement:

1. Parse as today.
2. If a scene is active: match the input against the options available in it.
   On a match, run the option and end the turn.
3. Otherwise continue as today: fallback, reject, movement (minus blocked
   exits), rules, built-in verbs.
4. If a scene is active and nothing handled the input, print its `OFFPATH`
   text instead of the generic "I don't understand".

Built-in verbs like `inventory`, `look` and `score` keep working during a
scene, because they are tried when no option matches.

## Matching input to options

The matcher is behind one function, so it can be improved without touching
world files. It returns an option id and a confidence, or no match.

1. **Keyword overlap (no model, always built).** Score each available option
   by the words its `MEANS` phrases share with the input, ignoring filler
   words. This is the baseline and what the tests run against.
2. **Sentence embeddings (optional).** A small embedding model compares the
   input with each `MEANS` phrase and takes the closest above a threshold.
   This handles paraphrases like "I wave some gold under his nose" for
   `bribe`.
3. **Small language model (optional).** Only used when the embedding match is
   unsure. A grammar restricts its output to the ids of available options or
   `none`.

Models are enabled with a build option (for example `-DTINYADV_LLM=ON`), so
the default build keeps its current size and still works over a serial line.

## Determinism and testing

Dice use a seeded pseudo-random generator. `tinyadv --seed N` fixes the seed,
so scripted tests replay exactly, including checks. Matcher quality gets its
own tests: a list of phrasings per option that must select it.

## Limits

New fixed tables, following the existing ones in `internal.h`. The numbers
are first guesses: 32 scenes, 16 options per scene, 16 `MEANS` phrases per
option, 8 stats.

## Open questions

- Should available options ever be shown to the player (for example via
  `HINT`), or only discovered by trying?
- Should a failed check be retryable, or should each option be usable once
  per scene unless the author says otherwise?
- Do checks need advantage/disadvantage, or opposed checks (player against an
  NPC's stat)?
- Combat: a scene type of its own, or plain options with `DAMAGE`?
- Is one `CHECK` per option enough, or do some options need several (climb,
  then jump)?
