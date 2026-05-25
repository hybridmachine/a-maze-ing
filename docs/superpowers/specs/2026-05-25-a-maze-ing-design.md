# A Maze Ing — Design Spec

**Date:** 2026-05-25
**Status:** Draft, revised after review (see `2026-05-25-a-maze-ing-design-suggestions.md`)
**Target platforms:** macOS, Windows, Linux
**Language:** C (C11)
**Engine:** raylib 5.x

## 1. Overview

A Maze Ing is a relaxing top-down 2D puzzle game. The player explores themed maze-worlds — one larger-than-screen scrolling world per theme — collecting items and using them to unlock new areas, change outfits, and progress to the maze's goal. There are no enemies, no death, and no time pressure. Mistakes are always reversible.

Visual style is pre-rendered isometric (Theme Hospital / Diablo aesthetic), with dynamic sun-driven shadows, parallax wall backdrops, and pixel-crisp upscaling. Audio is a layered ambient soundscape per theme — no melodic music. The game ships English-only at MVP with full localization infrastructure (drop in a `.lang` file to add a language).

The MVP is four to six hand-crafted themed mazes (Nature, Circuit Board, Village, Theme Park, Snow/Ice). Each maze is ~30–60 minutes of play. Full game is ~3–5 hours.

## 2. Game design

### 2.1 Core loop

The minute-to-minute player experience:

1. **Explore** until blocked or intrigued.
2. **Find** an item, outfit, clue, or NPC.
3. **Use** that discovery to open a route or change the maze state.
4. **Reach** a new landmark and repeat.
5. **Unlock** the goal path.

Every subsystem exists to serve this loop. Playtest evaluations are made against it.

### 2.2 Core mechanic — collect-and-use

The maze is a puzzle of locks and keys. "Locks" can be locked doors, hazards that block paths, sleeping NPCs, time-gated objects, and so on. "Keys" are items the player collects: literal keys, outfits (a winter coat unlocks the icy area), tools (a watering can revives a withered plant), tickets, seeds, lanterns, etc. The player wanders, picks up items, uses them on the right targets, and the maze gradually opens up until the goal is reachable.

### 2.3 Failure model and soft-lock prevention

There is no death, no game-over, and no enemies. Hazards are environmental puzzles, not threats:

- Ice tiles slide the player in their direction of motion until they hit something stoppable.
- Mud tiles slow movement.
- Water tiles block movement unless the player is wearing waders.
- Thorn tiles block movement unless the player has gardening gloves.

**Soft-lock prevention is an MVP requirement, not an open question.** A relaxing game cannot trap the player into a forced reset. The design enforces this through four rules, in order of preference:

1. **Prefer non-consumable tools** for critical progression. A watering can is reusable; a single-use seed is not.
2. **For consumables, guarantee replenishment** somewhere reachable from the player's current state.
3. **If a consumable can be misapplied, the target rejects it instead of consuming it.** Using a key on the wrong door produces a hint, not a lost key.
4. **Whole-maze reset is reserved for explicit player choice**, not routine puzzle recovery.

These rules are enforced during maze authoring; the validator (§5) checks the third rule structurally where possible.

Reset preserves outfits and items at profile scope (see §2.7) — resetting the ice maze never removes the winter coat the player earned in the village maze.

**Exact reset semantics.** When a player resets a maze, the engine performs the following operations under one save transaction, in order:

| State | Action on reset |
|---|---|
| `maze_inventory` for this maze | Cleared |
| `entity_overrides` for this maze | Cleared |
| `maze_snapshot` for this maze | Deleted (player respawns at the maze's `start` position) |
| Clock | Reset to the maze's `time_start` value |
| NPC dialog state and `repeat` counters | Reset to initial |
| Time-gated entity state | Recomputed from clock and current time gates |
| Player's currently-worn outfit | Preserved (profile-level) |
| `profile_items` and `outfit_worn` | Preserved |
| `maze_progress.state` for this maze | Reset to `in_progress` (or to `locked` if this maze required a profile item the player no longer has — guarded by save migration tests) |
| `maze_progress` for other mazes | Untouched |

The reset path is its own typed function `save_reset_maze(profile_id, maze_id)`; gameplay calls it once and never assembles the operations ad-hoc.

### 2.4 Movement and collision shapes

Free 8-direction movement at pixel granularity. The character walks with an animated cycle (arm swing, leg motion) and has a separate face layer for expressions (happy, surprised, cold, curious). Movement speed is constant; no sprinting.

Because pixel-granular movement on an isometric grid creates ambiguity between visual bounds, walkable space, and interaction space, collision is **shape-based** and decoupled from art bounds:

- **Player foot shape** — a small rectangle (or capsule) at the character's feet. This is what blocks against walls and triggers hazards.
- **Entity collision shape** — each blocking entity carries an explicit collision shape independent from its sprite bounds. A tree's canopy is decorative; the trunk is the blocker.
- **Hazard regions** — overlap-tested against the foot shape; they can overlap walkable ground (you walk through fog without colliding with it).
- **Interaction proximity** — each interactable has a separate proximity shape, used for the "press to interact" prompt and for nearest-eligible-target selection.

These four shape categories let visuals, walkable space, and interaction space disagree intentionally without bugs.

### 2.5 Interaction model

One primary action key drives all interactions.

When the player presses the action key:

1. The game scans interactables whose proximity shape overlaps the player's foot shape, plus those within a short facing-cone.
2. If exactly one is eligible, it becomes the target.
3. If multiple are eligible, the nearest along the facing direction wins; ties break by smaller entity id.
4. The target is queried for what items (if any) from the inventory apply.
5. **No items apply** — the target shows a localized hint (`hint:` key in its data) explaining what would help.
6. **One item applies** — the game uses it automatically.
7. **Multiple items apply** — a compact inventory picker appears, showing the eligible items only.

Failed uses do **not** consume the item (rule 3 in §2.3). Repeated failed interactions on the same target may escalate the hint text (`hint.2`, `hint.3` keys).

A separate inventory key opens the full inventory screen for browsing.

### 2.6 Maze structure and relationships

Each maze is one scrolling world bigger than the screen. The camera follows the player with a soft deadzone and smoothing. No loading screens within a maze.

**Maze relationships are light unlock ordering, not metroidvania.** Mazes are mostly independent and self-contained — each is solvable using items collected inside it. A small number of profile-scope unlocks (outfits, permanent tools) gate access to specific later mazes, but the player never has to backtrack into a completed maze to make progress. The Maze Select hub shows lock/unlock state per maze.

### 2.7 Item scope

Items have one of two scopes, declared in the entity definition:

- **`profile`** — outfits, permanent tools, maze-access items. Persist across all mazes for the active profile. Picked up once.
- **`maze`** — keys, tickets, seeds, lanterns, single-maze consumables. Cleared on maze reset; do not transfer to other mazes.

The save schema reflects this split (`outfits_owned` and a future `profile_items` are profile-scoped; `inventory` rows for maze-scoped items are keyed by maze).

### 2.8 Scope — 4–6 mazes

Each themed world is one ~30–60 minute experience. The full game is ~3–5 hours. All mazes are hand-crafted; nothing is procedurally generated.

Themes for MVP:

1. **Nature** — forest, glades, ponds, dawn light.
2. **Circuit Board** — pulsing traces, capacitor pillars, LED lakes.
3. **Village** — half-timber houses, a market square, a shopkeeper.
4. **Theme Park** — rides, ticket booths, carnival games.
5. **Snow/Ice** — frozen pond, snow drifts, requires the winter coat.

### 2.9 Time of day

A single `Clock` advances at ~1 game-hour per 3 real-minutes (~70 minutes of play per full day-night cycle). The clock pauses during dialogs, the Pause Menu, and the Inventory screen.

Time gates follow two rules to avoid waiting puzzles:

- **Required time gates** always have a nearby time-advance affordance (bench, campfire, rest-spot) and a clear localized hint pointing at it. Time-advance is a player action, not a passive wait.
- **Optional/secret time gates** (fireflies for a hidden cosmetic) may rely on observation only.

Most of the world is not gated; time is primarily atmospheric.

### 2.10 Saves — auto-save with multiple profiles

Up to four named profiles. The active profile auto-saves at coherent checkpoints — picking up an item, transitioning areas, completing a dialog, rolling past midnight, opening the Pause Menu, quitting. Auto-save is **transactional and debounced**: each checkpoint is one SQLite transaction wrapping all related writes (see §3.9 for the API surface).

Players close the app and resume exactly where they were. Save data lives in an SQLite database in the OS user-data dir (§3.9).

### 2.11 Hinting

Because there are no enemies, timers, or fail states, confusion is the main source of player friction. The hint policy:

- **Signs and NPCs** hint at nearby blockers and required items.
- **Failed item use** shows a short hint explaining what would help instead.
- **The inventory** exposes short item descriptions and (where useful) where the item was found.
- **Escalation** — repeated failed interactions on the same target may surface progressively more specific hint text.

All hint text routes through localization keys. No separate hint system is needed at MVP.

### 2.12 Audio — ambient soundscape

No melodic music. Each theme owns four layers:

- **Base** — always-on ambient bed (forest birds + wind, circuit hum + beeps, etc.).
- **Texture** — zone-specific (creek by water, market chatter by vendor).
- **Time-of-day** — swaps day → dusk → night (crickets, owls, chimes).
- **One-shots** — pickups, footsteps, dialog beeps, hourly bells.

Crossfades on zone change take ~1–2 seconds.

### 2.13 Protagonist

A single fixed character. Identity is fixed; appearance changes by outfit (winter coat, raincoat, gardener apron, etc.). Outfits are unlocked by collecting them in mazes. Some mazes require a specific outfit to enter.

The character is composited from two sprite layers at draw time: outfit (walk cycle) and face (expression). One walk cycle is rendered per outfit; faces are overlaid on top. This avoids combinatorial sprite explosion.

### 2.14 Localization

All player-visible text routes through a single lookup function `t(key)`. Strings live in plain-text `.lang` files in `data/strings/`. MVP ships with English only; adding a language is dropping a new file. UTF-8 throughout. Default language at first launch matches the OS locale if a matching `.lang` file exists, otherwise English.

RTL languages and pluralization rules are out of scope for MVP.

### 2.15 Accessibility

The following are MVP requirements, not stretch goals:

- **Remappable keyboard controls.** All actions go through a remap layer; bindings persist per profile.
- **Text speed setting** (slow / normal / instant), per profile.
- **Independent volume sliders** for master, ambience, and SFX.
- **No screenshake** in MVP. (If added later, default off.)
- **High-contrast interaction prompts** by default.
- **Color-independent puzzle cues.** Especially relevant for the Circuit Board theme — anywhere a color carries meaning, pair it with a shape, icon, or label.

Controller support remains out of scope for MVP but the input architecture should not preclude it (action mapping is already decoupled from raw key codes).

**Binding rules and recovery:**

- **Each key binds to at most one action.** Assigning a key already in use prompts a single "swap or cancel" confirmation; on swap, the previously bound action becomes unbound.
- **Reserved keys.** `Escape` is always bound to "back / pause" and cannot be remapped. `Enter` defaults to "confirm" in menus and is unbindable from menus. (Both can be supplemented with secondary bindings that are user-set.)
- **Required actions.** A binding set must include bindings for: move (4 directions), action, inventory, pause. Any missing required binding is flagged before the player can leave the Settings menu.
- **Reset to defaults.** Settings always offers a one-click "Reset bindings to defaults" option.
- **Recovery from a broken binding set.** If the persisted binding set is missing required actions on load (e.g., from a botched migration or a corrupted file), the input layer logs the issue and silently falls back to the defaults; the Settings menu opens with a non-blocking notice explaining the reset.

## 3. Technical architecture

### 3.1 Module layout

Flat C modules, one `.c/.h` pair per subsystem. All state is passed explicitly via a single top-level `Game *g`; no globals.

| Module | Responsibility |
|---|---|
| `main` | Window init, raylib lifecycle, top-level loop |
| `game` | `Game` struct, state-machine dispatch (title / profile / maze-select / in-maze / pause / inventory / complete) |
| `input` | Keyboard reads, action mapping, remapping |
| `player` | Position, velocity, animation state, outfit |
| `world` | Tile grid, entity arrays, maze load/unload |
| `entity` | Entity tagged union + behavior/event registry |
| `collision` | Shape-based walkability, hazard, and interaction queries |
| `interaction` | Target selection, eligibility checking, item-use dispatch |
| `render` | Iso projection, depth sort, all draw passes |
| `camera` | Follow-cam with deadzone and smoothing |
| `shadow` | Sun vector, per-entity shadow placement |
| `time_of_day` | Clock, time-gate checks, time-advance affordances |
| `parallax` | Faux-3D wall backdrop layers |
| `audio` | Layered ambient mixer + one-shots |
| `asset` | Texture/sound/font loader and cache |
| `save` | SQLite-backed profile + game-state persistence |
| `profile` | Profile list and active-profile state |
| `ui` | Title, HUD, dialogs, inventory, settings, accessibility menus |
| `localization` | `t(key)` lookup, `.lang` parser, language switching |
| `maze_data` | Maze file load/parse, validator hooks |
| `util` | Math helpers, logging, OS path resolution |

### 3.2 Dependency rules

- **One-way deps.** `render`, `audio`, `save`, `ui` read from `world`/`player`; never the reverse.
- **`raylib` quarantined.** Only `main`, `render`, `audio`, `input`, `asset` include `<raylib.h>`. Game logic stays portable and testable.
- **Asset cache lives in `asset`.** Other modules ask for textures and sounds by string key; `asset` deduplicates and reference-counts.
- **No globals.** A single `Game *g` is threaded through. Each subsystem takes only the parts it needs.
- **File-size guardrail.** Aim for under ~500 lines per `.c`. Split by category if a module grows past that (e.g., `entity_item.c`, `entity_npc.c`, `entity_hazard.c`).

### 3.3 Update pipeline (fixed-step, deterministic)

**Simulation runs at a fixed 60 Hz tick** (16.6 ms). Rendering runs at the display's refresh rate. The main loop uses a fixed-step accumulator: each frame, it adds elapsed wall time to an accumulator and runs as many 60 Hz ticks as fit (clamped to at most 5 ticks per frame to avoid spirals after long pauses). Fixed-step makes player movement, hazards, time gates, and entity callbacks reproducible — invaluable for tests and replay-style debugging.

Tick order (one tick):

1. **Poll input** — gather actions, never raw key codes, from `input`.
2. **Update UI or gameplay state** — pause/resume/menu transitions resolve before world simulation.
3. **Resolve player movement** — apply velocity, slide against blocking shapes, snap-out of overlaps.
4. **Apply hazards/triggers** — hazard regions act on the player's new position; triggers fire `on_player_enter`/`on_player_exit`.
5. **Run entity step callbacks** — NPCs patrol, animated decor advances frames, time-gated visibility recomputes. Callbacks receive the fixed tick `dt` of 1/60 s.
6. **Advance clock if unpaused.**
7. **Queue save events** — gameplay code calls `save_checkpoint(reason)` at coherent moments; no scattered individual writes.
8. **Update audio targets** — recompute mixer targets from zone and time-of-day state.

Rendering happens after the tick (or batch of ticks) on each frame, with optional sprite-position interpolation between the previous and current tick states if subtick smoothness is needed for high-refresh displays.

### 3.4 Memory ownership conventions

- **Maze data** is owned by `world`. Loaded by `maze_data` into freshly allocated buffers, freed when the maze unloads.
- **Entity arrays** are owned by `world`. Stable for the lifetime of the maze — entities are never moved, removed entries are tombstoned. Pointers into the array remain valid until the maze unloads.
- **Asset cache** uses string-keyed lookup with reference counting. Modules call `asset_acquire` / `asset_release`. Cache evicts entries with refcount 0 only on explicit `asset_gc()` (called at maze unload).
- **Strings.** Localization values are owned by `localization` and live for the duration of the active language. Callers must not free them; copy if the string must outlive a language switch.
- **Errors.** Return `bool` (or a small `Result` enum) from any operation that can fail. Detailed error text logged via `util_log`; never `exit()` from a subsystem. The top-level loop handles fatal errors by surfacing a player-facing message via `ui`.

### 3.5 Entity model

Entities are a tagged union with a small set of event callbacks resolved by a registry at load time.

```c
typedef enum {
    ENT_ITEM,         // pickable: seed, coat, ticket, key
    ENT_HAZARD,       // region: water, ice, mud, thorns
    ENT_INTERACT,     // switch, door, sundial, signpost
    ENT_NPC,          // shopkeeper, child, animal
    ENT_DECOR,        // visual-only, depth-sorted
} EntityKind;

typedef enum { SCOPE_MAZE, SCOPE_PROFILE } ItemScope;

typedef struct EntityCallbacks {
    void (*on_loaded)        (Game*, Entity*);              // once at maze load
    void (*on_player_enter)  (Game*, Entity*);              // foot enters proximity
    void (*on_player_exit)   (Game*, Entity*);              // foot leaves proximity
    void (*on_interact)      (Game*, Entity*, ItemId);      // ItemId=0 if none
    void (*on_step)          (Game*, Entity*, float dt);    // per tick (NPC patrol, anim)
    void (*on_time_changed)  (Game*, Entity*, int hour);    // gate edges, day/night
} EntityCallbacks;

typedef struct Entity {
    EntityKind kind;
    uint32_t   id;               // engine-internal, stable within a maze run
    char       stable_id[48];    // designer-assigned, persists across saves (§3.6)
    Vector2    pos;
    Shape      collide_shape;    // blocking; may be zero-size
    Shape      proximity_shape;  // for interaction
    int        sprite_id;
    int        shadow_id;        // -1 = no shadow
    uint32_t   flags;
    int8_t     gate_hour_a;      // -1 = always active
    int8_t     gate_hour_b;
    EntityCallbacks *cb;         // points into registry; never freed
    union {
        struct { ItemId item_id; ItemScope scope; bool taken; }   item;
        struct { int    type;    int  param;  }                   hazard;
        struct { int    target;  bool active; }                   interact;
        struct { int    dialog_id; int  state; }                  npc;
    };
} Entity;
```

Not every callback has to exist on every entity; NULL means "ignore." The registry is a small table mapping behavior names to `EntityCallbacks` records.

### 3.6 Maze data format — plain text with stable IDs

Each maze is a single file at `data/mazes/<theme>.maze`. Header is `key: value` lines. The `tiles:` block is a literal ASCII map; each character maps to a sprite via the theme's tileset definition. The `entities:` block is one entity per line.

**Entity lines carry an explicit `behavior#stable_id` prefix.** The stable id is a designer-assigned text string that persists across saves and survives reordering or insertion of other entities. The engine-internal numeric id is *not* used in saves.

**Stable ID convention.** The save schema requires uniqueness only within a maze (`(profile_id, maze_id, stable_id)` is the PK). By **convention**, however, stable IDs are required to begin with the maze's id as a prefix — e.g., `nature.seed.west_glade`, `circuit.gate.capacitor_b`. This gives global uniqueness for free, makes debugging traces and SQL queries readable, and means stable IDs work as universal references in tooling. The validator enforces both within-maze uniqueness (mandatory) and the maze-prefix convention (warning by default, error in CI).

```
name:       maze.nature.name
theme:      nature
size:       64x48
start:      8,4
time_start: 08:00
ambient:    birds_loop.ogg, wind_soft.ogg

tiles:
GGGGGGGGGGGGGGGGGGGG...
GG..WWW......TTT....
...

entities:
seed#nature.seed.west_glade        12 8   pickup item:seed         scope:maze
sundial#nature.sundial.main        20 15  gate:12-13
shopkeeper#nature.npc.shopkeeper   30 22  gate:06-22 dialog:npc.shopkeeper.greet
ice_patch#nature.hazard.pond_a     40 30  hazard:ice
locked_gate#nature.gate.ice_path   50 18  needs:winter_coat target:area_b
beaver#nature.npc.beaver_a         28 19  npc patrol:28,19-32,19
fireflies#nature.decor.glade       18 12  gate:19-23
winter_coat#nature.item.coat       16 30  pickup item:winter_coat  scope:profile

audio_zones:
creek        12-18 30-36  texture:water_creek.ogg
market       28-36 20-26  texture:village_chatter.ogg
```

`audio_zones:` declares rectangular regions in tile coords. When the player enters a zone, the audio mixer crossfades the named clip into the active texture layer; on exit, it crossfades back to silence. Zones may overlap; the topmost listed zone wins.

**Tiles vs entities split.** The tile grid only describes walkable ground. Anything with vertical extent (a fountain, a tree canopy, a sign) goes in `entities:` as a sprite with a shadow.

**Localization keys.** Maze names, NPC dialog references, item display names, and hint strings use localization keys (e.g., `maze.nature.name`), not English prose. Translators only touch `data/strings/`.

**Parser.** ~250 lines, hand-rolled, no JSON dependency. Diff-friendly. A future visual editor (e.g., Tiled `.tmx`) can be added by extending the loader; not needed for MVP.

### 3.7 Rendering pipeline

Rendering splits into two resolutions to keep the pixel-art world crisp while keeping UI text readable on modern displays:

- **World layer** renders to a fixed-size logical render texture (e.g., 480×270), then upscales with **nearest-neighbor** to the window. This is where the pixel-art look lives.
- **UI layer** (HUD, dialogs, menus, settings, inventory, interaction prompts) renders at the **native window resolution** with smooth text. UI scale is set by a Settings option (`auto` / `1x` / `1.5x` / `2x` / `3x`); `auto` picks a sensible multiplier based on window height (e.g., 1x at 720p, 1.5x at 1080p, 3x at 4K).

raylib's `BeginTextureMode` / `DrawTexturePro` handles the world upscale; UI is drawn in standard screen space after the upscale.

Per-frame draw order:

1. Sky / gradient (world layer) — tinted by time of day.
2. Far parallax (world layer) — distant hills/walls.
3. Near parallax (world layer) — closer wall layer.
4. Ground tiles (world layer) — iso, no z-sort (always below entities).
5. Shadows pass (world layer) — all shadows drawn under all sprites (so a shadow never occludes another sprite).
6. Entities (world layer) — y-sorted by foot point.
7. Player (world layer) — interleaved into the entity y-sort.
8. Light/dusk overlay (world layer) — multiplicative tint over the playfield.
9. **Upscale world texture to window.**
10. HUD and clock (UI layer) — native resolution.
11. Dialogs / menus / inventory (UI layer) — native resolution, drawn over HUD.
12. Interaction prompts (UI layer) — positioned in screen space using the target entity's UI-projected anchor.

**Iso projection.** `sx = (tx - ty) * TW/2; sy = (tx + ty) * TH/2`. Tile dimensions are nominally 64×32 (2:1 dimetric).

**Y-sort.** Visible entities are sorted by **foot point**, not sprite center — the foot point is declared in the asset manifest (§4). `qsort` each frame is fine at hundreds of entities.

**Dynamic shadows.** `time_of_day` exposes a sun vector (2D direction + length scalar + elevation alpha) derived from the current hour. `shadow.c` reads it once per frame; each shadow draws with offset = `sun.dir * sun.length * entity.height`, Y-scale modulated by `sun.length`, alpha by `sun.elevation`. The shadow-height scalar comes from the asset manifest. ~10 lines of code, no real 3D math, reads as 3D.

**Parallax walls.** Two sprite layers offset by camera position × per-layer factor (0.3 far, 0.7 near). The player cannot walk behind them; they're a faux-3D backdrop on the camera edge.

### 3.8 Audio mixer

Built on raylib's `Music` (for streaming ambient OGG layers) and `Sound` (for one-shots). `audio.c` keeps a small mixer table per active layer with current volume and target volume. Each frame, volumes lerp toward target; layer position is shared across layers (so the loop boundary stays synchronized).

Layer table is reconfigured on:

- Maze entry/exit (full set swap).
- Zone enter/leave (audio-zone tag in the maze file activates the texture layer).
- Time-of-day transitions (day/dusk/night layer set).

### 3.9 Save system — SQLite

One database file (`amazeing.db`) holds all profiles. WAL journal mode, single-threaded build (`SQLITE_THREADSAFE=0`), load-extension disabled.

**Location:**

- macOS: `~/Library/Application Support/AMazeIng/amazeing.db`
- Windows: `%APPDATA%\AMazeIng\amazeing.db`
- Linux: `$XDG_DATA_HOME/AMazeIng/amazeing.db` (fallback `~/.local/share/AMazeIng/`)

A small `util_paths.c` resolves the per-OS path at boot.

**Integrity and backup are MVP requirements.** On open:

1. Run `PRAGMA integrity_check`.
2. If it fails, copy the DB (and any `-wal`/`-shm` siblings) to a timestamped backup path next to the original.
3. Surface a player-facing dialog explaining the situation; never silently overwrite.
4. Start with a fresh DB **only after explicit player confirmation**.

**Schema (initial). All foreign keys are text where possible** for debuggability:

```sql
PRAGMA user_version = 1;
PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;

CREATE TABLE profiles (
  id           INTEGER PRIMARY KEY,
  name         TEXT NOT NULL UNIQUE,
  created_at   INTEGER NOT NULL,
  last_played  INTEGER NOT NULL
);

CREATE TABLE settings (
  profile_id  INTEGER REFERENCES profiles(id) ON DELETE CASCADE,
  key         TEXT NOT NULL,
  value       TEXT NOT NULL,
  PRIMARY KEY (profile_id, key)
);

-- Profile-scope ownership table. Single source of truth for outfits,
-- permanent tools, maze-access items.  Item kind is looked up from the
-- item definition manifest (see §4.2), not stored here, to avoid drift.
CREATE TABLE profile_items (
  profile_id  INTEGER REFERENCES profiles(id) ON DELETE CASCADE,
  item_id     TEXT NOT NULL,     -- "winter_coat", "watering_can"
  acquired_at INTEGER NOT NULL,
  PRIMARY KEY (profile_id, item_id)
);

-- Currently-worn outfit per profile. References an item_id whose
-- definition has kind:outfit. This is the only mutable outfit state;
-- ownership lives in profile_items.
CREATE TABLE outfit_worn (
  profile_id  INTEGER PRIMARY KEY REFERENCES profiles(id) ON DELETE CASCADE,
  outfit_id   TEXT NOT NULL
);

-- Maze-scope: keys, tickets, seeds, single-maze consumables.
CREATE TABLE maze_inventory (
  profile_id  INTEGER REFERENCES profiles(id) ON DELETE CASCADE,
  maze_id     TEXT NOT NULL,     -- "nature", "circuit", ...
  item_id     TEXT NOT NULL,
  count       INTEGER NOT NULL DEFAULT 1,
  acquired_at INTEGER NOT NULL,
  PRIMARY KEY (profile_id, maze_id, item_id)
);

CREATE TABLE maze_progress (
  profile_id   INTEGER REFERENCES profiles(id) ON DELETE CASCADE,
  maze_id      TEXT NOT NULL,
  state        TEXT NOT NULL,    -- 'locked' | 'in_progress' | 'complete'
  completed_at INTEGER,
  PRIMARY KEY (profile_id, maze_id)
);

CREATE TABLE maze_snapshot (
  profile_id   INTEGER,
  maze_id      TEXT,
  player_x     REAL NOT NULL,
  player_y     REAL NOT NULL,
  facing       INTEGER NOT NULL,
  clock_min    INTEGER NOT NULL,
  outfit_worn  TEXT NOT NULL,
  PRIMARY KEY (profile_id, maze_id),
  FOREIGN KEY (profile_id) REFERENCES profiles(id) ON DELETE CASCADE
);

-- Keyed by stable_id from the maze file, not engine-internal numeric id.
CREATE TABLE entity_overrides (
  profile_id  INTEGER,
  maze_id     TEXT,
  stable_id   TEXT NOT NULL,
  taken       INTEGER NOT NULL DEFAULT 0,
  active      INTEGER NOT NULL DEFAULT 0,
  state_blob  BLOB,              -- escape hatch for kind-specific extras
  PRIMARY KEY (profile_id, maze_id, stable_id),
  FOREIGN KEY (profile_id) REFERENCES profiles(id) ON DELETE CASCADE
);
```

**API.** `save.c` exposes a high-level coherent surface:

- `save_open(path)`, `save_close()`, `save_integrity_check()`, `save_backup(reason)`.
- `save_checkpoint(reason)` — called by gameplay code at coherent moments. Internally opens one transaction, writes all dirty subsystem state, commits. Reasons are enums: `CKPT_ITEM_PICKED`, `CKPT_AREA_TRANSITION`, `CKPT_DIALOG_DONE`, `CKPT_DAY_ROLLED`, `CKPT_PAUSE`, `CKPT_QUIT`.
- Typed mutator helpers used by `save_checkpoint` internally — never called directly by gameplay code: `save_set_inventory`, `save_set_entity_override`, `save_set_clock`, `save_set_progress`.
- Read helpers: `save_load_maze_snapshot`, `save_load_inventory`, etc.

Gameplay code never sees SQL.

**Migrations.** On open, read `PRAGMA user_version`; if older than the binary's expected version, run a sequence of embedded SQL migration scripts under one transaction, then bump the version. Migrations are tested (§7).

### 3.10 Localization

`localization.c` keeps an open-addressed hashmap from string key to UTF-8 value. On `loc_set_language("xx")`:

1. Open `data/strings/xx.lang`, parse key-value lines (`#` comments, `=` separator, trimmed values).
2. Rebuild the hashmap from the parsed entries.
3. The English file (`en.lang`) is always loaded as a fallback layer.

`t(key)` returns the active-language string if present, else the English string, else the key itself. **Never returns NULL** — UI code can always draw something.

On first launch, the loader inspects `getenv("LANG")` / `setlocale` / Windows `GetUserDefaultLocaleName` (per OS) and picks the matching `.lang` file if it exists, else `en`. Player's choice is then stored in the `settings` table and used from that point on.

Bundled font (Inter or Noto Sans, OFL-licensed) is baked at boot via `LoadFontEx` with a Latin Extended codepoint set. Codepoints outside the bundled font's coverage render as the font's "missing glyph" box — the game never crashes on a partially-translated `.lang` file or unexpected codepoint.

## 4. Asset pipeline and data manifests

### 4.1 Sources and outputs

- Source art lives in `art_src/` (Blender files, render scripts). Not shipped with the game.
- Final assets are PNG sprite sheets in `assets/themes/<theme>/`, `assets/character/outfits/`, `assets/character/faces/`, `assets/ui/`.
- Audio: OGG Vorbis throughout, mono for one-shots, stereo for ambient layers. Mastered to consistent loudness so the mixer can blend without per-clip gain compensation.
- No build-time baking. Game loads assets at runtime through `asset.c`.

### 4.2 Item definitions

Item behavior and metadata live in `data/items.txt`, not inside maze entity lines. The inventory UI, save schema, and validator all consume this single source.

```
item:        winter_coat
name:        item.winter_coat.name
desc:        item.winter_coat.desc
scope:       profile
kind:        outfit
icon:        ui/icons/winter_coat.png

item:        watering_can
name:        item.watering_can.name
desc:        item.watering_can.desc
scope:       profile
kind:        tool
icon:        ui/icons/watering_can.png
use_behavior: water_plant         # named in entity registry

item:        seed
name:        item.seed.name
desc:        item.seed.desc
scope:       maze
kind:        consumable
icon:        ui/icons/seed.png

item:        nature_ticket
name:        item.nature_ticket.name
scope:       maze
kind:        access
icon:        ui/icons/ticket.png
```

`kind` is one of `outfit | tool | consumable | access`. The inventory UI groups items by kind; save tables key by `item_id`. Maze entity lines reference items by `item:` and never duplicate their metadata.

### 4.3 Dialog definitions

NPC dialogs live in `data/dialogs/<theme>.txt`, also data-driven:

```
dialog:      npc.shopkeeper.greet
speaker:     npc.shopkeeper.name
lines:
  npc.shopkeeper.greet.l1
  npc.shopkeeper.greet.l2
repeat:      once_per_day
post_state:  give_item:winter_coat   # optional

dialog:      npc.beaver.idle
speaker:                              # blank: no speaker name shown
lines:
  npc.beaver.idle.l1
repeat:      always
```

`lines:` is an ordered list of localization keys; each key resolves to one line of text shown in sequence. `repeat:` is one of `once | once_per_day | always`. `post_state:` (optional) describes a state mutation when the dialog ends — granting an item, flipping an entity's `active` flag, advancing the clock. The dialog player consumes this list; the renderer animates the text reveal at the player's configured text speed (§2.15).

### 4.4 Sprite asset manifest

Each theme directory carries a `manifest.txt` declaring per-sprite metadata the engine needs. Without it, gameplay data would have to be derived from image dimensions:

```
sprite:        tree_oak.png
origin:        32, 96         # texture-space pixel of bottom-center anchor
foot:          0, 0           # offset from origin to y-sort foot point
collide:       rect 8 0 16 8  # default collision rect relative to foot point
proximity:     rect -4 -4 24 20
shadow_h:      1.4            # height scalar for shadow length
layer:         object         # object | decor | character | hud
interact:      0, -12         # anchor for interaction prompt UI
```

**Per-instance overrides.** Defaults come from the sprite manifest. A maze entity line can override any shape field for that specific instance (e.g., a wider proximity for a fountain visible from a path):

```
fountain#village.fountain.market   24 18   sprite:fountain  proximity:rect:-8,-8,40,30
```

This lets a single sprite serve different gameplay roles across mazes without forcing a new sprite for each variation.

### 4.5 Conventions document

A `RENDER_CONVENTIONS.md` in `assets/` pins iso angle (e.g., 30°), pixel-per-tile (64×32), shadow direction at noon, outfit-layer alignment grid. The asset and item manifest formats are fed by Blender export scripts living in `art_src/`.

## 5. Tooling

### 5.1 Content validator

A small CLI tool, `tools/validate.c`, compiled separately. Runs against `data/mazes/*.maze` and reports problems before they reach the running game:

- Required headers (`name`, `theme`, `size`, `start`) exist and parse.
- Map dimensions match `size`.
- Start position is walkable.
- Entity `stable_id`s are unique within the maze (required) and follow the maze-prefix convention from §3.6 (warning, escalates to error in CI).
- Behavior names resolve against the registry.
- Item references in maze entity lines (`item:winter_coat`, `needs:winter_coat`) resolve against `data/items.txt`.
- Dialog references in entity lines (`dialog:npc.shopkeeper.greet`) resolve against `data/dialogs/*.txt`.
- Localization keys referenced in maze data, item definitions, and dialog definitions exist in `en.lang`.
- Entity positions are inside the map.
- Audio zones are well-formed rectangles inside the map.
- Reachability: a simple graph walk over walkable tiles from the start position visits every required progression item.
- Soft-lock **structural** risks: for any progression that uses a `maze`-scope consumable, the validator verifies that either an additional reachable instance exists, or the entity is annotated as rejecting misuse.

**What the validator does not do.** It does not prove puzzle solvability across the full state space of inventory, gates, time-of-day, outfits, one-way state changes, and event callbacks. That is too expensive for a tile-graph walk and would over-constrain authoring. The validator catches structural mistakes (unreachable items, missing rejection annotations, ID collisions, broken localization keys); end-to-end solvability is verified by playthrough tests, not by static analysis.

The validator runs as part of CI (when CI is added) and is invokable locally via a CMake target.

## 6. Build & cross-platform

**Tooling.** CMake ≥ 3.20. C11. No platform-specific scripts.

**Dependencies.**

- **raylib** via `FetchContent` pinned to an exact release tag (`5.0` to start). For reproducible offline builds, the project supports a local mirror: setting `AMAZEING_RAYLIB_LOCAL=/path/to/raylib-5.0` causes CMake to skip the network fetch and use the local source. Document this in `BUILD.md`. A future move to fully vendored raylib (as a submodule) remains an option if FetchContent flakiness becomes a real problem.
- **SQLite** vendored as the amalgamation at `third_party/sqlite/sqlite3.c` + `sqlite3.h`. Built as a static library inside our project.

**Compile flags.**

- **First-party code only:** `-Wall -Wextra -Wpedantic -Werror` on Linux/macOS, MSVC equivalent on Windows. Third-party targets (raylib, sqlite) compile without `-Werror` to avoid breakage on new compiler versions.
- Debug: `-fsanitize=address,undefined` on Linux/macOS for first-party code.
- Release: `-O2 -DNDEBUG`.
- SQLite: `-DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_DEFAULT_WAL_SYNCHRONOUS=1`.

**Packaging.**

- macOS: `MACOSX_BUNDLE TRUE` produces `AMazeIng.app`; assets copied into `Contents/Resources`. Codesign + notarize is a separate release step (the existing `macos-release` skill handles it).
- Windows: single `amazeing.exe` with `assets/` alongside. Ship as zip for now; Inno Setup installer later.
- Linux: bare binary + `assets/` alongside. AppImage as an optional release script.

**One-command build:** `cmake -S . -B build && cmake --build build -j` on all three platforms.

## 7. Testing

All tests run without a GL context wherever possible, preserving the raylib quarantine.

- **Unit tests** for pure modules:
  - `maze_data` parser, including malformed-file cases.
  - `save` roundtrip — write state, close, reopen, verify equality.
  - **Save migrations** — run each migration script against a snapshot of the prior schema, verify result.
  - `collision` shape queries.
  - `shadow` math.
  - `time_of_day` gating, including hour-band edge cases (gate spanning midnight).
  - `localization` lookup, fallback chain, malformed `.lang` file parsing.
  - `interaction` target selection — facing, ties, multi-item eligibility.
  - **Entity behavior tests** with a fake `Game` state — verify `on_player_enter`/`on_interact` against scripted scenarios.
  - **Maze validator** — verify each rule catches its intended bug class.
- **Headless smoke test.** Boots, loads each maze file, and steps the simulation N frames without rendering. Catches data-driven regressions.
- **Manual playtest** for anything visual, audio, or feel-related. The "relaxing" goal can only be evaluated by playing.

## 8. First playable vertical slice

Before the full MVP, the project ships a **vertical slice** that proves every subsystem end-to-end on a minimal content set. This catches architectural problems before they cost a maze's worth of content.

Slice contents:

- **One maze** — Nature (smallest practical map).
- **One outfit gate** — use a **slice-only** outfit (`rain_boots`) found in the Nature slice that gates a small muddy patch inside the slice itself. The winter coat and ice maze remain final-progression content and are not exercised by the slice.
- **One consumable item interaction** — a seed-on-plant or equivalent, with the §2.3 rejection rule enforced.
- **One NPC dialog** — single-state, single-line.
- **One audio base layer + one zone layer.**
- **Save/resume** for that one maze.
- **Placeholder art accepted** — colored boxes, simple shapes — but the full render pipeline is exercised (parallax, shadows, y-sort, dusk overlay).
- **Localization wired** — every player-visible string in the slice uses `t(...)`.

A passing vertical slice unlocks the rest of the content production.

## 9. MVP acceptance criteria

The MVP ships when **all** of the following are demonstrably true:

1. **Completable cold.** A new player can complete every maze without external instructions.
2. **Resume correctness.** Closing and reopening the game resumes correctly from every maze, on every supported platform.
3. **Reset safety.** Resetting a maze cannot remove profile-level unlocks or items earned in other mazes.
4. **Soft-lock resistance.** The validator catches structural soft-lock risks (passes for every shipped maze), and every shipped maze has at least one human playthrough confirming completion under the soft-lock rules in §2.3.
5. **Fully localized.** Every player-visible string routes through `t(...)`. A grep for raw English literals in `ui/` and dialog code returns nothing meaningful.
6. **Accessibility minimum.** Remappable controls, three volume sliders, text speed setting, color-independent puzzle cues throughout.
7. **Save robustness.** `PRAGMA integrity_check` runs on boot; corrupt DBs trigger a player-facing dialog and a timestamped backup before any overwrite.
8. **Performance.** Steady 60fps on a 2018-era laptop at a 1080p window.
9. **Quiet exit.** Closing the app at any point commits all pending save data. On next launch, SQLite opens without recovery errors and the latest checkpoint is present. (Sidecar `-wal`/`-shm` files are normal SQLite behavior and are not a correctness measure; if explicit cleanup is desired, the close path can call `sqlite3_wal_checkpoint_v2(TRUNCATE)`.)

## 10. Out of scope (MVP)

The following are intentionally deferred. Anything in this list can be added post-launch without an architectural change.

- Procedural maze generation.
- More than ~6 mazes.
- Multiplayer or co-op.
- Multiple playable characters.
- Melodic music.
- A visual maze editor (the plain-text format suffices for one designer).
- Modding APIs or scripting languages.
- RTL languages (Arabic, Hebrew) — would require HarfBuzz integration.
- CJK languages — would require extending the bundled font's codepoint set.
- Pluralization rules in localization (no count-driven strings in MVP).
- Cloud saves or save sync across machines.
- Achievements / Steam integration.
- Controller support (keyboard only for MVP; input architecture does not preclude it).
- Action-log undo (see §11).

## 11. Open questions

Each question lists the decision needed and the default behavior if the question is not resolved before implementation.

- **Undo last action.**
  *Decision needed:* add action-log undo, accept reset-only, or rely on soft-lock prevention (§2.3) alone.
  *Default if unresolved:* rely on soft-lock prevention alone. No action log in MVP.

- **CI host.**
  *Decision needed:* GitHub Actions, GitLab CI, or self-hosted runner for unit tests + headless smoke test + validator on Mac/Win/Linux.
  *Default if unresolved:* no CI in MVP. Local testing only. Add post-launch.

- **`clang-format`.**
  *Decision needed:* adopt now or after first non-trivial PR; which style.
  *Default if unresolved:* adopt LLVM style with 4-space indent before the first non-trivial PR.

- **Code coverage.**
  *Decision needed:* track coverage in CI, ignore, or measure locally only.
  *Default if unresolved:* not tracked in MVP. Revisit when engine stabilizes.

- **Telemetry / crash reporting.**
  *Decision needed:* none, opt-in local crash dumps, or opt-in network reporting.
  *Default if unresolved:* none in MVP. If added later, must be opt-in and never collect PII.

- **Time-advance affordance UI.**
  *Decision needed:* a fade-to-black with elapsed-hours indicator, a real-time sped-up clock, or a slider the player drags.
  *Default if unresolved:* fade-to-black with elapsed-hours indicator. Simplest and most readable.

- **Maze-select hub style.**
  *Decision needed:* visual hub map, list menu, or chapter-style book of contents.
  *Default if unresolved:* visual hub map with parallax background, mazes shown as labelled icons with lock state.

## 12. Glossary

| Term | Meaning |
|---|---|
| **Action** | A semantic input (interact, move-up, pause). Not a raw key code; remappable. |
| **Affordance** | A visible element that signals an action the player can take (a bench for time-advance, a sign for a hint). |
| **Behavior** | A named code path registered in `entity.c` that supplies an entity's event callbacks. |
| **Blocker** | An entity or hazard that prevents movement until a condition is met. |
| **Checkpoint** | A coherent moment at which the save system writes one transaction. Triggered by `save_checkpoint(reason)`. |
| **Consumable** | An `item_kind:consumable` item that is removed (or decremented) by use. Always `maze`-scoped in MVP. |
| **Dialog** | A data-driven sequence of localized lines spoken by an NPC. Defined in `data/dialogs/<theme>.txt`. |
| **Entity** | An object in the world with a position, optional collision/proximity shapes, and behavior callbacks. |
| **Foot point** | The pixel offset on a sprite used for y-sort. Lives in the asset manifest. |
| **Gate** | A condition (item required, time-of-day, outfit) that an entity uses to restrict interaction or visibility. |
| **Hazard** | A region or tile type that has an effect on the player (slide, slow, block). Never harmful in the death sense. |
| **Hint** | Localized text surfaced when an interaction is rejected or a blocker is examined. |
| **Item** | A pickable thing. Has a `scope` (`profile` or `maze`) and a `kind` (`outfit`, `tool`, `consumable`, `access`). Defined in `data/items.txt`. |
| **Item kind** | One of `outfit`, `tool`, `consumable`, `access`. Drives inventory grouping and use-eligibility. |
| **Maze** | One scrolling world. Loaded as a unit. Has stable `maze_id` text. |
| **Outfit** | An `item_kind:outfit` profile-scope item that changes the character's sprite and may unlock maze entry. |
| **Profile** | A named save slot. Up to four per install. |
| **Proximity shape** | The shape used for "is the player near this entity for interaction." Distinct from collision shape. |
| **Reset** | Player-initiated restart of the current maze. Preserves profile-scope state. See §2.3 for the exact reset table. |
| **Slice** | The first-playable vertical slice (§8). Slice-only content (e.g., `rain_boots`) is not part of final progression. |
| **Soft-lock** | A state where the player cannot make progress and is forced to reset. Prevented by §2.3 rules. |
| **Stable ID** | A designer-assigned text identifier on a maze entity. Persists across saves; survives entity reordering. By convention prefixed with `maze_id.`. |
| **Tick** | One fixed-step simulation step at 60 Hz (16.6 ms). Rendering may run at a higher rate. |
| **Tool** | An `item_kind:tool` item (e.g., watering can). `profile`-scoped if it unlocks later mazes, otherwise `maze`-scoped. |
| **Trigger** | A non-blocking region that fires `on_player_enter` / `on_player_exit`. |
| **UI scale** | Native-resolution multiplier for HUD/menus, set in Settings. World layer always upscales from logical 480×270. |
| **Unlock** | The state of a maze, item, or outfit becoming available. Persisted at profile scope. |
| **Zone** | A rectangular region in a maze that activates a texture audio layer when entered. |
