# v2 game rules (draft)

Status: leaning. Nothing here is implemented. These rules come
before the world/script format, because they decide what the engine has to
model.

Version 2 is a point-and-click adventure (verbs, inventory, clickable scenes,
dialogue choices) with rules inspired by tabletop role-playing games. The
goal is to feel like a tabletop game without the engine becoming a full
rules simulator. The text engine of version 1 lives on the `v1` branch.

## Starting rule set

The proposed baseline, to be extended later:

- One hero (no party).
- Six abilities plus a small list of skills.
- d20 checks with advantage and disadvantage.
- Passive Perception reveals hidden clickable areas.
- Failures have consequences instead of allowing free retries.
- Social checks in dialogue.
- Narrative combat; turn-based combat stays open for later.

## Checks

A check is **d20 + ability modifier (+ proficiency bonus if proficient in the
skill) against a difficulty class (DC)**. The modifier is
`(score - 10) / 2`, rounded down.

Checks are triggered by actions, not chosen by the player: clicking *Open* on
a jammed door makes a Strength (Athletics) check, *Look at* on a suspicious
wall an Intelligence (Investigation) check. In dialogue the skill is shown on
the choice, for example `[Persuasion] "The captain sent me."`.

- **Advantage / disadvantage**: roll two d20 and keep the higher / lower. A
  cheap way to express circumstances: the right tool, darkness, a drunk
  guard.
- **Passive checks**: 10 + modifier, no roll. Passive Perception decides
  whether hidden hotspots are visible at all; a perceptive character sees
  more of the scene.

### Retries and failure

In classic adventure games you can click again and again; with dice the player
would simply click until the roll succeeds. Every check therefore uses one of
these, chosen by the author per check:

- **Consequence**: failure costs something (the lock pick breaks, the guard
  notices you, you take damage).
- **One attempt per situation**: the check can only be repeated after
  something changes (a better tool, a different time, a new approach).
- **Fail forward**: failure moves the story on in another direction instead
  of blocking it.

Dice are rolled from a seeded random generator. The seed is part of the save
game, so reloading doesn't reroll a failed check, and scripted tests replay
exactly.

## The character

- **Abilities**: Strength, Dexterity, Constitution, Intelligence, Wisdom,
  Charisma.
- **Skills**: a small list (for example Athletics, Stealth, Sleight of Hand,
  Investigation, Perception, Persuasion, Deception, Intimidation, Arcana),
  each tied to an ability. Proficiency adds a bonus.
- **Hit points** and **armor class**.
- **Progression**: experience and levels, raising the proficiency bonus and
  granting new capabilities. Details open.

## Character types and approaches

Different character types solve the same obstacle differently. A locked door
can be:

- **forced** (warrior: Strength check),
- **picked** (rogue: Dexterity check, needs thieves' tools),
- **opened by magic** (mage: a spell, costs a spell slot),
- **talked open** (bard: persuade the guard to hand over the key).

To keep this manageable for authors:

1. **Solutions are tied to capabilities, not classes.** An obstacle lists the
   capabilities it accepts (`force`, `unlock`, `magic:open`, ...), each with
   its check and outcome. A class is a set of capabilities, skill
   proficiencies and starting equipment. New classes reuse existing content,
   and world scripts don't have to mention every class.
2. **Capabilities appear as verbs.** The verb panel depends on the character:
   a mage has *Cast* with a spell list, a rogue has *Pick* and *Sneak*. The
   difference between characters is visible at a glance.
3. **Every class must be able to finish the game.** Each obstacle on the
   critical path needs a route for every class, or a route that doesn't
   depend on class (a key found elsewhere). A checking tool walks the world
   per class and reports dead ends, so authors can't create an unwinnable
   game by accident.

Magic must be a limited resource (spell slots, recovered by resting), or it
becomes a universal key that makes every other approach pointless.

One hero fits this best: the class chosen at the start shapes the whole
playthrough and gives replay value. A full party would always have every
capability and make the choice meaningless.

## Social encounters

Dialogue choices can carry a skill and roll when chosen. NPCs have an
attitude (hostile, neutral, friendly) that choices and checks shift, and
attitude can open or close options. The scenes-and-options idea from
`v1` (`docs/scenes-proposal.md` on that branch) maps directly onto dialogue
trees.

## Combat

Options, roughly in order of engine cost:

1. **Narrative** (chosen for the start): combat is a scene with options
   (attack, flee, talk, use an item), resolved by checks and hit point loss.
2. **Simple turn-based**: initiative, then each round the player picks an
   action from a menu against one or more enemies.
3. **Tactical grid**: positions, movement and range. A different game; not
   planned.

## Open questions

- Which classes to start with, and their capability lists.
- Magic: spell list, slots, and how spells appear in the interface.
- Resting and resources: hit point recovery, consumables, whether time
  matters.
- Death: game over, being knocked out and captured (fail forward), or death
  saves.
- Progression details: how experience is earned, what a level grants.
- Companions: could a recruitable companion bring one extra capability
  without becoming a full party?

## Licensing note

Game mechanics can't be copyrighted, and the 5th edition System Reference
Document is published under CC-BY-4.0, so its rules can be used with
attribution. Don't use the "Dungeons & Dragons" name or the publisher's own
named monsters and settings.
