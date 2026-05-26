# Vertical Slice Plan - Review Notes

Review of `2026-05-25-a-maze-ing-vertical-slice.md` against
`docs/superpowers/specs/2026-05-25-a-maze-ing-design.md`.

Status after updates: no blocking review items remain before Phase A. The plan has been updated to
fold the actionable notes below into the relevant tasks.

## Resolved Before Execution

### 1. Mud hazard coverage in the slice

Resolved in Task 26 and Task 49.

The slice now includes a required `hazard#nature.mud.patch` entity with `hazard:mud`,
`needs:rain_boots`, and a localized hint. Task 26 defines deep-mud semantics: without the needed
outfit, the player is reverted to `prev_pos`; with the outfit, traversal is allowed and the mud slow
effect still applies.

### 2. ESC double-bound to quit and pause

Resolved in Task 21.

The main loop calls `SetExitKey(0)` immediately after `InitWindow`, so ESC remains owned by the
input layer as `ACT_PAUSE`.

### 3. `auto_wear_first` compile gap

Resolved in Task 20, Task 28, and Task 30.

`Game` now includes `bool auto_wear_first`, initialized true by `game_create`. Outfit pickups and
dialog `give_item` post-states use it to equip newly-earned outfits immediately.

### 4. Profile-item re-pickup after reset

Resolved in Task 15.

`save_set_profile_item` is specified as an idempotent upsert using
`ON CONFLICT(profile_id, item_id) DO NOTHING`, and `save_set_outfit_worn` is specified as an upsert
by `profile_id`.

### 5. Validator reachability

Resolved in Task 51.

Reachability is now a fixed-point BFS: each pass opens any reachable door/blocker whose `needs:`
item has become reachable, then re-runs BFS until no new blockers open.

## Smaller Gaps Resolved

- CMake test wiring convention added after Task 4: every later test task must append sources to
  `test_runner` and register its suite in `tests/test_main.c`.
- Task 29 now names extras storage for doors, signs, and sundials, including load/unload ownership.
- Task 15 now verifies inventory counts after `ON CONFLICT` accumulation.
- Task 12 now specifies shape-override token grammar and tests it in the fixture.
- Task 52 now includes a short scripted smoke path after idle ticks.
- Task 36 now uses integer nearest-neighbor scaling with centered letterboxing.
- Task 32 now requires `e->cb` and `e->cb->on_time_changed` null checks.
- Task 39 now specifies dusk-overlay tint anchors and interpolation.
- Task 45 now stores picker target state explicitly in `Game`.

## Keep As-Is

- Phase ordering still builds dependencies cleanly.
- The concrete `Game` struct remains intentional for tests and headless tools.
- The validator and smoke-test tasks now have explicit stubs/registries for non-rendering workflows.
- The acceptance checklist remains a manual playthrough artifact, not a replacement for unit or smoke
  coverage.
