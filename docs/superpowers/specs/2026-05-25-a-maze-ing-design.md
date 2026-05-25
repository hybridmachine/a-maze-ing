# A Maze Ing — Design Spec

**Date:** 2026-05-25
**Status:** Draft, awaiting review
**Target platforms:** macOS, Windows, Linux
**Language:** C (C11)
**Engine:** raylib 5.x

## 1. Overview

A Maze Ing is a relaxing top-down 2D puzzle game. The player explores themed maze-worlds — one larger-than-screen scrolling world per theme — collecting items and using them to unlock new areas, change outfits, and progress to the maze's goal. There are no enemies, no death, and no time pressure. Mistakes are always reversible.

Visual style is pre-rendered isometric (Theme Hospital / Diablo aesthetic), with dynamic sun-driven shadows, parallax wall backdrops, and pixel-crisp upscaling. Audio is a layered ambient soundscape per theme — no melodic music. The game ships English-only at MVP with full localization infrastructure (drop in a `.lang` file to add a language).

The MVP is four to six hand-crafted themed mazes (Nature, Circuit Board, Village, Theme Park, Snow/Ice). Each maze is ~30–60 minutes of play. Full game is ~3–5 hours.

## 2. Game design

### 2.1 Core mechanic — collect-and-use

The maze is a puzzle of locks and keys, where "locks" can be locked doors, hazards that block paths, sleeping NPCs, time-gated objects, and so on, and "keys" are items the player collects: literal keys, outfits (a winter coat unlocks the icy area), tools (a watering can revives a withered plant), tickets, seeds, lanterns, etc. The player wanders, picks up items, uses them on the right targets, and the maze gradually opens up until the goal is reachable.

### 2.2 Failure model — no fail state

There is no death, no game-over, and no enemies. Hazards are environmental puzzles, not threats:

- Ice tiles slide the player in their direction of motion until they hit something stoppable.
- Mud tiles slow movement.
- Water tiles block movement unless the player is wearing waders.
- Thorn tiles block movement unless the player has gardening gloves.

If the player gets stuck (e.g., used the last seed on the wrong plant), they can reset the current maze from the Pause Menu. Reset restores the maze to its initial state but preserves outfits and items owned by the profile from prior mazes. A finer-grained "undo last action" (e.g., revert the most recent item-use) is desirable but not pinned for MVP — see Open Questions §8.

### 2.3 Movement

Free 8-direction movement at pixel granularity. The character walks with an animated cycle (arm swing, leg motion) and has a separate face layer for expressions (happy, surprised, cold, curious). Movement speed is constant; no sprinting.

### 2.4 Maze structure

Each maze is one scrolling world bigger than the screen. The camera follows the player with a soft deadzone and smoothing. No loading screens within a maze.

### 2.5 Scope — 4–6 mazes

Each themed world is one ~30–60 minute experience. The full game is ~3–5 hours. All mazes are hand-crafted; nothing is procedurally generated.

Themes for MVP:

1. **Nature** — forest, glades, ponds, dawn light.
2. **Circuit Board** — pulsing traces, capacitor pillars, LED lakes.
3. **Village** — half-timber houses, a market square, a shopkeeper.
4. **Theme Park** — rides, ticket booths, carnival games.
5. **Snow/Ice** — frozen pond, snow drifts, requires the winter coat.

### 2.6 Time of day

A single `Clock` advances at ~1 game-hour per 3 real-minutes (~70 minutes of play per full day-night cycle). The clock pauses during dialogs, the Pause Menu, and the Inventory screen. The player can accelerate time at benches/campfires.

A small set of objects are time-gated: a sundial only solves at noon, fireflies only appear at dusk, the village shopkeeper sleeps at night. Most of the world is not gated — time is primarily atmospheric.

### 2.7 Saves — auto-save with multiple profiles

Up to four named profiles. Active profile auto-saves on every meaningful event (item picked up, area transitioned, dialog completed, day rolled past midnight, pause menu opened, quit). Players close the app and resume exactly where they were. Save data lives in an SQLite database in the OS user-data dir (see §3.6).

### 2.8 Audio — ambient soundscape

No melodic music. Each theme owns four layers:

- **Base** — always-on ambient bed (forest birds + wind, circuit hum + beeps, etc.).
- **Texture** — zone-specific (creek by water, market chatter by vendor).
- **Time-of-day** — swaps day → dusk → night (crickets, owls, chimes).
- **One-shots** — pickups, footsteps, dialog beeps, hourly bells.

Crossfades on zone change take ~1–2 seconds.

### 2.9 Protagonist

A single fixed character. Identity is fixed; appearance changes by outfit (winter coat, raincoat, gardener apron, etc.). Outfits are unlocked by collecting them in mazes. Some mazes require a specific outfit to enter.

The character is composited from two sprite layers at draw time: outfit (walk cycle) and face (expression). One walk cycle is rendered per outfit; faces are overlaid on top. This avoids combinatorial sprite explosion.

### 2.10 Localization

All player-visible text routes through a single lookup function `t(key)`. Strings live in plain-text `.lang` files in `data/strings/`. MVP ships with English only; adding a language is dropping a new file. UTF-8 throughout. Default language at first launch matches the OS locale if a matching `.lang` file exists, otherwise English.

RTL languages and pluralization rules are out of scope for MVP.

## 3. Technical architecture

### 3.1 Module layout

Flat C modules, one `.c/.h` pair per subsystem. All state is passed explicitly via a single top-level `Game *g`; no globals.

| Module | Responsibility |
|---|---|
| `main` | Window init, raylib lifecycle, top-level loop |
| `game` | `Game` struct, state-machine dispatch (title / profile / maze-select / in-maze / pause / inventory / complete) |
| `input` | Keyboard reads, action mapping |
| `player` | Position, velocity, animation state, outfit |
| `world` | Tile grid, entity list, maze load/unload |
| `entity` | Entity tagged union + behavior registry |
| `collision` | Walkability and interaction queries |
| `render` | Iso projection, depth sort, all draw passes |
| `camera` | Follow-cam with deadzone and smoothing |
| `shadow` | Sun vector, per-entity shadow placement |
| `time_of_day` | Clock, time-gate checks |
| `parallax` | Faux-3D wall backdrop layers |
| `audio` | Layered ambient mixer + one-shots |
| `asset` | Texture/sound/font loader and cache |
| `save` | SQLite-backed profile + game-state persistence |
| `profile` | Profile list and active-profile state |
| `ui` | Title, HUD, dialogs, inventory, settings |
| `localization` | `t(key)` lookup, `.lang` parser, language switching |
| `maze_data` | Maze file load/parse |
| `util` | Math helpers, logging, OS path resolution |

### 3.2 Dependency rules

- **One-way deps.** `render`, `audio`, `save`, `ui` read from `world`/`player`; never the reverse.
- **`raylib` quarantined.** Only `main`, `render`, `audio`, `input`, `asset` include `<raylib.h>`. Game logic stays portable and testable.
- **Asset cache lives in `asset`.** Other modules ask for textures and sounds by string key; `asset` deduplicates and reference-counts.
- **No globals.** A single `Game *g` is threaded through. Each subsystem takes only the parts it needs.
- **File-size guardrail.** Aim for under ~500 lines per `.c`. Split by category if a module grows past that (e.g., `entity_item.c`, `entity_npc.c`, `entity_hazard.c`).

### 3.3 Entity model

Entities are a tagged union. The union holds kind-specific data; behavior functions are dispatched through fn-pointers resolved by a registry at load time.

```c
typedef enum {
    ENT_ITEM,         // pickable: seed, coat, ticket, key
    ENT_HAZARD,       // region: water, ice, mud, thorns
    ENT_INTERACT,     // switch, door, sundial, signpost
    ENT_NPC,          // shopkeeper, child, animal
    ENT_DECOR,        // visual-only, depth-sorted
} EntityKind;

typedef struct Entity {
    EntityKind kind;
    uint32_t   id;
    Vector2    pos;
    Rect       bbox;
    int        sprite_id;
    int        shadow_id;     // -1 = no shadow
    uint32_t   flags;
    int8_t     gate_hour_a;   // -1 = always active
    int8_t     gate_hour_b;
    void (*on_interact)(Game*, Entity*);
    void (*on_step)    (Game*, Entity*);
    union {
        struct { int  item_id; bool taken; }       item;
        struct { int  type;    int  param;  }      hazard;
        struct { int  target;  bool active; }      interact;
        struct { int  dialog_id; int  state; }     npc;
    };
} Entity;
```

Behavior registration: `entity.c` holds a small table mapping behavior names (`"shopkeeper"`, `"sundial"`, `"ice_patch"`) to fn-pointers. `maze_data` resolves names at load time. Adding a new behavior is a one-line registry row plus the fn definition.

### 3.4 Maze data format — plain text

Each maze is a single file at `data/mazes/<theme>.maze`. Header is `key: value` lines. The `tiles:` block is a literal ASCII map; each character maps to a sprite via the theme's tileset definition. The `entities:` block is one entity per line.

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
seed         12 8   pickup
sundial      20 15  gate:12-13
shopkeeper   30 22  gate:06-22  dialog:npc.shopkeeper.greet
ice_patch    40 30  hazard:ice
locked_gate  50 18  needs:winter_coat  target:area_b
beaver       28 19  npc  patrol:28,19-32,19
fireflies    18 12  gate:19-23

audio_zones:
creek        12-18 30-36  texture:water_creek.ogg
market       28-36 20-26  texture:village_chatter.ogg
```

`audio_zones:` declares rectangular regions in tile coords. When the player enters a zone, the audio mixer crossfades the named clip into the active texture layer; on exit, it crossfades back to silence. Zones may overlap; the topmost listed zone wins.

**Tiles vs entities split:** the tile grid only describes walkable ground. Anything with vertical extent (a fountain, a tree canopy, a sign) goes in `entities:` as a sprite with a shadow.

Maze names, NPC dialog references, and item names use localization keys (e.g., `maze.nature.name`), not English prose. Translators only touch `data/strings/`.

Parser is ~200 lines, hand-rolled, no JSON dependency. Diff-friendly. A future visual editor (e.g., Tiled `.tmx`) can be added by extending the loader; not needed for MVP.

### 3.5 Rendering pipeline

Each frame renders to a fixed-size logical render texture (e.g., 480×270), then upscales with nearest-neighbor to the window. Preserves pixel crispness at any window size. raylib's `BeginTextureMode` / `DrawTexturePro` handles this directly.

Per-frame draw order:

1. Sky / gradient — tinted by time of day.
2. Far parallax — distant hills/walls.
3. Near parallax — closer wall layer.
4. Ground tiles — iso, no z-sort (always below entities).
5. Shadows pass — all shadows drawn under all sprites (so a shadow never occludes another sprite).
6. Entities — y-sorted ascending.
7. Player — interleaved into the entity y-sort.
8. Light/dusk overlay — multiplicative tint over the playfield.
9. HUD and clock — screen space, no camera transform.

**Iso projection.** `sx = (tx - ty) * TW/2; sy = (tx + ty) * TH/2`. Tile dimensions are nominally 64×32 (2:1 dimetric).

**Y-sort.** `qsort` of visible entities each frame; at hundreds of entities the cost is negligible.

**Dynamic shadows.** `time_of_day` exposes a sun vector (2D direction + length scalar + elevation alpha) derived from the current hour. `shadow.c` reads it once per frame; each shadow draws with offset = `sun.dir * sun.length * entity.height`, Y-scale modulated by `sun.length`, alpha by `sun.elevation`. ~10 lines of code, no real 3D math, reads as 3D.

**Parallax walls.** Two sprite layers offset by camera position × per-layer factor (0.3 far, 0.7 near). The player cannot walk behind them; they're a faux-3D backdrop on the camera edge.

### 3.6 Save system — SQLite

One database file (`amazeing.db`) holds all profiles. WAL journal mode, single-threaded build (`SQLITE_THREADSAFE=0`), load-extension disabled.

**Location:**

- macOS: `~/Library/Application Support/AMazeIng/amazeing.db`
- Windows: `%APPDATA%\AMazeIng\amazeing.db`
- Linux: `$XDG_DATA_HOME/AMazeIng/amazeing.db` (fallback `~/.local/share/AMazeIng/`)

A small `util_paths.c` resolves the per-OS path at boot.

**Schema (initial):**

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

CREATE TABLE inventory (
  profile_id  INTEGER REFERENCES profiles(id) ON DELETE CASCADE,
  item_id     TEXT NOT NULL,
  count       INTEGER NOT NULL DEFAULT 1,
  acquired_at INTEGER NOT NULL,
  PRIMARY KEY (profile_id, item_id)
);

CREATE TABLE outfits_owned (
  profile_id  INTEGER REFERENCES profiles(id) ON DELETE CASCADE,
  outfit_id   TEXT NOT NULL,
  PRIMARY KEY (profile_id, outfit_id)
);

CREATE TABLE maze_progress (
  profile_id   INTEGER REFERENCES profiles(id) ON DELETE CASCADE,
  maze_id      TEXT NOT NULL,
  state        TEXT NOT NULL,  -- 'locked' | 'in_progress' | 'complete'
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

CREATE TABLE entity_overrides (
  profile_id  INTEGER,
  maze_id     TEXT,
  entity_id   INTEGER,
  taken       INTEGER NOT NULL DEFAULT 0,
  active      INTEGER NOT NULL DEFAULT 0,
  state_blob  BLOB,   -- escape hatch for kind-specific extras
  PRIMARY KEY (profile_id, maze_id, entity_id),
  FOREIGN KEY (profile_id) REFERENCES profiles(id) ON DELETE CASCADE
);
```

**API.** `save.c` exposes a typed surface — `save_open`, `save_close`, `save_begin`, `save_commit`, plus helpers like `save_set_inventory_item`, `save_set_entity_override`, `save_set_clock`, `save_load_maze_snapshot`. Other modules never see SQL.

**Migrations.** On open, read `PRAGMA user_version`; if older than the binary's expected version, run a sequence of embedded SQL migration scripts under one transaction, then bump the version.

### 3.7 Audio mixer

Built on raylib's `Music` (for streaming ambient OGG layers) and `Sound` (for one-shots). `audio.c` keeps a small mixer table per active layer with current volume and target volume. Each frame, volumes lerp toward target; layer position is shared across layers (so the loop boundary stays synchronized).

Layer table is reconfigured on:

- Maze entry/exit (full set swap).
- Zone enter/leave (a per-region "audio zone" tag in the maze file activates the texture layer).
- Time-of-day transitions (day/dusk/night layer set).

### 3.8 Localization

`localization.c` keeps an open-addressed hashmap from string key to UTF-8 value. On `loc_set_language("xx")`:

1. Open `data/strings/xx.lang`, parse key-value lines (`#` comments, `=` separator, trimmed values).
2. Rebuild the hashmap from the parsed entries.
3. The English file (`en.lang`) is always loaded as a fallback layer.

`t(key)` returns the active-language string if present, else the English string, else the key itself. **Never returns NULL** — UI code can always draw something.

On first launch, the loader inspects `getenv("LANG")` / `setlocale` / Windows `GetUserDefaultLocaleName` (per OS) and picks the matching `.lang` file if it exists, else `en`. Player's choice is then stored in the `settings` table and used from that point on.

Bundled font (Inter or Noto Sans, OFL-licensed) is baked at boot via `LoadFontEx` with a Latin Extended codepoint set. Codepoints outside the bundled font's coverage render as the font's "missing glyph" box — the game never crashes on a partially-translated `.lang` file or unexpected codepoint.

## 4. Asset pipeline

- Source art lives in `art_src/` (Blender files, render scripts). Not shipped with the game.
- Final assets are PNG sprite sheets in `assets/themes/<theme>/`, `assets/character/outfits/`, `assets/character/faces/`, `assets/ui/`.
- A `RENDER_CONVENTIONS.md` in `assets/` pins iso angle (e.g., 30°), pixel-per-tile (64×32), shadow direction at noon, outfit-layer alignment grid.
- Audio: OGG Vorbis throughout, mono for one-shots, stereo for ambient layers. Mastered to consistent loudness so the mixer can blend without per-clip gain compensation.
- No build-time baking. Game loads assets at runtime through `asset.c`.

## 5. Build & cross-platform

**Tooling.** CMake ≥ 3.20. C11. No platform-specific scripts.

**Dependencies.**

- **raylib** via `FetchContent` pinned to a release tag (5.0 to start). CMake downloads and builds it.
- **SQLite** vendored as the amalgamation at `third_party/sqlite/sqlite3.c` + `sqlite3.h`. Built as a static library inside our project.

**Compile flags.**

- All platforms: `-Wall -Wextra -Wpedantic -Werror` (MSVC equivalent).
- Debug: `-fsanitize=address,undefined` on Linux/macOS.
- Release: `-O2 -DNDEBUG`.
- SQLite: `-DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_DEFAULT_WAL_SYNCHRONOUS=1`.

**Packaging.**

- macOS: `MACOSX_BUNDLE TRUE` produces `AMazeIng.app`; assets copied into `Contents/Resources`. Codesign + notarize is a separate release step (the existing `macos-release` skill handles it).
- Windows: single `amazeing.exe` with `assets/` alongside. Ship as zip for now; Inno Setup installer later.
- Linux: bare binary + `assets/` alongside. AppImage as an optional release script.

**One-command build:** `cmake -S . -B build && cmake --build build -j` on all three platforms.

## 6. Testing

- **Unit tests** for pure modules: `maze_data` (parser), `save` (roundtrip), `collision`, `shadow` (math), `time_of_day` (gating), `localization` (lookup + fallback). Single-header test framework (`greatest.h` or similar), run via `ctest`.
- **Headless smoke test** boots, loads each maze file, and steps the simulation N frames without rendering. Catches data-driven regressions. Skipped in graphical CI without a GL context.
- **Manual playtest** for anything visual, audio, or feel-related. The "relaxing" goal can only be evaluated by playing.

## 7. Out of scope (MVP)

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
- Controller support (keyboard only for MVP).

## 8. Open questions

- **Undo last action.** Reset-whole-maze is the only roll-back in MVP. For collect-and-use puzzles where a single misuse of a consumable (e.g., the last seed) can soft-lock a maze, an "undo last item-use" would be a meaningful quality-of-life feature. Implementing it cleanly requires recording an action log alongside the live game state. Defer to a post-MVP decision.
- **CI.** Not specified in this spec. A future addition could be GitHub Actions running unit tests and a headless smoke test on Mac/Win/Linux.
- **`clang-format`.** Not yet decided. Recommend adopting before the first non-trivial PR.
- **Code coverage tracking.** Not in MVP scope; revisit after the engine stabilizes.
- **Telemetry / crash reporting.** Not in MVP. If added, must be opt-in and not collect PII.
- **Save DB corruption fallback.** If `amazeing.db` fails to open or fails integrity check, the boot path should offer the player a "create fresh" option that backs up the broken DB rather than silently overwriting. Detailed flow not yet pinned.
