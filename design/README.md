# Design notes (brainstorm)

This folder lives on the `design` branch only. It is deliberately messy: a
place to write ideas down in full, try alternatives side by side and argue
with ourselves. As decisions are made, the notes get boiled down into a few
clean documents, and the branch is rebased/squashed before implementation
starts on `main`.

## Status markers

Put one at the top of each file, and use them on individual sections when a
file mixes settled and open parts:

| Marker | Meaning |
|---|---|
| `idea` | Brainstorm; nothing agreed yet |
| `open` | Discussed, but with open questions |
| `leaning` | We have a preferred direction, not final |
| `decided` | Agreed; candidate for the boiled-down documents |
| `dropped` | Rejected; kept with the reason, so we don't rediscuss it |

## Files

| File | Topic | Status |
|---|---|---|
| [rules.md](rules.md) | Starting rule set: checks, retries, character, character types and approaches, social, combat | leaning |

Planned topics, not written yet: classes and capabilities, magic, combat in
detail, dialogue and NPC attitude, progression, resting and resources,
death and failure, the verb interface, the solvability checker, and finally
the world/script format.

## Related material elsewhere

- `v1` branch: the original C99 text engine, and
  `docs/scenes-proposal.md` (scenes, options and checks for the text
  version; much of it carries over to dialogue).
- `main`: the C++20 port of the v1 engine, a checkpoint with undecided
  architecture.
