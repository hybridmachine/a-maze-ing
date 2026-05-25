# A Maze Ing - Follow-up Design Suggestions

**Date:** 2026-05-25
**Review target:** `2026-05-25-a-maze-ing-design.md`
**Context:** Second-pass review after the original design was revised from `2026-05-25-a-maze-ing-design-suggestions.md`
**Status:** Follow-up suggestions before implementation

## Summary

The revised design addresses the major first-pass issues well. It now has a vertical slice, explicit interaction targeting, item scope, stable IDs, save integrity behavior, validation tooling, accessibility requirements, and concrete MVP acceptance criteria.

The remaining concerns are mostly second-order consistency issues. They are not reasons to block the design, but they are worth tightening before code starts because several of them affect schema shape, rendering architecture, save behavior, and validation claims.

## Priority Follow-ups

### 1. Make validator claims match actual validator power

The spec says the validator performs a simple graph walk over walkable tiles and then uses validator success as part of "soft-lock proof." A tile reachability pass will catch many data errors, but it cannot prove puzzle solvability once progress depends on inventory, gates, time of day, outfits, one-way state changes, and entity callbacks.

Recommendation:

- Keep the simple graph validator for structural checks.
- Add a later state-aware progression validator only if needed.
- Change the MVP acceptance wording from "validator proves soft-lock safety" to "validator catches structural soft-lock risks; completion is verified by playthrough tests."

This keeps the validator useful without overpromising what it can prove.

### 2. Fix the WAL shutdown acceptance criterion

The save system uses SQLite WAL mode, which normally creates `amazeing.db-wal` and `amazeing.db-shm` sidecar files. The current "Quiet exit" criterion says no `-wal` / `-shm` orphans persist. That is too strict and can conflict with normal SQLite behavior.

Recommendation:

- Replace "no `-wal`/`-shm` orphans persist" with a criterion like:
  - "Closing the app at any point checkpoints or cleanly commits all pending save data."
  - "On next launch, SQLite opens without recovery errors and the latest checkpoint is present."

If the project wants sidecar files removed on exit, specify an explicit `sqlite3_wal_checkpoint_v2` close path and test it, but do not make sidecar absence the core correctness measure.

### 3. Use one canonical source for profile-scope items and outfits

The schema has both `profile_items` and `outfits_owned`. Since outfits are profile-scope items, these two tables can diverge unless every write updates both perfectly.

Recommendation:

- Make `profile_items` the canonical ownership table.
- Define item metadata separately, with fields such as `kind: outfit | tool | access`.
- Derive outfit ownership from profile items whose item definition is `kind: outfit`.

If a separate `outfits_owned` table is kept for convenience, treat it as a cache that can be rebuilt, not as the source of truth.

### 4. Render gameplay and UI at different resolutions

The fixed 480x270 logical render texture is good for pixel-crisp world art. It is less good for accessibility, dialog readability, remappable-control UI, and inventory screens. UI text rendered into a low-resolution world texture may look chunky or cramped at larger window sizes.

Recommendation:

- Render the world to the fixed low-resolution texture.
- Upscale the world with nearest-neighbor.
- Render HUD, dialogs, menus, and settings at native/window resolution.
- Add a UI scale setting or at least define UI scale behavior for 720p, 1080p, and 4K windows.

This preserves the pixel-art look while keeping text and controls readable.

### 5. Relax or explain global stable ID uniqueness

The save schema keys entity overrides by `(profile_id, maze_id, stable_id)`, so stable IDs only need to be unique within a maze. The validator currently requires global uniqueness across mazes, which is stricter than the schema requires.

Recommendation:

- Either require uniqueness only within each maze, or
- Require globally unique IDs by convention and state that stable IDs must include the maze prefix.

The examples already use maze-prefixed stable IDs, so the second option is fine. The important part is making the rule intentional rather than accidental.

### 6. Avoid confusing the vertical slice with final progression

The full MVP says Snow/Ice requires the winter coat. The vertical slice says Nature includes or pre-grants the winter coat. That is fine for testing, but it can muddy the real progression model if left ambiguous.

Recommendation:

- Use a slice-only outfit or tool gate, such as `rain_boots` or `garden_gloves`, or
- Mark the winter coat as pre-granted test data for the slice only.

This keeps the slice focused on proving mechanics without implying final content placement.

### 7. Define timestep behavior

The update pipeline is deterministic in order, but not necessarily deterministic in simulation if callbacks receive variable `dt`.

Recommendation:

- Choose a fixed 60 Hz simulation tick for player movement, hazards, interactions, time gates, and entity callbacks, or
- Use variable `dt` with a clamp and explicitly state that tests pass controlled `dt` values.

For a puzzle game, fixed-step simulation is usually simpler to reason about and easier to test.

## Secondary Suggestions

### Clarify item definitions

The maze file declares item IDs and scopes, but the spec does not yet define where item metadata lives. Add an `items.txt` or equivalent manifest for:

- Item ID.
- Display name localization key.
- Description localization key.
- Scope.
- Kind (`outfit`, `tool`, `consumable`, `access`).
- Optional icon asset.
- Optional use behavior.

This keeps item behavior out of ad hoc entity lines and gives inventory UI a reliable data source.

### Add a dialog data format

The spec references NPC dialog keys, text speed, and hint escalation, but does not define dialog data. Add a minimal format before building UI:

- Dialog ID.
- One or more localized text keys.
- Optional speaker name key.
- Optional post-dialog state change.
- Optional repeat behavior.

MVP can stay simple, but dialog should be data-driven from the start.

### Define save reset semantics precisely

The spec says maze reset clears maze-scoped items and preserves profile-scoped items. It should also state what happens to:

- Maze snapshot.
- Entity overrides.
- Clock time.
- NPC state.
- Time-gated object state.
- Current outfit worn.
- Maze progress state.

A reset checklist will prevent partial reset bugs.

### Add input conflict rules

Remappable controls are now MVP. Add binding validation rules:

- Whether multiple actions may share a key.
- Which actions are reserved.
- How escape/back works if remapped.
- How the game recovers from an unusable binding set.

This is easier to define before the settings UI exists.

### Clarify asset manifest ownership

The asset manifest includes collision and proximity shapes. That is useful, but some gameplay shapes may be maze-specific rather than sprite-specific. For example, the same gate sprite might have different interaction ranges in different mazes.

Recommendation:

- Let asset manifests provide defaults.
- Let maze entity lines override collision, proximity, shadow height, and interaction anchor when needed.

## Suggested Spec Edits Before Implementation

1. Reword validator and MVP "soft-lock proof" language to avoid implying full formal solvability proof.
2. Replace the `-wal` / `-shm` shutdown criterion with a committed-data and clean-reopen criterion.
3. Collapse outfit ownership into `profile_items`, or document `outfits_owned` as derived/cache state.
4. Split world rendering and UI rendering resolutions.
5. Make stable ID uniqueness rules match the schema, or explicitly require globally maze-prefixed stable IDs.
6. Make the vertical-slice gate item test-only or use a local slice-specific outfit/tool.
7. Add a fixed-step or clamped-`dt` timestep policy.
8. Add item definition and dialog data formats.
9. Add exact maze reset semantics.
10. Add remappable-input conflict and recovery rules.

