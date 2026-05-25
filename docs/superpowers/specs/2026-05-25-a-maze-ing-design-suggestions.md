# A Maze Ing - Design Suggestions

**Date:** 2026-05-25
**Review target:** `2026-05-25-a-maze-ing-design.md`
**Status:** Design review suggestions

## Summary

The design is buildable and has a clear identity: a calm, handcrafted, top-down maze adventure with collect-and-use progression. The strongest parts are the scoped engine choice, plain-text content pipeline, explicit save model, and intentionally low-pressure failure model.

The main risk is that the MVP is doing more than the implementation budget implied by "greenfield C11 + raylib" suggests. Four to six 30-60 minute handcrafted mazes, isometric pre-rendered art, layered ambient audio, localization infrastructure, SQLite persistence, time gates, outfit compositing, and cross-platform packaging together form a large first release. The suggestions below focus on preserving the concept while reducing ambiguity and preventing early architecture from becoming brittle.

## Priority Suggestions

### 1. Narrow the first playable milestone

Keep the full MVP vision, but add a smaller milestone before MVP:

- One playable maze.
- One outfit gate.
- One consumable item interaction.
- One NPC dialog.
- One audio base layer plus one zone layer.
- Save/resume for that maze only.
- Placeholder art accepted, final render pipeline exercised.

This gives the project a vertical slice that proves movement, collision, camera, data loading, interaction, rendering order, localization lookup, and saves before the content plan expands. Without this, there is a real risk of building many subsystems before proving the game is pleasant to move through.

### 2. Make soft-lock prevention an MVP requirement

The current design allows whole-maze reset if a player uses the last seed incorrectly. For a relaxing game, that is a harsh penalty even if there is no death. Avoiding soft-locks should be part of MVP, but it does not have to mean full undo.

Recommended approach:

- Prefer non-consumable tools for critical progression.
- For consumables, guarantee replenishment somewhere reachable.
- If an item can be spent incorrectly, make the target reject it instead of consuming it.
- Reserve whole-maze reset for explicit player choice, not routine puzzle recovery.

This keeps the no-fail promise true without adding an action-log system early.

### 3. Define interaction rules in more detail

The spec describes collect-and-use progression, but not how the player selects or applies items. Pin this before implementation because it affects UI, save data, entity behavior, and play feel.

Suggested decisions:

- Interact uses one primary action key.
- If the player faces or overlaps an interactable, the game selects the nearest eligible target.
- If exactly one inventory item applies, use it automatically.
- If multiple items apply, open a compact inventory picker.
- If no item applies, show a short localized hint from the target.

Also define interaction range, priority ordering, and tie-breaking. Ambiguous interactions are common in dense handcrafted mazes.

### 4. Reconsider free 8-direction movement with isometric tile mazes

Pixel-granular 8-direction movement can feel good, but it complicates collision and puzzle readability in an isometric grid. Ice sliding, narrow gates, thorn/water blockers, and y-sorted decor all depend on predictable spatial rules.

Recommendation: keep free movement if that is important to the feel, but specify collision as shape-based rather than tile-center based:

- Player has a small foot rectangle or capsule, not a full sprite bbox.
- Blocking entities expose collision shapes independent from art bounds.
- Hazards are trigger regions that can overlap walkable ground.
- Interactions use a separate proximity shape.

This avoids the common isometric problem where visuals, walkable space, and interaction space disagree.

### 5. Add stable content IDs to the maze format

`entity_overrides` persists by `entity_id`, but the maze format does not show explicit IDs. If IDs are assigned by load order, save files become fragile when designers reorder or insert entities.

Add explicit stable IDs in maze data, for example:

```text
seed#nature.seed.west_glade        12 8   pickup item:seed
sundial#nature.sundial.main        20 15  gate:12-13
locked_gate#nature.gate.ice_path   50 18  needs:winter_coat target:area_b
```

Save tables should use text IDs for inventory, outfits, mazes, and entities unless there is a strong reason to use integers. Human-readable IDs make debugging and migrations easier.

### 6. Separate profile-wide inventory from per-maze state

The design says maze reset preserves outfits and items owned by the profile from prior mazes. That implies at least two item categories:

- Profile-level unlocks: outfits, permanent tools, maze access items.
- Maze-local state: keys, tickets, seeds, temporary puzzle items.

Make this explicit in both design and schema. Otherwise resets, cross-maze requirements, and save migration will get muddy. A simple `scope` field on item definitions may be enough.

### 7. Reduce time-gate friction

Atmospheric time of day is a good fit. Mandatory time gates can become waiting puzzles, especially with a 70-minute day-night cycle.

Suggested rule: every required time gate should have a nearby time-advance affordance and a clear hint. Optional secrets can rely more on observation. This keeps benches/campfires from being a convenience feature and makes them part of puzzle readability.

### 8. Add a content validation tool early

The plain-text maze format is practical, but handcrafted data will need automated validation. Add a small CLI validator before content grows.

It should check:

- Required headers exist.
- Map dimensions match `size`.
- Start position is walkable.
- Entity IDs are unique and stable.
- Behavior names resolve.
- Localization keys exist in `en.lang`.
- Entity positions are inside bounds.
- Audio zones are valid rectangles.
- Required progression items are reachable at least by simple graph rules.

This can start as a developer tool, not a polished editor.

## Gameplay Suggestions

### Make the core loop explicit

The player loop should be written in one short section, for example:

1. Explore until blocked or intrigued.
2. Find an item, outfit, clue, or NPC.
3. Use that discovery to open a route or change the maze state.
4. Reach a new landmark and repeat.
5. Unlock the goal path.

This helps every subsystem serve the same experience and gives playtests something concrete to evaluate.

### Define hinting expectations

Because there are no enemies, timers, or fail states, confusion is the main source of friction. Add a lightweight hint policy:

- Signs and NPCs should hint at nearby blockers.
- Failed item use should explain why it did not work.
- The inventory should expose short item descriptions.
- Repeated failed interactions can escalate the hint text.

This can all route through localization keys and does not require a separate hint system at first.

### Decide whether mazes are independent or metroidvania-like

The spec says some mazes require outfits from prior mazes. That creates cross-maze progression. Decide how strong that dependency should be:

- Mostly independent mazes with light unlock ordering.
- A hub-and-spoke sequence.
- A larger metroidvania progression across themed worlds.

The save schema and maze-select UI should reflect this choice.

### Add accessibility requirements before UI implementation

Even for MVP, consider specifying:

- Remappable keyboard controls.
- Text speed or instant text.
- Master, ambience, and SFX volume sliders.
- Screenshake disabled by default or absent.
- High-contrast interaction prompts.
- Color-independent puzzle cues, especially for Circuit Board.

Controller support can remain out of scope, but keyboard remapping is worth considering early because it affects input architecture.

## Technical Suggestions

### Consider vendoring raylib rather than FetchContent for release stability

`FetchContent` is convenient, but it makes clean builds depend on network access unless the dependency is already cached. For a small C game, a vendored raylib source or documented dependency bootstrap may be more predictable.

If FetchContent stays, pin an exact tag or commit and support an offline build path.

### Avoid Werror by default for third-party and release builds

`-Werror` is useful in CI for first-party code, but it can create cross-compiler friction, especially on Windows/MSVC or newer compiler versions. Use it for first-party code in CI or developer strict mode, but do not let third-party warnings break normal builds.

### Define memory ownership conventions

The architecture says all state flows through `Game *g`, but C projects need explicit ownership rules. Add conventions for:

- Who allocates and frees maze data.
- Whether entity arrays are stable or can be reallocated.
- How asset cache references are acquired and released.
- Whether strings are interned, copied, or borrowed.
- How errors are reported without exceptions.

This is especially important for save loading, maze reloads, and localization switching.

### Specify deterministic update order

Add a short update pipeline to match the render pipeline:

1. Poll input.
2. Update UI or gameplay state.
3. Resolve player movement.
4. Apply hazards/triggers.
5. Run entity step callbacks.
6. Advance clock if unpaused.
7. Queue save events.
8. Update audio targets.

This will prevent bugs around time gates, stepping onto hazards, and interactions that mutate entity state mid-frame.

### Revisit entity callback shape

Function pointers per entity are fine, but the current `on_interact` / `on_step` model may become too coarse. Consider an event-style surface:

- `on_interact`
- `on_player_enter`
- `on_player_exit`
- `on_time_changed`
- `on_loaded`

Not every callback has to exist at first, but naming expected event boundaries will help avoid stuffing unrelated behavior into `on_step`.

### Make save writes transactional and debounced

Auto-saving on every meaningful event is right for the player experience. Implementation should batch closely related changes into one transaction. For example, picking up an item may update inventory, entity override, maze snapshot, and last played timestamp together.

Recommendation: expose a gameplay-level `save_checkpoint(reason)` that writes the current coherent state rather than many individual write calls scattered through systems.

### Add save integrity and backup behavior to MVP

The open question about DB corruption is important enough to promote. At minimum:

- Run `PRAGMA integrity_check` on boot.
- If it fails, copy the DB to a timestamped backup.
- Start with a fresh DB only after explicit player confirmation.

Silent overwrite would be especially painful with 3-5 hour progression.

### Clarify asset coordinate conventions

The asset pipeline mentions render conventions. Add the specific metadata the engine needs:

- Sprite origin point.
- Foot point for y-sort.
- Collision shape.
- Shadow height scalar.
- Interaction anchor.
- Layer category.

This can live in a simple per-theme asset manifest. Without it, the engine will either hardcode assumptions or derive gameplay data from image dimensions.

### Plan for tests that do not require raylib

The raylib quarantine is good. Preserve it aggressively so most tests can run without a GL context. In addition to the listed tests, add:

- Entity behavior tests with fake `Game` state.
- Save migration tests.
- Localization parser malformed-file tests.
- Maze validator tests.

## Documentation Suggestions

### Add a glossary

Terms like item, tool, outfit, key, unlock, entity, hazard, blocker, maze-local, and profile-wide should have precise meanings. A short glossary will save implementation churn.

### Add explicit acceptance criteria for MVP

The MVP section currently describes content quantity and systems. Add shippable criteria such as:

- A new player can complete every maze without external instructions.
- Closing and reopening resumes correctly from every maze.
- Resetting a maze cannot remove profile-level unlocks.
- Every player-visible string routes through localization.
- No required puzzle can be made impossible by item misuse.

### Keep open questions action-oriented

Each open question should list the decision needed, owner if applicable, and default if no decision is made. For example:

```text
Undo last action
Decision needed: add action-log undo, avoid consumable soft-locks, or accept reset-only.
Recommended default: avoid consumable soft-locks for MVP.
```

## Suggested Spec Changes Before Implementation

1. Add a "First playable vertical slice" section before the full MVP scope.
2. Promote soft-lock prevention from open concern to design requirement.
3. Define item scopes: profile unlock, permanent tool, maze-local consumable.
4. Add stable text IDs to maze entities and save schema.
5. Specify interaction targeting, inventory use, and failed-use behavior.
6. Define player collision, trigger, and interaction shapes.
7. Add content validation tooling to the technical plan.
8. Add update-order and memory-ownership conventions.
9. Promote DB corruption fallback into save-system requirements.
10. Add concrete MVP acceptance criteria.

