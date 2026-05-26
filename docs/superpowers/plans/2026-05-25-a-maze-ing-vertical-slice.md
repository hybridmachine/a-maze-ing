# A Maze Ing — Vertical Slice Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship the first-playable vertical slice from §8 of the design spec — one Nature maze that exercises every subsystem end-to-end with placeholder art, proving the engine before content production begins.

**Architecture:** Flat C11 modules in `src/`, one `.c/.h` per subsystem. State threaded through a single top-level `Game *g`. raylib quarantined to `main`/`render`/`audio`/`input`/`asset`. Fixed-step 60 Hz simulation with an accumulator; rendering at display refresh. SQLite (vendored amalgamation) for saves. Data-driven content: maze files, items, dialogs, sprite manifests, `.lang` strings. Validator runs as a CLI tool against the data files.

**Tech Stack:** C11, CMake ≥ 3.20, raylib 5.x (via `FetchContent`), SQLite amalgamation vendored at `third_party/sqlite/`, small in-house test harness, OGG audio, PNG sprites.

---

## Scope

This plan covers the §8 vertical slice **only**: one Nature maze, one slice-only outfit gate (`rain_boots`), one consumable interaction with the §2.3 rejection rule, one NPC dialog, one base + one zone audio layer, save/resume, placeholder art exercising the full render pipeline, full localization wiring.

**Out of scope for this plan** (separate plans follow once the slice passes):
- Mazes 2–5 (Circuit, Village, Theme Park, Snow/Ice) and final-progression items (winter coat).
- Production art, audio, and dialogs beyond what the slice needs.
- CI host selection, clang-format adoption, telemetry — left as §11 open questions.

The vertical slice gates further content work. Per §8 of the spec: "A passing vertical slice unlocks the rest of the content production."

---

## File Structure

Files created by this plan. Designer-facing data files (`data/`, `assets/`) get a single set; engine files (`src/`) split by responsibility. Aim for under ~500 lines per `.c` (spec §3.2).

### Build & top-level

```
CMakeLists.txt                          # raylib FetchContent, sqlite static lib, app + tests + validate targets
BUILD.md                                # build instructions, AMAZEING_RAYLIB_LOCAL env var, deps
.clang-format                           # LLVM, 4-space (§11 default)
third_party/sqlite/sqlite3.c            # SQLite amalgamation (downloaded once, vendored)
third_party/sqlite/sqlite3.h
third_party/test_harness/test_harness.h # in-house assert+run macros
third_party/test_harness/test_harness.c # shared counter storage for all test TUs
```

### Engine (`src/`)

```
src/main.c                              # raylib init, window, top-level loop call
src/game.c, game.h                      # Game struct, state-machine dispatch
src/input.c, input.h                    # action map, buffered edges, remap
src/player.c, player.h                  # pos, vel, anim state, outfit
src/world.c, world.h                    # tile grid, entity arrays, maze load/unload
src/entity.c, entity.h                  # Entity union, behavior registry
src/entity_item.c                       # item pickup/use behaviors
src/entity_hazard.c                     # water/ice/mud/thorns behaviors
src/entity_interact.c                   # door, sign, sundial behaviors
src/entity_npc.c                        # NPC + dialog driver
src/collision.c, collision.h            # shape overlap queries
src/interaction.c, interaction.h        # target selection, item-use dispatch
src/render.c, render.h                  # top-level draw driver
src/render_world.c                      # tiles, entities, shadows, parallax, dusk
src/render_ui.c                         # native-resolution UI dispatch
src/camera.c, camera.h                  # follow with deadzone+smoothing
src/shadow.c, shadow.h                  # sun vector, per-entity shadow draw
src/time_of_day.c, time_of_day.h        # clock, day_index, gates
src/parallax.c, parallax.h              # faux-3D wall backdrop
src/audio.c, audio.h                    # mixer table, crossfades, zones
src/asset.c, asset.h                    # texture/sound/font loader+cache
src/save.c, save.h                      # SQLite-backed persistence + checkpoint API
src/profile.c, profile.h                # profile list + active-profile state
src/ui.c, ui.h                          # UI state dispatch
src/ui_title.c                          # title screen
src/ui_profile.c                        # profile select / create
src/ui_maze_select.c                    # maze-select hub
src/ui_hud.c                            # in-maze HUD (clock, outfit indicator)
src/ui_dialog.c                         # dialog renderer with text speed
src/ui_inventory.c                      # inventory grid (kind grouping)
src/ui_settings.c                       # settings + remap + volumes + lang
src/ui_interaction_prompt.c             # overhead "press to interact" prompt
src/localization.c, localization.h      # t(key), .lang parser, fallback
src/maze_data.c, maze_data.h            # maze file parse + validator hooks
src/util_log.c, util_log.h              # leveled logging to stderr + file
src/util_paths.c, util_paths.h          # per-OS user-data dir resolver
src/util_math.c, util_math.h            # Vector2, rect/capsule, lerp
```

### Tools

```
tools/validate.c                        # CLI maze/item/dialog validator
tools/smoke_test.c                      # headless: load each maze, step N frames
```

### Tests (`tests/`)

```
tests/test_main.c                       # registers all suites
tests/test_util_math.c
tests/test_localization.c
tests/test_maze_data.c
tests/test_items.c
tests/test_dialogs.c
tests/test_save_roundtrip.c
tests/test_save_migrations.c
tests/test_save_reset.c
tests/test_collision.c
tests/test_shadow.c
tests/test_time_of_day.c
tests/test_input_remap.c
tests/test_interaction.c
tests/test_dialog_repeat.c
tests/test_entity_behaviors.c
tests/test_validator.c
tests/fixtures/                         # minimal .maze, items.txt, en.lang fixtures
```

### Data & assets

```
data/items.txt                          # slice items (seed, rain_boots, withered_plant tag use)
data/dialogs/nature.txt                 # one NPC dialog
data/mazes/nature.maze                  # the slice maze
data/strings/en.lang                    # all slice strings keyed
assets/themes/nature/manifest.txt       # per-sprite manifest
assets/themes/nature/*.png              # placeholder colored rects baked as PNG
assets/character/outfits/default.png    # placeholder
assets/character/outfits/rain_boots.png
assets/character/faces/*.png            # 4 placeholder expressions
assets/ui/icons/*.png                   # item icons
assets/ui/font/Inter-Regular.ttf        # bundled OFL font
assets/audio/birds_loop.ogg             # placeholder ambient
assets/audio/wind_soft.ogg
assets/audio/water_creek.ogg            # zone layer
assets/audio/pickup.ogg                 # one-shot
assets/RENDER_CONVENTIONS.md            # iso angle, ppt, shadow direction at noon
```

---

## Phases

54 tasks across these phases. Tasks within a phase mostly serialize; the phase boundaries are review checkpoints.

| Phase | Tasks | Purpose |
|---|---|---|
| **A. Foundation** | 1–5 | CMake, paths, math, logging, asset cache, test harness |
| **B. Localization** | 6–8 | `.lang` parser, `t(key)`, OS locale detection |
| **C. Data manifests** | 9–12 | items, dialogs, sprite manifest, maze parsers |
| **D. Save** | 13–17 | SQLite, schema, migrations, checkpoint API, reset, profiles |
| **E. Input** | 18–19 | action map, buffered edges, remap rules |
| **F. Game core** | 20–21 | `Game` struct, fixed-step loop, state machine |
| **G. World simulation** | 22–27 | tiles, entity union, collision, movement, hazards, interaction |
| **H. Entity behaviors** | 28–30 | item pickup, door/sign, NPC + dialog driver |
| **I. Time of day** | 31–32 | clock, sun vector, gates |
| **J. Audio** | 33–34 | mixer, zones, time-of-day swap |
| **K. Camera + Render** | 35–39 | follow-cam, world texture, iso, y-sort, shadows, parallax, dusk |
| **L. UI** | 40–46 | title, profile, hub, HUD, dialog, inventory, settings |
| **M. Slice content** | 47–50 | manifest, placeholder art, maze data, audio wiring |
| **N. Validator + Acceptance** | 51–54 | validator CLI, smoke test, integrity recovery, playthrough |

---

## Phase A — Foundation

### Task 1: Project skeleton, CMake, raylib window

**Goal:** A bare `amazeing` binary that opens an 800×450 raylib window, clears to black, and exits cleanly when the close button is hit.

**Files:**
- Create: `CMakeLists.txt`
- Create: `BUILD.md`
- Create: `.clang-format`
- Create: `src/main.c`

- [ ] **Step 1: Write CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.20)
project(amazeing C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
if(DEFINED ENV{AMAZEING_RAYLIB_LOCAL})
    add_subdirectory($ENV{AMAZEING_RAYLIB_LOCAL} raylib_build)
else()
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.0)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror
        $<$<CONFIG:Debug>:-fsanitize=address,undefined>
        $<$<CONFIG:Release>:-O2>)
    add_link_options($<$<CONFIG:Debug>:-fsanitize=address,undefined>)
endif()

add_executable(amazeing src/main.c)
target_link_libraries(amazeing PRIVATE raylib)
if(APPLE)
    set_target_properties(amazeing PROPERTIES MACOSX_BUNDLE TRUE)
endif()
```

- [ ] **Step 2: Write src/main.c**

```c
#include <raylib.h>

int main(void) {
    InitWindow(800, 450, "A Maze Ing");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("A Maze Ing - boot", 20, 20, 20, RAYWHITE);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
```

- [ ] **Step 3: Write BUILD.md**

```markdown
# Build

```
cmake -S . -B build
cmake --build build -j
./build/amazeing      # macOS/Linux
./build/amazeing.exe  # Windows
```

Offline raylib: set `AMAZEING_RAYLIB_LOCAL=/path/to/raylib-5.0` before cmake.
```

- [ ] **Step 4: Write .clang-format**

```
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
AllowShortFunctionsOnASingleLine: None
```

- [ ] **Step 5: Configure and build**

Run: `cmake -S . -B build && cmake --build build -j`
Expected: builds without warnings; produces `build/amazeing` (or `build/amazeing.app/Contents/MacOS/amazeing` on macOS).

- [ ] **Step 6: Run, verify window opens**

Run: `./build/amazeing` (macOS: `open build/amazeing.app`)
Expected: black window with "A Maze Ing - boot"; closes when window-close is clicked.

- [ ] **Step 7: Commit**

```bash
git add CMakeLists.txt BUILD.md .clang-format src/main.c
git commit -m "feat(build): bootstrap CMake + raylib window"
```

---

### Task 2: util_log, util_paths

**Goal:** Leveled logging + per-OS resolver for the user-data dir where `amazeing.db` will live.

**Files:**
- Create: `src/util_log.c`, `src/util_log.h`
- Create: `src/util_paths.c`, `src/util_paths.h`
- Modify: `CMakeLists.txt` — add new sources to `amazeing` target.

- [ ] **Step 1: Write util_log.h**

```c
#pragma once
#include <stdarg.h>

typedef enum { LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR } LogLevel;

void util_log_init(const char *file_path);   // file_path may be NULL = stderr only
void util_log(LogLevel lvl, const char *fmt, ...);
void util_log_close(void);

#define LOGD(...) util_log(LOG_DEBUG, __VA_ARGS__)
#define LOGI(...) util_log(LOG_INFO,  __VA_ARGS__)
#define LOGW(...) util_log(LOG_WARN,  __VA_ARGS__)
#define LOGE(...) util_log(LOG_ERROR, __VA_ARGS__)
```

- [ ] **Step 2: Write util_log.c**

```c
#include "util_log.h"
#include <stdio.h>
#include <time.h>

static FILE *g_log_file = NULL;

void util_log_init(const char *file_path) {
    if (file_path) g_log_file = fopen(file_path, "a");
}

void util_log(LogLevel lvl, const char *fmt, ...) {
    static const char *names[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    time_t t = time(NULL);
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char prefix[64];
    snprintf(prefix, sizeof prefix, "[%02d:%02d:%02d %s] ",
             tm.tm_hour, tm.tm_min, tm.tm_sec, names[lvl]);

    va_list ap;
    va_start(ap, fmt);
    fputs(prefix, stderr); vfprintf(stderr, fmt, ap); fputc('\n', stderr);
    if (g_log_file) {
        va_end(ap); va_start(ap, fmt);
        fputs(prefix, g_log_file); vfprintf(g_log_file, fmt, ap); fputc('\n', g_log_file);
        fflush(g_log_file);
    }
    va_end(ap);
}

void util_log_close(void) {
    if (g_log_file) { fclose(g_log_file); g_log_file = NULL; }
}
```

- [ ] **Step 3: Write util_paths.h**

```c
#pragma once
#include <stdbool.h>
#include <stddef.h>

// Fills `out` with the absolute path to AMazeIng's user-data dir for the
// current OS (creating it if missing). Returns true on success.
bool util_paths_userdata_dir(char *out, size_t out_size);

// Fills `out` with the absolute path to the save DB file inside that dir.
bool util_paths_save_db(char *out, size_t out_size);
```

- [ ] **Step 4: Write util_paths.c (per-OS resolution)**

```c
#include "util_paths.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
  #include <direct.h>
  #include <shlobj.h>
#endif

static bool ensure_dir(const char *path) {
#ifdef _WIN32
    return _mkdir(path) == 0 || errno == EEXIST;
#else
    return mkdir(path, 0700) == 0 || errno == EEXIST;
#endif
}

bool util_paths_userdata_dir(char *out, size_t n) {
#ifdef __APPLE__
    const char *home = getenv("HOME");
    if (!home) return false;
    snprintf(out, n, "%s/Library/Application Support/AMazeIng", home);
#elif defined(_WIN32)
    char appdata[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata) != S_OK) return false;
    snprintf(out, n, "%s\\AMazeIng", appdata);
#else
    const char *xdg = getenv("XDG_DATA_HOME");
    const char *home = getenv("HOME");
    if (xdg && *xdg) snprintf(out, n, "%s/AMazeIng", xdg);
    else if (home)   snprintf(out, n, "%s/.local/share/AMazeIng", home);
    else return false;
#endif
    return ensure_dir(out);
}

bool util_paths_save_db(char *out, size_t n) {
    char dir[1024];
    if (!util_paths_userdata_dir(dir, sizeof dir)) return false;
    snprintf(out, n, "%s/amazeing.db", dir);
    return true;
}
```

- [ ] **Step 5: Add to CMakeLists.txt**

In `add_executable(amazeing ...)`, append: `src/util_log.c src/util_paths.c`.

- [ ] **Step 6: Smoke check from main.c**

Edit `src/main.c` to call `util_log_init(NULL); LOGI("user-data: %s", path); util_log_close();` where `path` comes from `util_paths_userdata_dir`. Rebuild, run, confirm the path prints and the dir was created.

- [ ] **Step 7: Commit**

```bash
git add src/util_log.* src/util_paths.* src/main.c CMakeLists.txt
git commit -m "feat(util): leveled logging + per-OS user-data path resolver"
```

---

### Task 3: util_math (Vector2, shapes, helpers)

**Goal:** Math primitives the rest of the engine reuses. Pure code, fully unit-tested.

**Files:**
- Create: `src/util_math.h`, `src/util_math.c`
- Test: created in Task 4 (`tests/test_util_math.c`) once harness is in place.

- [ ] **Step 1: Write util_math.h**

```c
#pragma once
#include <stdbool.h>

typedef struct { float x, y; } Vec2;

typedef enum { SHAPE_RECT, SHAPE_CAPSULE } ShapeKind;

typedef struct {
    ShapeKind kind;
    float ox, oy;          // local origin offset from entity pos
    float w, h;            // rect: full size. capsule: w=diameter, h=segment length
} Shape;

Vec2  v2_add(Vec2 a, Vec2 b);
Vec2  v2_sub(Vec2 a, Vec2 b);
Vec2  v2_scale(Vec2 a, float s);
float v2_len(Vec2 a);
float v2_dot(Vec2 a, Vec2 b);
float clampf(float v, float lo, float hi);
float lerpf(float a, float b, float t);

// Overlap tests in world space. `pos_a` is the world position of shape a's owner.
bool shape_overlap(Shape a, Vec2 pos_a, Shape b, Vec2 pos_b);

// Slide a point along a blocking shape: given desired position, blocking shape,
// foot shape, return the corrected position (closest non-overlapping point).
Vec2 shape_slide(Vec2 desired, Shape foot, Shape block, Vec2 block_pos);
```

- [ ] **Step 2: Write util_math.c**

Implement: `v2_*` trivially; `clampf`, `lerpf`; `shape_overlap` covering rect-rect (AABB), rect-capsule (project capsule axis onto rect), capsule-capsule (segment-to-segment distance vs sum of radii). `shape_slide` resolves on the smaller-overlap axis (push out along x or y, whichever has less overlap).

Implementation outline (~150 lines). The detailed math: for a capsule, treat it as a circle swept along a vertical segment of length `h-w` (so a `h==w` capsule is a circle). Rect-capsule overlap = distance from rect to the capsule's segment ≤ radius (w/2). Use `clampf` to project the capsule axis points onto the rect.

- [ ] **Step 3: Wire into CMake**

Append `src/util_math.c` to the `amazeing` executable in `CMakeLists.txt`.

- [ ] **Step 4: Build to verify it compiles**

Run: `cmake --build build -j`
Expected: builds without warnings. (Tests come in Task 4.)

- [ ] **Step 5: Commit**

```bash
git add src/util_math.* CMakeLists.txt
git commit -m "feat(util): Vec2, Shape, overlap and slide"
```

---

### Task 4: Test harness + first test suite

**Goal:** Tiny test harness and a `tests` CMake target that runs `test_util_math` first. Sets up the pattern every later test follows.

**Files:**
- Create: `third_party/test_harness/test_harness.h`
- Create: `third_party/test_harness/test_harness.c`
- Create: `tests/test_main.c`
- Create: `tests/test_util_math.c`
- Modify: `CMakeLists.txt` — add `tests` executable + `enable_testing` + `add_test`.

- [ ] **Step 1: Write test_harness.h + test_harness.c**

```c
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int g_test_passed;
extern int g_test_failed;
extern const char *g_current_test;

#define ASSERT(cond) do {                                                     \
    if (!(cond)) {                                                            \
        fprintf(stderr, "  FAIL %s:%d in %s: %s\n",                           \
                __FILE__, __LINE__, g_current_test, #cond);                   \
        g_test_failed++; return;                                              \
    }                                                                         \
} while (0)

#define ASSERT_EQ_INT(a, b) do {                                              \
    long _a = (long)(a), _b = (long)(b);                                      \
    if (_a != _b) {                                                           \
        fprintf(stderr, "  FAIL %s:%d in %s: %s != %s (%ld vs %ld)\n",        \
                __FILE__, __LINE__, g_current_test, #a, #b, _a, _b);          \
        g_test_failed++; return;                                              \
    }                                                                         \
} while (0)

#define ASSERT_NEAR(a, b, eps) do {                                           \
    double _d = (double)(a) - (double)(b);                                    \
    if (_d < -(eps) || _d > (eps)) {                                          \
        fprintf(stderr, "  FAIL %s:%d in %s: %s !~ %s (%.6f vs %.6f)\n",      \
                __FILE__, __LINE__, g_current_test, #a, #b,                   \
                (double)(a), (double)(b));                                    \
        g_test_failed++; return;                                              \
    }                                                                         \
} while (0)

#define RUN_TEST(fn) do {                                                     \
    g_current_test = #fn;                                                     \
    int prev_failed = g_test_failed;                                          \
    fn();                                                                     \
    if (g_test_failed == prev_failed) { g_test_passed++; printf("  ok %s\n", #fn); } \
} while (0)

#define TEST_SUMMARY() do {                                                   \
    printf("\n%d passed, %d failed\n", g_test_passed, g_test_failed);         \
    return g_test_failed == 0 ? 0 : 1;                                        \
} while (0)
```

```c
// third_party/test_harness/test_harness.c
#include "test_harness.h"

int g_test_passed = 0;
int g_test_failed = 0;
const char *g_current_test = NULL;
```

Counter storage must live in one translation unit. Keeping these variables `static`
in the header gives each test file its own counters and makes failures invisible
to `TEST_SUMMARY()`.

- [ ] **Step 2: Write tests/test_util_math.c (a failing test first)**

```c
#include "test_harness.h"
#include "util_math.h"

void test_v2_add(void) {
    Vec2 r = v2_add((Vec2){1, 2}, (Vec2){3, 4});
    ASSERT_NEAR(r.x, 4.0f, 0.0001f);
    ASSERT_NEAR(r.y, 6.0f, 0.0001f);
}

void test_rect_rect_overlap(void) {
    Shape a = {SHAPE_RECT, 0, 0, 10, 10};
    Shape b = {SHAPE_RECT, 0, 0, 10, 10};
    ASSERT(shape_overlap(a, (Vec2){0, 0}, b, (Vec2){5, 5}));
    ASSERT(!shape_overlap(a, (Vec2){0, 0}, b, (Vec2){20, 20}));
}

void test_slide_resolves_overlap(void) {
    Shape foot = {SHAPE_RECT, -2, -2, 4, 4};
    Shape block = {SHAPE_RECT, 0, 0, 10, 10};
    Vec2 desired = {5, 0};         // foot center wants to be inside block
    Vec2 out = shape_slide(desired, foot, block, (Vec2){10, 0});
    // Foot's right edge should not exceed block's left edge (10 - 5 = 5).
    ASSERT(out.x + 2 <= 10.0001f);
}

void register_util_math_tests(void) {
    RUN_TEST(test_v2_add);
    RUN_TEST(test_rect_rect_overlap);
    RUN_TEST(test_slide_resolves_overlap);
}
```

- [ ] **Step 3: Write tests/test_main.c**

```c
#include "test_harness.h"

void register_util_math_tests(void);

int main(void) {
    printf("util_math\n");
    register_util_math_tests();
    TEST_SUMMARY();
}
```

- [ ] **Step 4: CMake wiring**

Append to `CMakeLists.txt`:

```cmake
enable_testing()
add_executable(test_runner
    third_party/test_harness/test_harness.c
    tests/test_main.c
    tests/test_util_math.c
    src/util_math.c)
target_include_directories(test_runner PRIVATE src third_party/test_harness)
add_test(NAME test_runner COMMAND test_runner)
```

For every later task that adds tests: append the new `src/<module>.c` and
`tests/test_<module>.c` files to the `test_runner` target sources in `CMakeLists.txt`, and add the
matching `register_<module>_tests();` call in `tests/test_main.c`. Do not leave a test file
created but unregistered.

- [ ] **Step 5: Run tests, expect PASS (math from Task 3 should already work)**

Run: `cmake --build build -j && ctest --test-dir build --output-on-failure`
Expected: 3 passed, 0 failed. If `shape_slide` fails, fix the slide axis-selection in `util_math.c` until it passes.

- [ ] **Step 6: Commit**

```bash
git add third_party/test_harness/ tests/ CMakeLists.txt src/util_math.c
git commit -m "test(util): tiny test harness + math suite"
```

---

### Task 5: Asset cache with refcount

**Goal:** Modules ask for textures/sounds/fonts by string key; `asset.c` deduplicates and refcounts. Cache evicts only on `asset_gc()` (called at maze unload).

**Files:**
- Create: `src/asset.c`, `src/asset.h`

- [ ] **Step 1: Write asset.h**

```c
#pragma once
#include <raylib.h>
#include <stdbool.h>

typedef enum { ASSET_TEXTURE, ASSET_SOUND, ASSET_MUSIC, ASSET_FONT } AssetKind;

void  asset_init(const char *assets_root);
void  asset_shutdown(void);

Texture2D *asset_acquire_texture(const char *key);   // key is relative to assets_root
Sound     *asset_acquire_sound(const char *key);
Music     *asset_acquire_music(const char *key);
Font      *asset_acquire_font(const char *key, int size, const int *codepoints, int n);

void asset_release(const char *key);                  // -1 refcount
void asset_gc(void);                                  // evict refcount==0 entries
```

- [ ] **Step 2: Write asset.c — open-addressed hashmap keyed by string, value = {kind, refcount, raylib handle}**

Implementation outline (~250 lines):
- Hashmap entries hold `{ key[128], kind, refcount, union{ Texture2D, Sound, Music, Font } payload }`.
- `acquire_*` looks up; on miss, loads from `assets_root + "/" + key` and inserts with refcount 1; on hit, increments refcount and returns the existing pointer.
- `release` finds the entry and decrements refcount; never frees here.
- `gc` walks the table; on refcount==0, calls the appropriate raylib `Unload*` and zeroes the slot.

- [ ] **Step 3: Add to CMakeLists.txt amazeing target**

Append `src/asset.c`. No new test (raylib-dependent; covered indirectly by the smoke test in Task 52).

- [ ] **Step 4: Build to verify it compiles**

Run: `cmake --build build -j`
Expected: clean build.

- [ ] **Step 5: Commit**

```bash
git add src/asset.* CMakeLists.txt
git commit -m "feat(asset): refcounted texture/sound/font cache"
```

---

## Phase B — Localization

### Task 6: `.lang` parser

**Goal:** Parse a `data/strings/<lang>.lang` file (UTF-8) of `key = value` lines with `#` comments. Returns an owned key/value array.

**Files:**
- Create: `src/localization.h`
- Create: `src/localization.c` (parser portion only this task)
- Create: `tests/fixtures/en.lang` (minimal)
- Create: `tests/fixtures/malformed.lang`
- Create: `tests/test_localization.c`

- [ ] **Step 1: Write localization.h**

```c
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct LangEntry { char *key; char *value; } LangEntry;

typedef struct LangTable {
    LangEntry *entries;
    size_t     count;
    size_t     cap;
} LangTable;

bool        loc_parse_file(const char *path, LangTable *out);  // returns false on I/O error
void        loc_table_free(LangTable *t);

bool        loc_load_language(const char *lang_code);   // loads en.lang always + lang.lang
void        loc_shutdown(void);
const char *t(const char *key);                          // never NULL: returns lang -> en -> key
```

- [ ] **Step 2: Write tests/fixtures/en.lang**

```
# slice strings
ui.title.start = Start
ui.title.quit  = Quit
maze.nature.name = Nature
item.seed.name   = Seed
```

- [ ] **Step 3: Write tests/fixtures/malformed.lang**

```
this line has no equals
ok.key = ok value
   # leading whitespace comment
=value_only
key_only=
key.trim   =   trimmed value   
```

- [ ] **Step 4: Write failing tests in tests/test_localization.c**

```c
#include "test_harness.h"
#include "localization.h"
#include <string.h>

void test_parse_basic(void) {
    LangTable t = {0};
    ASSERT(loc_parse_file("tests/fixtures/en.lang", &t));
    ASSERT_EQ_INT(t.count, 4);
    bool found = false;
    for (size_t i = 0; i < t.count; i++)
        if (strcmp(t.entries[i].key, "item.seed.name") == 0 &&
            strcmp(t.entries[i].value, "Seed") == 0) found = true;
    ASSERT(found);
    loc_table_free(&t);
}

void test_parse_malformed_recovers(void) {
    LangTable t = {0};
    ASSERT(loc_parse_file("tests/fixtures/malformed.lang", &t));
    // Lines without =, leading =, trailing = are skipped. "key.trim" trimmed.
    bool found = false;
    for (size_t i = 0; i < t.count; i++)
        if (strcmp(t.entries[i].key, "key.trim") == 0 &&
            strcmp(t.entries[i].value, "trimmed value") == 0) found = true;
    ASSERT(found);
    loc_table_free(&t);
}

void register_localization_tests(void) {
    RUN_TEST(test_parse_basic);
    RUN_TEST(test_parse_malformed_recovers);
}
```

Add `register_localization_tests();` call to `tests/test_main.c`.

- [ ] **Step 5: Implement loc_parse_file in localization.c**

Read whole file, scan line by line. Skip empty/comment lines. Find first `=`, split, trim both sides (drop leading/trailing whitespace). Skip if either side is empty. `strdup` into a growing array.

- [ ] **Step 6: CMake wire test + run**

Add `src/localization.c tests/test_localization.c` to `test_runner` sources. Run `ctest`. Expected: both new tests pass.

- [ ] **Step 7: Commit**

```bash
git add src/localization.* tests/test_localization.c tests/fixtures/*.lang tests/test_main.c CMakeLists.txt
git commit -m "feat(loc): .lang parser with malformed-line recovery"
```

---

### Task 7: `t(key)` hashmap with fallback chain

**Goal:** `loc_load_language` builds a hashmap; `t()` returns active → English → literal key. Never returns NULL.

**Files:**
- Modify: `src/localization.c` — add hashmap + t() + load chain.
- Modify: `tests/test_localization.c` — add lookup tests.
- Create: `tests/fixtures/de.lang` (partial translation).

- [ ] **Step 1: Write tests/fixtures/de.lang (partial translation)**

```
ui.title.start = Start (DE)
# intentionally missing ui.title.quit so fallback is exercised
```

- [ ] **Step 2: Add failing tests**

```c
void test_t_falls_back_to_english(void) {
    ASSERT(loc_load_language_from("tests/fixtures/", "de"));   // loads en.lang + de.lang
    ASSERT_EQ_INT(strcmp(t("ui.title.start"), "Start (DE)"), 0);
    ASSERT_EQ_INT(strcmp(t("ui.title.quit"),  "Quit"), 0);     // English fallback
    ASSERT_EQ_INT(strcmp(t("missing.key"),    "missing.key"), 0); // literal key
    loc_shutdown();
}
```

Add prototype for `loc_load_language_from(strings_dir, lang_code)` to localization.h.

- [ ] **Step 3: Implement open-addressed hashmap and lookup chain**

In localization.c, maintain two tables: `g_en` (always loaded) and `g_active` (the chosen language, may equal en). `t()`: lookup in active, else en, else return key. Lookup uses FNV-1a hash on the key, linear probe.

- [ ] **Step 4: Run tests, expect PASS**

Run: `ctest --test-dir build --output-on-failure`

- [ ] **Step 5: Commit**

```bash
git add src/localization.c src/localization.h tests/test_localization.c tests/fixtures/de.lang
git commit -m "feat(loc): t() with active-then-english-then-key fallback"
```

---

### Task 8: OS locale detection at boot

**Goal:** First-launch picks a `.lang` file matching the OS locale if available, else English. Player's subsequent choice persists in `settings`.

**Files:**
- Modify: `src/localization.c`, `src/localization.h`

- [ ] **Step 1: Add API**

```c
// Returns a two-letter language code matching the OS locale, or "en".
// Caller copies into its own storage.
const char *loc_detect_os_lang(void);
```

- [ ] **Step 2: Implement detection**

Include `<windows.h>` under `_WIN32`.

```c
const char *loc_detect_os_lang(void) {
#ifdef _WIN32
    static char buf[16];
    wchar_t wide[16];
    if (GetUserDefaultLocaleName(wide, (int)(sizeof wide / sizeof wide[0])) > 0 &&
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, buf, (int)sizeof buf, NULL, NULL) > 0) {
        buf[2] = '\0';
        return buf;
    }
    return "en";
#else
    const char *env = getenv("LANG"); if (!env) env = getenv("LC_ALL");
    static char out[3] = "en";
    if (env && strlen(env) >= 2) { out[0] = env[0]; out[1] = env[1]; out[2] = 0; }
    return out;
#endif
}
```

- [ ] **Step 3: No test (env-dependent); manual smoke**

Run main, set `LANG=de_DE.UTF-8 ./build/amazeing`. Confirm via LOGI it picked `de`. With `LANG=fr ./build/amazeing` and no `fr.lang` present, `loc_load_language` falls through to `en` (English-only table).

- [ ] **Step 4: Commit**

```bash
git add src/localization.c src/localization.h
git commit -m "feat(loc): detect OS language at boot"
```

---

## Phase C — Data manifests

### Task 9: `data/items.txt` parser

**Goal:** Parse item definitions (§4.2) into an in-memory `ItemDef` array. Inventory UI and save reads consume this single source.

**Files:**
- Create: `src/items.c`, `src/items.h`
- Create: `tests/fixtures/items.txt`
- Create: `tests/test_items.c`

- [ ] **Step 1: Write items.h**

```c
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef enum { SCOPE_MAZE, SCOPE_PROFILE } ItemScope;
typedef enum { KIND_OUTFIT, KIND_TOOL, KIND_CONSUMABLE, KIND_ACCESS } ItemKind;

typedef struct ItemDef {
    char       id[48];
    char       name_key[64];
    char       desc_key[64];
    char       icon_path[96];
    ItemScope  scope;
    ItemKind   kind;
    char       use_tag[32];   // empty string if absent
} ItemDef;

bool        items_load(const char *path);    // appends to global registry
const ItemDef *items_find(const char *id);
size_t        items_count(void);
const ItemDef *items_at(size_t i);
void          items_shutdown(void);
```

- [ ] **Step 2: Write tests/fixtures/items.txt**

```
item:        rain_boots
name:        item.rain_boots.name
desc:        item.rain_boots.desc
scope:       profile
kind:        outfit
icon:        ui/icons/rain_boots.png

item:        seed
name:        item.seed.name
desc:        item.seed.desc
scope:       maze
kind:        consumable
icon:        ui/icons/seed.png

item:        watering_can
name:        item.watering_can.name
scope:       profile
kind:        tool
icon:        ui/icons/watering_can.png
use_tag:     water_plant
```

- [ ] **Step 3: Failing tests in tests/test_items.c**

```c
#include "test_harness.h"
#include "items.h"
#include <string.h>

void test_items_load(void) {
    ASSERT(items_load("tests/fixtures/items.txt"));
    ASSERT_EQ_INT(items_count(), 3);
    const ItemDef *d = items_find("watering_can");
    ASSERT(d != NULL);
    ASSERT_EQ_INT(d->scope, SCOPE_PROFILE);
    ASSERT_EQ_INT(d->kind,  KIND_TOOL);
    ASSERT_EQ_INT(strcmp(d->use_tag, "water_plant"), 0);
    items_shutdown();
}

void test_items_unknown_returns_null(void) {
    ASSERT(items_load("tests/fixtures/items.txt"));
    ASSERT(items_find("not_a_thing") == NULL);
    items_shutdown();
}

void register_items_tests(void) {
    RUN_TEST(test_items_load);
    RUN_TEST(test_items_unknown_returns_null);
}
```

Add to `tests/test_main.c`.

- [ ] **Step 4: Implement items.c**

Parser pattern: read lines; `item:` line starts a new record; `name:`, `desc:`, `scope:`, `kind:`, `icon:`, `use_tag:` set fields. Blank line or new `item:` flushes. Validate `scope` ∈ {`profile`,`maze`}, `kind` ∈ {`outfit`,`tool`,`consumable`,`access`}; unknown → LOGW + skip.

- [ ] **Step 5: Wire to CMake, run tests, expect PASS**

Add `src/items.c tests/test_items.c` to `test_runner` sources.

- [ ] **Step 6: Commit**

```bash
git add src/items.* tests/test_items.c tests/fixtures/items.txt tests/test_main.c CMakeLists.txt
git commit -m "feat(items): parse data/items.txt into ItemDef registry"
```

---

### Task 10: `data/dialogs/*.txt` parser

**Goal:** Parse dialog definitions (§4.3) into a `DialogDef` registry keyed by dialog id.

**Files:**
- Create: `src/dialogs.c`, `src/dialogs.h`
- Create: `tests/fixtures/dialogs.txt`
- Create: `tests/test_dialogs.c`

- [ ] **Step 1: Write dialogs.h**

```c
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef enum { REPEAT_ONCE, REPEAT_ONCE_PER_DAY, REPEAT_ALWAYS } DialogRepeat;

typedef struct {
    char id[64];
    char speaker_key[64];          // may be empty
    char lines[8][96];             // localization keys
    int  line_count;
    DialogRepeat repeat;
    char post_state[96];           // raw, e.g. "give_item:winter_coat"; empty if none
} DialogDef;

bool             dialogs_load(const char *path);
const DialogDef *dialogs_find(const char *id);
void             dialogs_shutdown(void);
```

- [ ] **Step 2: Write tests/fixtures/dialogs.txt**

```
dialog:      npc.beaver.idle
speaker:
lines:
  npc.beaver.idle.l1
  npc.beaver.idle.l2
repeat:      once_per_day

dialog:      npc.gardener.greet
speaker:     npc.gardener.name
lines:
  npc.gardener.greet.l1
repeat:      always
post_state:  give_item:rain_boots
```

- [ ] **Step 3: Failing tests**

```c
void test_dialogs_load(void) {
    ASSERT(dialogs_load("tests/fixtures/dialogs.txt"));
    const DialogDef *d = dialogs_find("npc.gardener.greet");
    ASSERT(d != NULL);
    ASSERT_EQ_INT(d->repeat, REPEAT_ALWAYS);
    ASSERT_EQ_INT(strcmp(d->post_state, "give_item:rain_boots"), 0);
    ASSERT_EQ_INT(d->line_count, 1);
    dialogs_shutdown();
}

void test_dialogs_repeat_once_per_day(void) {
    ASSERT(dialogs_load("tests/fixtures/dialogs.txt"));
    const DialogDef *d = dialogs_find("npc.beaver.idle");
    ASSERT_EQ_INT(d->repeat, REPEAT_ONCE_PER_DAY);
    ASSERT_EQ_INT(d->line_count, 2);
    dialogs_shutdown();
}
```

- [ ] **Step 4: Implement dialogs.c**

`lines:` collects indented (`  `) keys until next blank/non-indented line. `repeat:` token → enum. Unknown `repeat:` → LOGW + default to `always`.

- [ ] **Step 5: Wire + run tests, expect PASS**

- [ ] **Step 6: Commit**

```bash
git add src/dialogs.* tests/test_dialogs.c tests/fixtures/dialogs.txt CMakeLists.txt
git commit -m "feat(dialogs): parse data/dialogs/*.txt into DialogDef registry"
```

---

### Task 11: Sprite asset `manifest.txt` parser

**Goal:** Parse a theme's per-sprite manifest (§4.4) into a `SpriteDef` table.

**Files:**
- Create: `src/sprite_manifest.c`, `src/sprite_manifest.h`
- Create: `tests/fixtures/manifest.txt`
- Add tests to `tests/test_items.c` or create `tests/test_sprite_manifest.c`

- [ ] **Step 1: Write sprite_manifest.h**

```c
#pragma once
#include "util_math.h"
#include <stdbool.h>

typedef enum { LAYER_OBJECT, LAYER_DECOR, LAYER_CHARACTER, LAYER_HUD } SpriteLayer;

typedef struct SpriteDef {
    int         id;                 // stable index in the loaded manifest table
    char        name[64];           // e.g. "tree_oak.png" (also asset key)
    float       origin_x, origin_y;
    float       foot_x, foot_y;
    Shape       collide;
    Shape       proximity;
    float       shadow_h;
    SpriteLayer layer;
    float       interact_x, interact_y;
} SpriteDef;

bool             sprites_load(const char *path);
const SpriteDef *sprites_find(const char *name);
void             sprites_shutdown(void);
```

- [ ] **Step 2: Write tests/fixtures/manifest.txt**

```
sprite:        tree_oak.png
origin:        32, 96
foot:          0, 0
collide:       rect 8 0 16 8
proximity:     rect -4 -4 24 20
shadow_h:      1.4
layer:         object
interact:      0, -12

sprite:        fountain.png
origin:        32, 64
collide:       rect 0 -4 32 12
proximity:     rect -8 -8 40 30
shadow_h:      0.6
layer:         object
```

- [ ] **Step 3: Failing test**

```c
void test_sprite_load(void) {
    ASSERT(sprites_load("tests/fixtures/manifest.txt"));
    const SpriteDef *s = sprites_find("tree_oak.png");
    ASSERT(s != NULL);
    ASSERT_NEAR(s->shadow_h, 1.4f, 0.0001f);
    ASSERT_EQ_INT(s->layer, LAYER_OBJECT);
    ASSERT_EQ_INT(s->collide.kind, SHAPE_RECT);
    ASSERT_NEAR(s->collide.w, 16.0f, 0.001f);
    sprites_shutdown();
}
```

- [ ] **Step 4: Implement parser**

Token-based. `collide:` and `proximity:` accept `rect ox oy w h` or `capsule ox oy w h`. Optional fields default sensibly: `proximity` defaults to `collide` if absent, `shadow_h` to 1.0, `layer` to `object`, `interact` to `0,0`.
Assign `SpriteDef.id` to the manifest-table index when each sprite is flushed.

- [ ] **Step 5: Run tests + commit**

```bash
git add src/sprite_manifest.* tests/test_sprite_manifest.c tests/fixtures/manifest.txt CMakeLists.txt
git commit -m "feat(sprites): parse theme manifest.txt"
```

---

### Task 12: `.maze` file parser

**Goal:** Parse a maze file (§3.6) — header, tile grid, entities (with `behavior#stable_id` prefix), audio zones — into a `MazeFile` struct. Per-instance shape overrides supported.

**Files:**
- Create: `src/maze_data.c`, `src/maze_data.h`
- Create: `tests/fixtures/mini.maze`
- Create: `tests/test_maze_data.c`

- [ ] **Step 1: Write maze_data.h**

```c
#pragma once
#include "util_math.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    char     behavior[32];                  // registered behavior: "pickup", "door", "npc", ...
    char     stable_id[48];
    int      tile_x, tile_y;
    char     args[8][48];                   // "sprite:seed_pickup.png", "item:seed", ...
    int      arg_count;
    bool     has_shape_override;
    Shape    shape_override;                // applied to proximity/collide on attach
} MazeEntityLine;

typedef struct {
    int  x0, y0, x1, y1;                    // tile-coord rect, inclusive
    char clip[64];                          // "water_creek.ogg"
} AudioZoneLine;

typedef struct MazeFile {
    char     name_key[64];                  // "maze.nature.name"
    char     theme[32];                     // "nature"
    char     maze_id[32];                   // derived from filename, e.g. "nature"
    int      width, height;
    int      start_x, start_y;
    int      time_start_min;                // minutes since midnight
    char     ambient[4][48];                // base ambient clips
    int      ambient_count;
    char    *tiles;                         // width*height bytes
    MazeEntityLine *entities;               // owned
    int      entity_count;
    AudioZoneLine  *zones;
    int      zone_count;
} MazeFile;

bool maze_data_load(const char *path, MazeFile *out);
void maze_data_free(MazeFile *m);
```

- [ ] **Step 2: Write tests/fixtures/mini.maze**

```
name:       maze.test.name
theme:      nature
size:       8x4
start:      1,1
time_start: 08:00
ambient:    birds_loop.ogg, wind_soft.ogg

tiles:
........
.GGGGGG.
.G....G.
........

entities:
pickup#test.seed.a            2 2   item:seed
door#test.gate.b              5 2   needs:rain_boots target:area_b proximity:rect:-10,-10,20,20

audio_zones:
creek 1 1 3 2 texture:water_creek.ogg
```

- [ ] **Step 3: Failing tests**

```c
void test_maze_header(void) {
    MazeFile m = {0};
    ASSERT(maze_data_load("tests/fixtures/mini.maze", &m));
    ASSERT_EQ_INT(m.width, 8); ASSERT_EQ_INT(m.height, 4);
    ASSERT_EQ_INT(m.start_x, 1); ASSERT_EQ_INT(m.start_y, 1);
    ASSERT_EQ_INT(m.time_start_min, 8 * 60);
    ASSERT_EQ_INT(m.ambient_count, 2);
    maze_data_free(&m);
}

void test_maze_entities(void) {
    MazeFile m = {0};
    ASSERT(maze_data_load("tests/fixtures/mini.maze", &m));
    ASSERT_EQ_INT(m.entity_count, 2);
    ASSERT_EQ_INT(strcmp(m.entities[0].behavior, "pickup"), 0);
    ASSERT_EQ_INT(strcmp(m.entities[0].stable_id, "test.seed.a"), 0);
    ASSERT_EQ_INT(m.entities[0].tile_x, 2);
    ASSERT_EQ_INT(strcmp(m.entities[1].args[0], "needs:rain_boots"), 0);
    ASSERT(m.entities[1].has_shape_override);
    ASSERT_EQ_INT(m.entities[1].shape_override.kind, SHAPE_RECT);
    maze_data_free(&m);
}

void test_maze_tiles(void) {
    MazeFile m = {0};
    ASSERT(maze_data_load("tests/fixtures/mini.maze", &m));
    ASSERT_EQ_INT(m.tiles[1 * 8 + 1], 'G');   // top-left grass
    ASSERT_EQ_INT(m.tiles[0 * 8 + 0], '.');
    maze_data_free(&m);
}

void test_maze_zones(void) {
    MazeFile m = {0};
    ASSERT(maze_data_load("tests/fixtures/mini.maze", &m));
    ASSERT_EQ_INT(m.zone_count, 1);
    ASSERT_EQ_INT(m.zones[0].x0, 1);
    ASSERT_EQ_INT(strcmp(m.zones[0].clip, "water_creek.ogg"), 0);
    maze_data_free(&m);
}
```

- [ ] **Step 4: Implement maze_data.c**

State machine: read header (`key: value`) until `tiles:`. Then read exactly `height` lines of literal map data (validate length == width). Then `entities:` section: parse `behavior#stable_id   x y   arg arg ...`. Then `audio_zones:` section. `maze_id` is the filename minus extension.

Header parser supports `size: WxH`, `start: X,Y`, `time_start: HH:MM`, `ambient: a.ogg, b.ogg`. Unknown keys logged as warnings.

Per-instance shape overrides are single `key:value` tokens: `collide:rect:OX,OY,W,H`,
`collide:capsule:OX,OY,W,H`, `proximity:rect:OX,OY,W,H`, or
`proximity:capsule:OX,OY,W,H`. Commas separate numeric fields inside the value so the main
entity parser can keep whitespace tokenization simple.

- [ ] **Step 5: Run tests + commit**

```bash
git add src/maze_data.* tests/test_maze_data.c tests/fixtures/mini.maze CMakeLists.txt
git commit -m "feat(maze): parse .maze header, tiles, entities, audio_zones"
```

---

## Phase D — Save

### Task 13: SQLite amalgamation + save_open/close

**Goal:** Vendor SQLite, build it as a static lib, expose `save_open(path)`/`save_close()` with `PRAGMA integrity_check` + timestamped backup on failure.

**Files:**
- Add: `third_party/sqlite/sqlite3.c`, `sqlite3.h` (downloaded amalgamation)
- Create: `src/save.c`, `src/save.h`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Download the SQLite amalgamation**

```bash
mkdir -p third_party/sqlite
curl -L -o /tmp/sqlite.zip https://www.sqlite.org/2024/sqlite-amalgamation-3450000.zip
unzip -j /tmp/sqlite.zip -d third_party/sqlite \
    sqlite-amalgamation-3450000/sqlite3.c \
    sqlite-amalgamation-3450000/sqlite3.h
```

Treat the vendored files as opaque — never edit them.

- [ ] **Step 2: Add sqlite static library to CMakeLists.txt**

```cmake
add_library(sqlite3 STATIC third_party/sqlite/sqlite3.c)
target_compile_definitions(sqlite3 PUBLIC
    SQLITE_THREADSAFE=0
    SQLITE_OMIT_LOAD_EXTENSION=1
    SQLITE_DEFAULT_WAL_SYNCHRONOUS=1)
target_include_directories(sqlite3 PUBLIC third_party/sqlite)
if(NOT MSVC)
    target_compile_options(sqlite3 PRIVATE -w)
endif()
```

Add `sqlite3` as link dep to `amazeing` and `test_runner`.

- [ ] **Step 3: Write save.h**

```c
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CKPT_ITEM_PICKED, CKPT_AREA_TRANSITION, CKPT_DIALOG_DONE,
    CKPT_DAY_ROLLED,  CKPT_PAUSE,           CKPT_QUIT
} CheckpointReason;

bool save_open(const char *path);
void save_close(void);
bool save_integrity_check(void);
bool save_backup(const char *reason);
```

- [ ] **Step 4: Implement save.c — open + close + integrity + backup**

Open via `sqlite3_open`; set `PRAGMA journal_mode=WAL;` and `PRAGMA foreign_keys=ON;`. Close calls `sqlite3_wal_checkpoint_v2(..., TRUNCATE, ...)` then `sqlite3_close`. `save_integrity_check` prepares `PRAGMA integrity_check`, reads first row, returns true iff text is "ok".

Backup: use the SQLite online-backup API (`sqlite3_backup_init/step/finish`) — copies via the engine, handles WAL files correctly, doesn't require closing the source.

```c
bool save_backup(const char *reason) {
    time_t t = time(NULL);
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char dst[1024];
    snprintf(dst, sizeof dst, "%s.%04d%02d%02d-%02d%02d%02d.bak",
             g_db_path, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);
    sqlite3 *out = NULL;
    if (sqlite3_open(dst, &out) != SQLITE_OK) return false;
    sqlite3_backup *b = sqlite3_backup_init(out, "main", g_db, "main");
    if (!b) { sqlite3_close(out); return false; }
    sqlite3_backup_step(b, -1);
    sqlite3_backup_finish(b);
    sqlite3_close(out);
    LOGW("save backup written (%s): %s", reason, dst);
    return true;
}
```

Cache `g_db_path` set during `save_open`.

- [ ] **Step 5: Roundtrip test in tests/test_save_roundtrip.c**

```c
#include "test_harness.h"
#include "save.h"
#include <stdio.h>

void test_open_creates_db(void) {
    remove("/tmp/amazeing_test.db");
    ASSERT(save_open("/tmp/amazeing_test.db"));
    ASSERT(save_integrity_check());
    save_close();
}

void register_save_tests(void) { RUN_TEST(test_open_creates_db); }
```

- [ ] **Step 6: Run + commit**

```bash
git add third_party/sqlite/ src/save.* tests/test_save_roundtrip.c CMakeLists.txt
git commit -m "feat(save): vendor sqlite, integrity check, open/close"
```

---

### Task 14: Schema + migrations

**Goal:** Embed the §3.9 schema, run missing migrations under one transaction keyed by `PRAGMA user_version`.

**Files:**
- Modify: `src/save.c`, `src/save.h`
- Create: `tests/test_save_migrations.c`

- [ ] **Step 1: Embed migration array**

```c
static const char *const MIGRATIONS[] = {
    "CREATE TABLE profiles ("
    "  id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE,"
    "  created_at INTEGER NOT NULL, last_played INTEGER NOT NULL);"
    "CREATE TABLE settings ("
    "  profile_id INTEGER REFERENCES profiles(id) ON DELETE CASCADE,"
    "  key TEXT NOT NULL, value TEXT NOT NULL,"
    "  PRIMARY KEY (profile_id, key));"
    "CREATE TABLE profile_items ("
    "  profile_id INTEGER REFERENCES profiles(id) ON DELETE CASCADE,"
    "  item_id TEXT NOT NULL, acquired_at INTEGER NOT NULL,"
    "  PRIMARY KEY (profile_id, item_id));"
    "CREATE TABLE outfit_worn ("
    "  profile_id INTEGER PRIMARY KEY REFERENCES profiles(id) ON DELETE CASCADE,"
    "  outfit_id TEXT NOT NULL);"
    "CREATE TABLE maze_inventory ("
    "  profile_id INTEGER REFERENCES profiles(id) ON DELETE CASCADE,"
    "  maze_id TEXT NOT NULL, item_id TEXT NOT NULL,"
    "  count INTEGER NOT NULL DEFAULT 1, acquired_at INTEGER NOT NULL,"
    "  PRIMARY KEY (profile_id, maze_id, item_id));"
    "CREATE TABLE maze_progress ("
    "  profile_id INTEGER REFERENCES profiles(id) ON DELETE CASCADE,"
    "  maze_id TEXT NOT NULL, state TEXT NOT NULL,"
    "  completed_at INTEGER, PRIMARY KEY (profile_id, maze_id));"
    "CREATE TABLE maze_snapshot ("
    "  profile_id INTEGER, maze_id TEXT,"
    "  player_x REAL NOT NULL, player_y REAL NOT NULL,"
    "  facing INTEGER NOT NULL,"
    "  clock_min INTEGER NOT NULL, day_index INTEGER NOT NULL,"
    "  PRIMARY KEY (profile_id, maze_id),"
    "  FOREIGN KEY (profile_id) REFERENCES profiles(id) ON DELETE CASCADE);"
    "CREATE TABLE entity_overrides ("
    "  profile_id INTEGER, maze_id TEXT, stable_id TEXT NOT NULL,"
    "  taken INTEGER NOT NULL DEFAULT 0, active INTEGER NOT NULL DEFAULT 0,"
    "  last_dialog_day INTEGER NOT NULL DEFAULT -1, state_blob BLOB,"
    "  PRIMARY KEY (profile_id, maze_id, stable_id),"
    "  FOREIGN KEY (profile_id) REFERENCES profiles(id) ON DELETE CASCADE);"
};
#define MIGRATION_COUNT ((int)(sizeof MIGRATIONS / sizeof *MIGRATIONS))
```

- [ ] **Step 2: Implement run_migrations**

Read `PRAGMA user_version`. If less than `MIGRATION_COUNT`, `BEGIN`, run each missing migration as one `sqlite3_exec`, bump `PRAGMA user_version`, `COMMIT`. On any failure: `ROLLBACK`, return false. Call from end of `save_open`.

- [ ] **Step 3: Migration test**

```c
#include "test_harness.h"
#include "save.h"
#include "sqlite3.h"
#include <stdio.h>

sqlite3 *save_db_for_tests(void);

void test_migrations_create_schema(void) {
    remove("/tmp/amz_mig.db");
    ASSERT(save_open("/tmp/amz_mig.db"));
    sqlite3_stmt *s = NULL;
    sqlite3_prepare_v2(save_db_for_tests(),
        "SELECT count(*) FROM sqlite_master WHERE name='profiles';",
        -1, &s, NULL);
    ASSERT(sqlite3_step(s) == SQLITE_ROW);
    ASSERT_EQ_INT(sqlite3_column_int(s, 0), 1);
    sqlite3_finalize(s);
    save_close();
}

void test_reopen_is_idempotent(void) {
    ASSERT(save_open("/tmp/amz_mig.db"));
    save_close();
    ASSERT(save_open("/tmp/amz_mig.db"));
    save_close();
}

void register_save_migration_tests(void) {
    RUN_TEST(test_migrations_create_schema);
    RUN_TEST(test_reopen_is_idempotent);
}
```

Expose `save_db_for_tests` in save.h behind `#ifdef AMAZEING_TESTS`. Define `AMAZEING_TESTS` only on the `test_runner` target.

- [ ] **Step 4: Run + commit**

```bash
git add src/save.* tests/test_save_migrations.c tests/test_main.c CMakeLists.txt
git commit -m "feat(save): embedded schema + user_version migrations"
```

---

### Task 15: Typed mutators + read helpers

**Goal:** Internal helpers `save_set_*` / `save_load_*`. Gameplay code never sees SQL.

**Files:**
- Modify: `src/save.c`, `src/save.h`
- Modify: `tests/test_save_roundtrip.c`

- [ ] **Step 1: API additions in save.h**

```c
typedef struct { float x, y; int facing; int clock_min; int day_index; } MazeSnapshot;

bool save_set_clock        (int profile_id, const char *maze_id, int clock_min, int day_index);
bool save_set_position     (int profile_id, const char *maze_id, float x, float y, int facing);
bool save_set_progress     (int profile_id, const char *maze_id, const char *state);
bool save_set_inventory_add(int profile_id, const char *maze_id, const char *item_id, int delta);
bool save_set_profile_item (int profile_id, const char *item_id);
bool save_set_outfit_worn  (int profile_id, const char *outfit_id);
bool save_set_entity_override(int profile_id, const char *maze_id, const char *stable_id,
                              int taken, int active, int last_dialog_day);

bool save_load_maze_snapshot (int profile_id, const char *maze_id, MazeSnapshot *out);
int  save_load_profile_items (int profile_id, char ids[][48], int max);
int  save_load_maze_inventory(int profile_id, const char *maze_id, char ids[][48], int max);
bool save_load_maze_inventory_count(int profile_id, const char *maze_id,
                                    const char *item_id, int *out_count);
bool save_load_outfit_worn   (int profile_id, char out[48]);
```

- [ ] **Step 2: Implement using prepared statements cached in static slots**

Lazy `sqlite3_prepare_v2` on first call, store in static `sqlite3_stmt*`, `sqlite3_reset` between calls.
Each mutator returns `true` only when every prepare/bind/step succeeds (`sqlite3_step == SQLITE_DONE`).
Inventory-add uses `INSERT ... ON CONFLICT DO UPDATE SET count = count + excluded.count`.
Profile item adds use `INSERT ... ON CONFLICT(profile_id, item_id) DO NOTHING` so resetting a maze
and re-triggering a profile-scope pickup cannot fail on the primary key. Outfit selection uses
`INSERT ... ON CONFLICT(profile_id) DO UPDATE SET outfit_id=excluded.outfit_id`.
`save_close` finalizes all cached statements before closing the DB so reopen tests do not reuse stale
statement handles.

- [ ] **Step 3: Roundtrip test**

```c
void test_inventory_roundtrip(void) {
    remove("/tmp/amz_rt.db");
    ASSERT(save_open("/tmp/amz_rt.db"));
    sqlite3 *db = save_db_for_tests();
    sqlite3_stmt *s = NULL;
    sqlite3_prepare_v2(db,
        "INSERT INTO profiles(name,created_at,last_played) VALUES('p',0,0);",
        -1, &s, NULL);
    sqlite3_step(s); sqlite3_finalize(s);
    int pid = (int)sqlite3_last_insert_rowid(db);
    save_set_inventory_add(pid, "nature", "seed", 1);
    save_set_inventory_add(pid, "nature", "seed", 2);
    char ids[8][48];
    int n = save_load_maze_inventory(pid, "nature", ids, 8);
    ASSERT_EQ_INT(n, 1);
    ASSERT_EQ_INT(strcmp(ids[0], "seed"), 0);
    int count = 0;
    ASSERT(save_load_maze_inventory_count(pid, "nature", "seed", &count));
    ASSERT_EQ_INT(count, 3);
    save_close();
}
```

- [ ] **Step 4: Run + commit**

```bash
git add src/save.* tests/test_save_roundtrip.c
git commit -m "feat(save): typed mutators and read helpers"
```

---

### Task 16: `save_checkpoint(reason)` + `save_reset_maze`

**Goal:** Single transactional entry point gameplay code calls. Reset implements the exact §2.3 table.

**Files:**
- Modify: `src/save.c`, `src/save.h`
- Create: `tests/test_save_reset.c`

- [ ] **Step 1: API additions**

```c
typedef struct CheckpointPayload {
    int           profile_id;
    char          maze_id[32];

    bool          pos_dirty;     MazeSnapshot pos;
    bool          prog_dirty;    char prog_state[16];   // "in_progress" | "complete"

    int           inv_add_count;
    struct { char item_id[48]; int delta; } inv_adds[16];

    int           ovr_count;
    struct { char stable_id[48]; int taken; int active; int last_dialog_day; }
                  ovrs[32];

    int           profile_item_count;
    char          profile_items[8][48];                  // adds only

    bool          outfit_dirty;  char outfit_id[48];
} CheckpointPayload;

bool save_checkpoint   (const CheckpointPayload *p, CheckpointReason reason);
bool save_reset_maze   (int profile_id, const char *maze_id);
```

- [ ] **Step 2: Implement save_checkpoint**

`BEGIN`. Apply each dirty field via the typed mutators and check every `bool` return. `COMMIT`.
On any false return: `ROLLBACK` and `LOGE`. Log the `reason` enum name.

- [ ] **Step 3: Implement save_reset_maze (§2.3 table)**

```c
bool save_reset_maze(int pid, const char *mid) {
    if (sqlite3_exec(g_db, "BEGIN;", NULL, NULL, NULL) != SQLITE_OK) return false;
    sqlite3_stmt *s = NULL;

    sqlite3_prepare_v2(g_db,
        "DELETE FROM maze_inventory WHERE profile_id=? AND maze_id=?;", -1, &s, NULL);
    sqlite3_bind_int(s, 1, pid); sqlite3_bind_text(s, 2, mid, -1, SQLITE_STATIC);
    sqlite3_step(s); sqlite3_finalize(s);

    sqlite3_prepare_v2(g_db,
        "DELETE FROM entity_overrides WHERE profile_id=? AND maze_id=?;", -1, &s, NULL);
    sqlite3_bind_int(s, 1, pid); sqlite3_bind_text(s, 2, mid, -1, SQLITE_STATIC);
    sqlite3_step(s); sqlite3_finalize(s);

    sqlite3_prepare_v2(g_db,
        "DELETE FROM maze_snapshot WHERE profile_id=? AND maze_id=?;", -1, &s, NULL);
    sqlite3_bind_int(s, 1, pid); sqlite3_bind_text(s, 2, mid, -1, SQLITE_STATIC);
    sqlite3_step(s); sqlite3_finalize(s);

    // Reset progress to in_progress. (Maze-locked-by-missing-item handling is
    // tested separately in test_save_reset.c via the migration scenario.)
    sqlite3_prepare_v2(g_db,
        "INSERT INTO maze_progress(profile_id,maze_id,state) VALUES(?,?, 'in_progress') "
        "ON CONFLICT(profile_id,maze_id) DO UPDATE SET state='in_progress', completed_at=NULL;",
        -1, &s, NULL);
    sqlite3_bind_int(s, 1, pid); sqlite3_bind_text(s, 2, mid, -1, SQLITE_STATIC);
    sqlite3_step(s); sqlite3_finalize(s);

    sqlite3_exec(g_db, "COMMIT;", NULL, NULL, NULL);
    return true;
}
```

The snippet shows the operation order; the real implementation checks every `sqlite3_prepare_v2`,
bind, and `sqlite3_step` result and rolls back on failure. Do not insert a replacement
`maze_snapshot` during reset. The spec requires the snapshot row to be deleted so the next load
respawns at the maze file's `start` position and `time_start` value. The gameplay reset path should
also update the in-memory player position and clock from the loaded `MazeFile` immediately after
this transaction commits.

- [ ] **Step 4: Reset-semantics test**

```c
#include "test_harness.h"
#include "save.h"
#include "sqlite3.h"
#include <stdio.h>
#include <string.h>

sqlite3 *save_db_for_tests(void);

void test_reset_clears_maze_state_preserves_profile(void) {
    remove("/tmp/amz_reset.db");
    ASSERT(save_open("/tmp/amz_reset.db"));
    sqlite3_stmt *s = NULL;
    sqlite3_prepare_v2(save_db_for_tests(),
        "INSERT INTO profiles(name,created_at,last_played) VALUES('p',0,0);",
        -1, &s, NULL); sqlite3_step(s); sqlite3_finalize(s);
    int pid = (int)sqlite3_last_insert_rowid(save_db_for_tests());

    save_set_profile_item(pid, "rain_boots");
    save_set_outfit_worn (pid, "rain_boots");
    save_set_inventory_add(pid, "nature", "seed", 1);
    save_set_entity_override(pid, "nature", "nature.seed.west_glade", 1, 0, -1);

    ASSERT(save_reset_maze(pid, "nature"));

    char ids[8][48];
    ASSERT_EQ_INT(save_load_maze_inventory(pid, "nature", ids, 8), 0);
    ASSERT_EQ_INT(save_load_profile_items(pid, ids, 8), 1);
    ASSERT_EQ_INT(strcmp(ids[0], "rain_boots"), 0);
    char outfit[48]; ASSERT(save_load_outfit_worn(pid, outfit));
    ASSERT_EQ_INT(strcmp(outfit, "rain_boots"), 0);

    MazeSnapshot snap;
    ASSERT(!save_load_maze_snapshot(pid, "nature", &snap));

    save_close();
}

void register_save_reset_tests(void) {
    RUN_TEST(test_reset_clears_maze_state_preserves_profile);
}
```

- [ ] **Step 5: Run + commit**

```bash
git add src/save.* tests/test_save_reset.c tests/test_main.c CMakeLists.txt
git commit -m "feat(save): checkpoint API and reset semantics per spec 2.3"
```

---

### Task 17: Profile module

**Goal:** Up to four named profiles; CRUD via `profile.c` backed by the `profiles` table.

**Files:**
- Create: `src/profile.c`, `src/profile.h`
- Modify: `tests/test_save_roundtrip.c`

- [ ] **Step 1: Write profile.h**

```c
#pragma once
#include <stdbool.h>

#define PROFILE_NAME_MAX 32
#define PROFILE_MAX_COUNT 4

typedef struct { int id; char name[PROFILE_NAME_MAX]; long last_played; } Profile;

int  profile_list  (Profile *out, int max);
int  profile_create(const char *name);          // returns id or -1
bool profile_delete(int id);
bool profile_touch (int id);                    // bump last_played
```

- [ ] **Step 2: Implement using sqlite via save.c**

`profile_create` rejects when `profile_list` already returns 4. Names are unique (DB enforces). `profile_touch` sets `last_played = (long)time(NULL)`.

- [ ] **Step 3: Roundtrip test**

```c
void test_profile_crud(void) {
    remove("/tmp/amz_prof.db");
    ASSERT(save_open("/tmp/amz_prof.db"));
    int a = profile_create("Alice");  ASSERT(a > 0);
    int b = profile_create("Bob");    ASSERT(b > 0);
    Profile ps[4]; int n = profile_list(ps, 4);
    ASSERT_EQ_INT(n, 2);
    ASSERT(profile_delete(a));
    ASSERT_EQ_INT(profile_list(ps, 4), 1);
    save_close();
}

void test_profile_cap(void) {
    remove("/tmp/amz_pcap.db");
    ASSERT(save_open("/tmp/amz_pcap.db"));
    ASSERT(profile_create("a") > 0);
    ASSERT(profile_create("b") > 0);
    ASSERT(profile_create("c") > 0);
    ASSERT(profile_create("d") > 0);
    ASSERT_EQ_INT(profile_create("e"), -1);
    save_close();
}
```

- [ ] **Step 4: Run + commit**

```bash
git add src/profile.* tests/test_save_roundtrip.c CMakeLists.txt
git commit -m "feat(profile): CRUD up to four profiles"
```

---

## Phase E — Input

### Task 18: Action map + buffered sampling

**Goal:** A semantic `Action` enum, a key→action mapping with defaults, and per-frame sampling that yields (a) held-action snapshot and (b) counted edge-action queue. Ticks consume from this buffer.

**Files:**
- Create: `src/input.c`, `src/input.h`

- [ ] **Step 1: Write input.h**

```c
#pragma once
#include <stdbool.h>

typedef enum {
    ACT_MOVE_UP, ACT_MOVE_DOWN, ACT_MOVE_LEFT, ACT_MOVE_RIGHT,
    ACT_INTERACT, ACT_INVENTORY, ACT_PAUSE, ACT_CONFIRM, ACT_CANCEL,
    ACT_COUNT
} Action;

typedef struct {
    bool held[ACT_COUNT];
    int  edge_count[ACT_COUNT];      // counted presses since last consume
} InputFrame;

void input_init(void);                                  // defaults
void input_load_bindings(int profile_id);               // from settings table
bool input_set_binding(Action a, int raylib_key);       // false on reserved-key conflict
int  input_get_binding(Action a);
void input_reset_to_defaults(void);
bool input_missing_required(void);                      // true if any required unbound

void input_sample(void);                                // call once per rendered frame
void input_consume(InputFrame *out);                    // call once per fixed tick
```

- [ ] **Step 2: Implement defaults and sampling**

Defaults: WASD for move, `E` for interact, `I` for inventory, `Esc` for pause, `Enter` for confirm. Reserved: `KEY_ESCAPE` is permanently bound to `ACT_PAUSE`; `KEY_ENTER` defaults to `ACT_CONFIRM` and is unbindable from menus (still settable as a secondary).

`input_sample` is called between rendered frames: for each action, set `held[a] = IsKeyDown(binding[a])`; on a fresh `IsKeyPressed`, increment `edge_count[a]` in the internal buffer. `input_consume` copies the buffer into `*out` and zeroes the edge counts. The held snapshot is replaced each call.

- [ ] **Step 3: Wire to CMake amazeing target**

Add `src/input.c`.

- [ ] **Step 4: Build**

Run: `cmake --build build -j`. Expected: clean.

- [ ] **Step 5: Commit**

```bash
git add src/input.* CMakeLists.txt
git commit -m "feat(input): action map + buffered edges + held sampling"
```

---

### Task 19: Remap rules, persistence, recovery

**Goal:** Implement §2.15 binding rules: one key per action, swap-or-cancel prompt, reserved keys, required-action validation, recovery from broken sets, persistence in `settings` table.

**Files:**
- Modify: `src/input.c`, `src/input.h`
- Create: `tests/test_input_remap.c`

- [ ] **Step 1: API additions**

```c
typedef enum { REMAP_OK, REMAP_RESERVED, REMAP_SWAP_REQUIRED } RemapStatus;

RemapStatus input_propose_binding(Action a, int raylib_key, Action *displaces_out);
void        input_apply_swap(Action a, int raylib_key, Action displaced);
void        input_save_bindings(int profile_id);
```

- [ ] **Step 2: Failing tests**

```c
#include "test_harness.h"
#include "input.h"

void test_swap_required_on_collision(void) {
    input_reset_to_defaults();
    Action other;
    // Default INTERACT is 'E'. Try to bind MOVE_UP to E.
    RemapStatus s = input_propose_binding(ACT_MOVE_UP, /* KEY_E */ 69, &other);
    ASSERT_EQ_INT(s, REMAP_SWAP_REQUIRED);
    ASSERT_EQ_INT(other, ACT_INTERACT);
    input_apply_swap(ACT_MOVE_UP, 69, other);
    ASSERT_EQ_INT(input_get_binding(ACT_MOVE_UP), 69);
    ASSERT_EQ_INT(input_get_binding(ACT_INTERACT), 0);   // 0 == unbound
}

void test_reserved_escape(void) {
    input_reset_to_defaults();
    Action other;
    RemapStatus s = input_propose_binding(ACT_MOVE_UP, /* KEY_ESCAPE */ 256, &other);
    ASSERT_EQ_INT(s, REMAP_RESERVED);
}

void test_missing_required_after_unbind(void) {
    input_reset_to_defaults();
    Action other;
    input_propose_binding(ACT_INTERACT, /* KEY_W */ 87, &other);
    input_apply_swap(ACT_INTERACT, 87, ACT_MOVE_UP);
    // MOVE_UP is now unbound; required-action check should flag it.
    ASSERT(input_missing_required());
}

void test_load_recovery_from_corrupt(void) {
    /* Simulate: settings table has bindings missing required actions.
       input_load_bindings logs a warning and falls back to defaults. */
    input_reset_to_defaults();
    int interact_default = input_get_binding(ACT_INTERACT);
    /* call internal hook to seed corrupt state */
    input_load_corrupt_for_tests();
    ASSERT_EQ_INT(input_get_binding(ACT_INTERACT), interact_default);
}

void register_input_remap_tests(void) {
    RUN_TEST(test_swap_required_on_collision);
    RUN_TEST(test_reserved_escape);
    RUN_TEST(test_missing_required_after_unbind);
    RUN_TEST(test_load_recovery_from_corrupt);
}
```

- [ ] **Step 3: Implement remap + persistence + recovery**

- `input_propose_binding`: returns `REMAP_RESERVED` if key is `KEY_ESCAPE` (or, for menu-context, `KEY_ENTER`). If another action already uses the key, returns `REMAP_SWAP_REQUIRED` with the displaced action.
- `input_apply_swap`: sets the new binding and unbinds the displaced action.
- `input_save_bindings`: writes one `settings` row per action (`key="binding.move_up"`, `value="87"`, etc.) under one transaction.
- `input_load_bindings`: reads those rows; if any required action ends up unbound, calls `input_reset_to_defaults()` and `LOGW`. The Settings UI later surfaces the non-blocking notice (see Task 46).

`input_load_corrupt_for_tests` is an `#ifdef AMAZEING_TESTS` helper that simulates a missing required binding and then invokes the recovery path.

- [ ] **Step 4: Run tests, expect PASS**

- [ ] **Step 5: Commit**

```bash
git add src/input.* tests/test_input_remap.c tests/test_main.c CMakeLists.txt
git commit -m "feat(input): remap rules, persistence, and recovery"
```

---

## Phase F — Game core

### Task 20: `Game` struct and state machine

**Goal:** Top-level `Game` aggregate; state enum and dispatch table for the menu states (title, profile, maze_select, in_maze, pause, inventory, complete).

**Files:**
- Create: `src/game.c`, `src/game.h`
- Modify: `src/main.c`

- [ ] **Step 1: Write game.h**

```c
#pragma once
#include "input.h"
#include "save.h"
#include <stdbool.h>

typedef enum {
    GS_TITLE, GS_PROFILE, GS_MAZE_SELECT, GS_IN_MAZE,
    GS_PAUSE, GS_INVENTORY, GS_DIALOG, GS_COMPLETE
} GameState;

typedef struct Game {
    GameState state;
    int       active_profile_id;
    bool      quit_requested;
    bool      auto_wear_first;       // default true in game_create; applies outfit unlocks immediately
    CheckpointPayload pending_checkpoint;
    /* Later subsystem tasks extend this concrete struct with World, Player,
       Clock, GameCamera, audio, and UI fields before tests/tools access them. */
} Game;

Game *game_create(void);
void  game_destroy(Game *g);

void  game_set_state(Game *g, GameState s);
GameState game_state(const Game *g);

// Per-tick update; ticks dispatch to the active state's handler.
void  game_tick(Game *g, const InputFrame *in, float dt);

// Render driver (called between tick batches).
void  game_render(Game *g, float alpha);   // alpha is interpolation between prev and current tick
```

- [ ] **Step 2: Implement state machine dispatch**

`Game` is intentionally concrete in `game.h` because tests and headless tools stack-allocate `Game`
and inspect `g.world`, `g.player`, and similar fields. Start with the minimal fields above;
`game_create` sets `auto_wear_first = true` so outfit pickups and dialog grants can immediately
equip the first newly-earned outfit. Later subsystem tasks modify `game.h` to add `World world`,
`Player player`, `Clock clock`,
`GameCamera camera`, and UI/audio state before any code uses those fields. Tick switches on
`state` and dispatches to per-state tick functions (initially stubs that change state on
`ACT_CONFIRM`/`ACT_CANCEL`).

- [ ] **Step 3: Update main.c to use Game**

Replace boot main with: open save DB, init asset cache, init input defaults + load bindings for default profile (if any), then `Game *g = game_create()`; loop calls `game_tick`/`game_render` (full loop wired in Task 21).

- [ ] **Step 4: Build**

Expected: builds without warnings.

- [ ] **Step 5: Commit**

```bash
git add src/game.* src/main.c CMakeLists.txt
git commit -m "feat(game): Game struct + state-machine dispatch"
```

---

### Task 21: Fixed-step main loop with accumulator

**Goal:** Implement the §3.3 update pipeline: sample input once per rendered frame, drain a 60 Hz accumulator (max 5 catch-up ticks), render once at the end with optional interpolation alpha.

**Files:**
- Modify: `src/main.c`
- Modify: `src/game.c`, `src/game.h`

- [ ] **Step 1: Wire the loop in main.c**

```c
#define TICK_HZ 60
#define TICK_DT (1.0 / TICK_HZ)
#define MAX_CATCHUP_TICKS 5

int main(void) {
    InitWindow(1280, 720, "A Maze Ing");
    SetExitKey(0);                           // ESC is handled by input as ACT_PAUSE
    SetTargetFPS(0);                         // we control pacing manually
    util_log_init(NULL);

    char db[1024]; util_paths_save_db(db, sizeof db);
    if (!save_open(db)) { LOGE("save_open failed"); return 1; }
    if (!save_integrity_check()) {
        save_backup("integrity_check_failed");
        /* UI dialog covered in Task 53 */
    }
    asset_init("assets");
    loc_load_language("en");
    input_init();

    Game *g = game_create();
    double accum = 0;
    double last = GetTime();

    while (!WindowShouldClose()) {
        double now = GetTime();
        double frame_dt = now - last; last = now;
        accum += frame_dt;
        if (accum > MAX_CATCHUP_TICKS * TICK_DT) accum = MAX_CATCHUP_TICKS * TICK_DT;

        input_sample();
        while (accum >= TICK_DT) {
            InputFrame in; input_consume(&in);
            game_tick(g, &in, (float)TICK_DT);
            accum -= TICK_DT;
        }
        float alpha = (float)(accum / TICK_DT);
        game_render(g, alpha);
    }

    /* CKPT_QUIT and shutdown */
    game_destroy(g);
    asset_shutdown();
    save_close();
    util_log_close();
    CloseWindow();
    return 0;
}
```

- [ ] **Step 2: Render once per frame; ticks call into state-specific handlers**

In `game_tick`, when `state == GS_IN_MAZE`, dispatch to placeholder `in_maze_tick(g, in, dt)` that just moves the player by `dt * speed * direction` (covered fully in Task 25).

- [ ] **Step 3: Manual smoke**

Run `./build/amazeing`. Expected: window opens, title-screen placeholder draws, ESC closes. No crashes.

- [ ] **Step 4: Commit**

```bash
git add src/main.c src/game.* CMakeLists.txt
git commit -m "feat(loop): fixed-step accumulator main loop"
```

---

## Phase G — World simulation

### Task 22: Tile grid + walkable check

**Goal:** `World` owns the tile grid; lookups distinguish walkable ground from blocked tiles. The walkability rule comes from a per-theme tile table.

**Files:**
- Create: `src/world.c`, `src/world.h`
- Modify: `src/game.h` — add `World world;` to the concrete `Game` struct.
- Create: `tests/fixtures/themes/test/tiles.txt`

- [ ] **Step 1: Write world.h**

```c
#pragma once
#include "maze_data.h"
#include <stdbool.h>

typedef struct World {
    MazeFile maze;
    char     theme[32];
    /* tile->walkable lookup: per-char bool, derived from theme/tiles.txt */
    bool     walkable[128];
    /* entity arrays added in Task 23 */
} World;

bool world_load_maze (World *w, const char *maze_path, const char *theme_dir);
void world_unload    (World *w);
bool world_tile_walkable(const World *w, int tx, int ty);
```

- [ ] **Step 2: Theme tile table format**

```
# tiles.txt: char  walkable
. 1
G 1
T 0
W 0
S 1
```

- [ ] **Step 3: Implement loader**

`world_load_maze` calls `maze_data_load`, then parses `<theme_dir>/tiles.txt` and fills `walkable`. Unknown chars default to walkable with a warning.

- [ ] **Step 4: Tests in tests/test_maze_data.c (additional)**

```c
void test_world_walkable(void) {
    World w = {0};
    ASSERT(world_load_maze(&w, "tests/fixtures/mini.maze",
                           "tests/fixtures/themes/test"));
    ASSERT(world_tile_walkable(&w, 0, 0));    // '.'
    ASSERT(!world_tile_walkable(&w, 1, 1));   // 'G' configured non-walkable in fixture
    world_unload(&w);
}
```

(Use a fixture where `G` is non-walkable so the assertion is meaningful. Adjust `tests/fixtures/themes/test/tiles.txt` accordingly.)

- [ ] **Step 5: Run + commit**

```bash
git add src/world.* tests/fixtures/themes/test/tiles.txt tests/test_maze_data.c CMakeLists.txt
git commit -m "feat(world): tile grid + walkable lookup from theme tile table"
```

---

### Task 23: Entity tagged union + behavior registry

**Goal:** Engine-side `Entity` struct (§3.5), tombstone-friendly stable array, and a registry mapping behavior names to `EntityCallbacks`. World load attaches behaviors to each entity line.

**Files:**
- Create: `src/entity.c`, `src/entity.h`
- Modify: `src/world.c`, `src/world.h`

- [ ] **Step 1: Write entity.h**

```c
#pragma once
#include "util_math.h"
#include "items.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum { ENT_ITEM, ENT_HAZARD, ENT_INTERACT, ENT_NPC, ENT_DECOR } EntityKind;

struct Game; struct Entity;

typedef struct EntityCallbacks {
    void (*on_loaded)       (struct Game*, struct Entity*);
    void (*on_player_enter) (struct Game*, struct Entity*);
    void (*on_player_exit)  (struct Game*, struct Entity*);
    void (*on_interact)     (struct Game*, struct Entity*, const char *item_id);
    void (*on_step)         (struct Game*, struct Entity*, float dt);
    void (*on_time_changed) (struct Game*, struct Entity*, int hour);
} EntityCallbacks;

typedef struct Entity {
    EntityKind kind;
    uint32_t   id;
    char       stable_id[48];
    Vec2       pos;
    Shape      collide;
    Shape      proximity;
    int        sprite_id;
    int        shadow_id;
    uint32_t   flags;          /* bit field */
    int8_t     gate_hour_a, gate_hour_b;     // -1 == always
    const EntityCallbacks *cb;
    bool       alive;          /* false => tombstoned */
    union {
        struct { char item_id[48]; ItemScope scope; bool taken; } item;
        struct { int  type; int param; }                          hazard;
        struct { int  target; bool active; }                      interact;
        struct { char dialog_id[64]; int  state; }                npc;
    };
} Entity;

void entity_registry_init(void);
void entity_registry_register(const char *behavior, const EntityCallbacks *cb);
const EntityCallbacks *entity_registry_find(const char *behavior);
```

- [ ] **Step 2: Write entity.c — registry storage**

Open-addressed hashmap keyed by behavior name; values are `const EntityCallbacks *`. Registry entries live as static structs in `entity_item.c`, `entity_hazard.c`, etc., and each module's `entity_*_register()` is called once at boot.

- [ ] **Step 3: Extend world.h/world.c**

```c
#define WORLD_MAX_ENTITIES 1024
struct World {
    /* ... fields from Task 22 ... */
    Entity   entities[WORLD_MAX_ENTITIES];
    int      entity_count;
};
```

`world_load_maze` populates `entities[]` from `maze.entities[]`:
- Find sprite from an explicit `sprite:<name>.png` arg when present. If absent,
  fall back to a behavior-derived sprite key only for simple one-sprite behaviors.
  Slice content must use registered behavior names as prefixes and `sprite:` args
  for visual variants such as `seed_pickup.png` and `locked_gate.png`.
- Apply per-instance overrides from the maze line.
- Resolve behavior callbacks via `entity_registry_find(line->behavior)`.
- Apply args: `item:X` on behavior `pickup` → `kind=ITEM, item.item_id=X`,
  `kind` derived from `items_find`; `sprite:X.png` → sprite/default-shape lookup;
  `gate:H1-H2` → `gate_hour_*`; `needs:Y target:Z` → `kind=INTERACT`;
  `dialog:X` on behavior `npc` → `kind=NPC`; etc.

- [ ] **Step 4: Hook stub registry init from game_create**

Call `entity_registry_init()` and the per-kind register hooks (added in Phase H). For now, register a no-op `decor` behavior so empty maze files load.

- [ ] **Step 5: Build + commit**

```bash
git add src/entity.* src/world.* CMakeLists.txt
git commit -m "feat(entity): tagged union + behavior registry + world load"
```

---

### Task 24: Collision shapes module

**Goal:** Aggregate world collision queries: blocking-shape sweep against player foot; interaction-shape overlap iterator; hazard-region overlap.

**Files:**
- Create: `src/collision.c`, `src/collision.h`
- Create: `tests/test_collision.c`

- [ ] **Step 1: Write collision.h**

```c
#pragma once
#include "world.h"
#include "util_math.h"

typedef bool (*EntityFilter)(const Entity *e);

bool collision_blocked       (const World *w, Shape foot, Vec2 pos);
int  collision_overlap_query (const World *w, Shape s, Vec2 pos,
                              EntityFilter f, Entity **out, int max);
// Resolved slide for the player foot moving from `from` toward `to` along blocking entities + walls.
Vec2 collision_slide_player  (const World *w, Shape foot, Vec2 from, Vec2 to);
```

- [ ] **Step 2: Implement**

`collision_blocked`: iterate `world.entities` whose `collide.w > 0`; `shape_overlap(foot, pos, e->collide, e->pos)` short-circuits. Also tests against tile-walkability for tiles covered by the foot's AABB.

`collision_slide_player`: try desired `to`; if blocked, try axis-decomposed moves (`(from.x, to.y)`, `(to.x, from.y)`) and return the unblocked one; else return `from`.

`collision_overlap_query`: iterates entities, calls filter, returns matches into `out[]`.

- [ ] **Step 3: Tests**

```c
#include "test_harness.h"
#include "collision.h"

void test_blocked_against_entity(void) {
    World w = {0};
    w.entity_count = 1;
    w.entities[0].alive = true;
    w.entities[0].pos = (Vec2){10, 10};
    w.entities[0].collide = (Shape){SHAPE_RECT, -4, -4, 8, 8};
    Shape foot = {SHAPE_RECT, -2, -2, 4, 4};
    ASSERT(collision_blocked(&w, foot, (Vec2){10, 10}));
    ASSERT(!collision_blocked(&w, foot, (Vec2){30, 30}));
}

void test_slide_falls_back_axis(void) {
    World w = {0};
    w.entity_count = 1;
    w.entities[0].alive = true;
    w.entities[0].pos = (Vec2){10, 10};
    w.entities[0].collide = (Shape){SHAPE_RECT, -4, -4, 8, 8};
    Shape foot = {SHAPE_RECT, -2, -2, 4, 4};
    /* Walking diagonally into the entity from below-left should slide along
       the wall on whichever axis is unblocked. */
    Vec2 from = {3, 3};
    Vec2 to   = {7, 7};
    Vec2 out  = collision_slide_player(&w, foot, from, to);
    ASSERT(!collision_blocked(&w, foot, out));
    ASSERT(out.x == to.x || out.y == to.y);
}
```

- [ ] **Step 4: Run + commit**

```bash
git add src/collision.* tests/test_collision.c CMakeLists.txt
git commit -m "feat(collision): blocked-query and axis-slide"
```

---

### Task 25: Player movement

**Goal:** 8-direction movement at constant speed; foot-shape slides against blocking shapes via `collision_slide_player`. Animation state (walk cycle frame, facing) advances.

**Files:**
- Create: `src/player.c`, `src/player.h`
- Modify: `src/game.h` — add `Player player;` after `player.h` exists.
- Modify: `src/game.c`

- [ ] **Step 1: Write player.h**

```c
#pragma once
#include "util_math.h"

typedef struct Player {
    Vec2  pos, prev_pos;
    int   facing;             // 0..7: N, NE, E, SE, S, SW, W, NW
    float speed;              // tiles per second
    int   anim_frame;
    float anim_time;
    char  outfit[48];         // current outfit item id (e.g. "default", "rain_boots")
    char  face[32];           // current expression (e.g. "neutral")
} Player;

struct Game;
void player_init(Player *p, Vec2 start);
void player_tick(struct Game *g, Player *p, const InputFrame *in, float dt);
```

- [ ] **Step 2: Implement player_tick**

Read `held` from `in`. Sum axis directions, normalize if both axes pressed. Compute `desired = pos + dir * speed * dt`. `pos = collision_slide_player(world, foot, pos, desired)`. Update `facing` from `dir`. Advance `anim_time`; on threshold, increment `anim_frame % WALK_FRAMES`.

Hazard application happens in Task 26 (called from `game_tick` after movement).

- [ ] **Step 3: Wire into game_tick GS_IN_MAZE branch**

`player_tick(g, &g->player, in, dt);`

- [ ] **Step 4: Manual smoke**

Build and run. WASD should move the player position (visible after Task 35 wires the camera and Task 37 draws ground). For now, drawing is just a stub circle at `player.pos`.

- [ ] **Step 5: Commit**

```bash
git add src/player.* src/game.c CMakeLists.txt
git commit -m "feat(player): 8-dir movement with collision slide"
```

---

### Task 26: Hazard regions

**Goal:** Water, mud, ice, thorns. Hazards are entities with `kind=ENT_HAZARD`; `on_player_enter`/`on_player_exit` change player velocity modifiers. Ice slides; mud slows; water/thorns block unless the player wears the unlocking outfit/item.

**Files:**
- Create: `src/entity_hazard.c`
- Modify: `src/player.c`, `src/player.h`
- Modify: `src/game.c` — register hazards at boot.

- [ ] **Step 1: Hazard types**

```c
typedef enum { HAZ_WATER, HAZ_MUD, HAZ_ICE, HAZ_THORNS } HazardType;
```

Stored in `entity.hazard.type`. The maze parser maps `hazard:water` etc. to `type`.

- [ ] **Step 2: Implement entity_hazard.c**

Per-hazard callbacks:
- `water`: on `collision_blocked` test (called from `player_tick` after slide), if the player foot overlaps a water hazard and the player isn't wearing waders, revert to `prev_pos`. Use a query-helper because the hazard is overlapping-allowed otherwise (you can walk through fog).
- `mud`: `player.speed_mul *= 0.5` while overlapping; reset on exit. If the maze line
  includes `needs:<outfit_id>`, treat it as deep mud: without that outfit, revert to `prev_pos`
  and show the hazard hint; with the outfit, allow traversal and still apply the slow multiplier.
- `ice`: while overlapping, force `player.velocity` to retain its last direction (sliding) — implement via a `player.sliding` flag set in `on_player_enter` and cleared in `on_player_exit`.
- `thorns`: same pattern as water but unlocking item is `garden_gloves`. (Slice does not exercise thorns; behavior still registered for completeness.)

Register: `entity_hazard_register()` exposes all four hazard behavior names to the registry:
`hazard`, `water`, `mud`, `ice`, and `thorns` should resolve either directly or through `hazard`
plus a `hazard:<type>` / `type:<type>` arg.

- [ ] **Step 3: Test in tests/test_entity_behaviors.c**

```c
void test_water_blocks_without_waders(void) {
    Game g = {0};
    World *w = &g.world;
    w->entity_count = 1;
    w->entities[0] = (Entity){
        .kind = ENT_HAZARD, .alive = true,
        .pos = {10, 10},
        .collide = {SHAPE_RECT, -4, -4, 8, 8},
        .hazard.type = HAZ_WATER,
    };
    /* Player starts dry next to the water. */
    g.player.pos      = (Vec2){4, 10};
    g.player.prev_pos = (Vec2){4, 10};
    strcpy(g.player.outfit, "default");
    /* Apply hazard post-move: pretend player_tick moved into the water. */
    g.player.pos = (Vec2){10, 10};
    hazard_apply_post_move(&g);
    ASSERT_NEAR(g.player.pos.x, 4.0f, 0.01f);   // reverted
}

void test_deep_mud_blocks_without_rain_boots(void) {
    /* Build a mud hazard with needs:rain_boots. Without the outfit, entering
       reverts to prev_pos; with rain_boots equipped, the player remains in the
       region and speed_mul is reduced. */
}
```

`hazard_apply_post_move` is exposed from `entity_hazard.c` so player.c can call it after `collision_slide_player`.

- [ ] **Step 4: Run + commit**

```bash
git add src/entity_hazard.c src/player.* src/game.c tests/test_entity_behaviors.c CMakeLists.txt
git commit -m "feat(hazard): water/mud/ice/thorns behaviors"
```

---

### Task 27: Interaction model

**Goal:** Target selection from proximity + facing cone, eligibility, dispatch via `cb->on_interact(item_id_or_NULL)`. Multi-item case raises a picker (UI in Task 45); single-item auto-uses; no-item shows the target's hint.

**Files:**
- Create: `src/interaction.c`, `src/interaction.h`
- Create: `tests/test_interaction.c`

- [ ] **Step 1: Write interaction.h**

```c
#pragma once
#include "world.h"
#include "player.h"

struct Game;

typedef struct InteractionTarget {
    Entity *e;
    int     applicable_items[16];   // indices into the player's eligible inventory
    int     applicable_count;
} InteractionTarget;

bool interaction_find_target(struct Game *g, InteractionTarget *out);
void interaction_dispatch  (struct Game *g, InteractionTarget *t, const char *item_id_or_null);
// Helper used by the UI picker after the player chooses which item to use.
```

- [ ] **Step 2: Implement**

`interaction_find_target`:
1. Iterate entities whose `proximity.w > 0` and `proximity` overlaps player's foot — call this set A.
2. Compute the player's facing-cone (±30° wedge in front of player for radius `1.5 * TILE`). Entities whose center is inside the wedge form set B.
3. Eligible = A ∪ B. If empty, return false.
4. Sort by distance along facing; ties break by `e->id`.
5. For the chosen target, build `applicable_items` by asking the target which items apply. Targets opt in via a small per-kind helper (`door_accepts(needs_id, player_items)`, `withered_plant_accepts(use_tag, player_items)`, etc.).

`interaction_dispatch`:
- If `item_id_or_null == NULL` and `applicable_count == 0`: show hint via `ui_dialog_show(t(target_hint_key))`.
- If `applicable_count == 1` and called with NULL: auto-use that item.
- Otherwise call `cb->on_interact(g, e, item_id_or_null)`. The target's callback performs the world mutation, then on success calls `save_set_entity_override` etc. via `game_mark_dirty_*`.

- [ ] **Step 3: Tests**

```c
void test_facing_picks_front_entity(void) {
    Game g = {0};
    World *w = &g.world;
    w->entity_count = 2;
    w->entities[0] = (Entity){
        .alive = true, .id = 1,
        .pos = (Vec2){10, 10},
        .proximity = {SHAPE_RECT, -8, -8, 16, 16},
    };
    w->entities[1] = (Entity){
        .alive = true, .id = 2,
        .pos = (Vec2){8, 10},
        .proximity = {SHAPE_RECT, -8, -8, 16, 16},
    };
    g.player.pos = (Vec2){9, 10};
    g.player.facing = 2;   // east
    InteractionTarget t;
    ASSERT(interaction_find_target(&g, &t));
    ASSERT_EQ_INT(t.e->id, 1);    // east of player
}

void test_id_tie_break(void) {
    /* Two equidistant eligible targets; lower id wins. */
}
```

- [ ] **Step 4: Run + commit**

```bash
git add src/interaction.* tests/test_interaction.c CMakeLists.txt
git commit -m "feat(interaction): proximity+facing target selection and dispatch"
```

---

## Phase H — Entity behaviors

### Task 28: Item pickup behavior

**Goal:** `pickup` behavior — picking up moves the item into the right scope (profile vs maze) per `items.txt`. Item tombstones; save state updates via the checkpoint payload.

**Files:**
- Create: `src/entity_item.c`
- Modify: `src/game.c` — register at boot.

- [ ] **Step 1: Implement entity_item.c**

```c
static void item_on_interact(Game *g, Entity *e, const char *item_id) {
    (void)item_id;
    if (e->item.taken) return;
    const ItemDef *d = items_find(e->item.item_id);
    if (!d) { LOGW("unknown item: %s", e->item.item_id); return; }
    if (d->scope == SCOPE_PROFILE) {
        game_dirty_profile_item_add(g, d->id);
        if (d->kind == KIND_OUTFIT && g->auto_wear_first) {
            game_dirty_outfit_set(g, d->id);
        }
    } else {
        game_dirty_inventory_add(g, d->id, 1);
    }
    game_dirty_entity_override(g, e->stable_id, /*taken*/1, /*active*/0,
                               /*last_dialog_day*/-1);
    e->item.taken = true;
    e->alive = false;
    game_request_checkpoint(g, CKPT_ITEM_PICKED);
}

static const EntityCallbacks ITEM_CB = { .on_interact = item_on_interact };

void entity_item_register(void) {
    entity_registry_register("pickup", &ITEM_CB);
}
```

- [ ] **Step 2: Test in tests/test_entity_behaviors.c**

```c
void test_pickup_profile_item(void) {
    /* Build a Game with a profile, run save_open on tmp DB, load items.txt,
       construct an entity with kind=ITEM, scope=PROFILE.
       Call item_on_interact via interaction_dispatch.
       Verify save_load_profile_items contains the item id and entity is alive=false. */
}
```

(Full test setup is repetitive; reuse a `Game *make_test_game(void)` helper to be added to a small test-only `tests/test_game_fixture.c`.)

- [ ] **Step 3: Run + commit**

```bash
git add src/entity_item.c src/game.c tests/test_entity_behaviors.c tests/test_game_fixture.c CMakeLists.txt
git commit -m "feat(entity): pickup behavior with scope-aware persistence"
```

---

### Task 29: Door, sign, sundial behaviors

**Goal:** `door` (locked gate with `needs:<item_id>`), `sign` (hint-only), `sundial` (time-advance affordance).

**Files:**
- Create: `src/entity_interact.c`
- Modify: `src/game.c`

- [ ] **Step 1: Door**

```c
static bool door_accepts(const Entity *e, const char *item_id) {
    /* The maze parser stores the required item id in e->interact target field
       OR a sibling string; encode in a small Door extras struct keyed off
       e->id in a hash table inside entity_interact.c. */
    return strcmp(item_id, door_needs(e)) == 0;
}

static void door_on_interact(Game *g, Entity *e, const char *item_id) {
    if (e->interact.active) return;            /* already open */
    if (!item_id) {
        if (!player_has_item(&g->player, door_needs(e))) {
            ui_dialog_show_key(g, hint_for(e, "hint.door.locked"));
            return;
        }
        item_id = door_needs(e);   /* single applicable item; auto-pick */
    }
    if (!door_accepts(e, item_id)) {
        ui_dialog_show_key(g, hint_for(e, "hint.door.wrong_item"));
        return;
    }
    e->interact.active = true;
    e->collide.w = 0;   /* no longer blocking */
    game_dirty_entity_override(g, e->stable_id, 0, 1, -1);
    game_request_checkpoint(g, CKPT_DIALOG_DONE);
}
```

Extras storage is explicit and reset on maze load/unload:
`entity_interact.c` owns fixed arrays keyed by `Entity.id`, for example
`DoorExtras g_door_extras[WORLD_MAX_ENTITIES]`, `SignExtras g_sign_extras[...]`, and
`SundialExtras g_sundial_extras[...]`. `world_load_maze` fills these after entity attachment from
`needs:`, `target:`, `sign:`, and `gate:` args; `world_unload` calls an
`entity_interact_clear_extras()` helper. `door_needs(e)` reads from this storage.

- [ ] **Step 2: Sign**

```c
static void sign_on_interact(Game *g, Entity *e, const char *item_id) {
    (void)item_id;
    /* sign stores a localization key in extras */
    ui_dialog_show_key(g, sign_key(e));
}
```

- [ ] **Step 3: Sundial**

Time-advance affordance: interacting fades to black and advances clock to the next hour band that unlocks something nearby. Default-from-spec §11 says "fade-to-black with elapsed-hours indicator". Implementation defers the actual fade to the UI layer (Task 32 wires the clock advance API; Task 44 wires the fade).

```c
static void sundial_on_interact(Game *g, Entity *e, const char *item_id) {
    (void)item_id;
    int target = sundial_next_band(g, e);     /* next gate boundary nearby */
    ui_fade_then_advance_clock(g, target);
    game_request_checkpoint(g, CKPT_DAY_ROLLED);
}
```

- [ ] **Step 4: Register all three**

```c
void entity_interact_register(void) {
    entity_registry_register("door",    &DOOR_CB);
    entity_registry_register("sign",    &SIGN_CB);
    entity_registry_register("sundial", &SUNDIAL_CB);
}
```

- [ ] **Step 5: Tests (door rejection preserves item; door accept opens)**

```c
void test_door_rejects_wrong_item(void) {
    /* Build Game with a door needing rain_boots; player has only seed. */
    /* Dispatch with item_id="seed" -> dialog shown, door stays blocking,
       player still has seed (rule 3 in §2.3). */
}

void test_door_accepts_correct_item(void) {
    /* Player has rain_boots. interaction_dispatch finds single applicable
       item, auto-uses, door active=true, collide.w == 0. */
}
```

- [ ] **Step 6: Run + commit**

```bash
git add src/entity_interact.c src/game.c tests/test_entity_behaviors.c CMakeLists.txt
git commit -m "feat(entity): door, sign, sundial behaviors"
```

---

### Task 30: NPC + dialog driver

**Goal:** NPC behavior reads `DialogDef` from `dialogs.c`, plays lines one at a time (text speed handled by UI in Task 44), respects `repeat:` rules.

**Files:**
- Create: `src/entity_npc.c`
- Create: `tests/test_dialog_repeat.c`

- [ ] **Step 1: NPC behavior**

```c
static void npc_on_interact(Game *g, Entity *e, const char *item_id) {
    const DialogDef *d = dialogs_find(e->npc.dialog_id);
    if (!d) return;
    int today = g->clock.day_index;
    int last  = entity_override_last_dialog_day(g, e->stable_id);
    if (d->repeat == REPEAT_ONCE && last >= 0) return;
    if (d->repeat == REPEAT_ONCE_PER_DAY && last == today) return;

    ui_dialog_play(g, d, e);
    game_dirty_entity_override_dialog_day(g, e->stable_id, today);

	    if (d->post_state[0]) {
	        /* Parse "give_item:winter_coat" or "set_active:false" etc.
	           For give_item of KIND_OUTFIT, respect g->auto_wear_first and
	           call game_dirty_outfit_set so slice outfits work immediately. */
	        npc_apply_post_state(g, e, d->post_state);
	    }
    game_request_checkpoint(g, CKPT_DIALOG_DONE);
}
```

- [ ] **Step 2: tests/test_dialog_repeat.c**

```c
void test_repeat_once(void) {
    /* Setup: NPC with dialog repeat=once. Run interact twice; second call
       returns early without playing. */
}
void test_repeat_once_per_day(void) {
    /* Setup: dialog repeat=once_per_day, day=0. Play once. Advance day to 1.
       Play succeeds. Replay same day -> no play. */
}
void test_post_state_give_item(void) {
    /* Setup: dialog post_state=give_item:rain_boots. After play, save shows
       rain_boots in profile_items. */
}
```

- [ ] **Step 3: Register**

```c
void entity_npc_register(void) {
    entity_registry_register("npc", &NPC_CB);
}
```

- [ ] **Step 4: Run + commit**

```bash
git add src/entity_npc.c tests/test_dialog_repeat.c tests/test_main.c CMakeLists.txt
git commit -m "feat(entity): NPC + dialog driver with repeat rules"
```

---

## Phase I — Time of day

### Task 31: Clock + day_index + pause rules

**Goal:** `Clock` advances at 1 game-hour per 3 real-minutes; pauses during dialog, pause menu, inventory. Day rollover bumps `day_index`.

**Files:**
- Create: `src/time_of_day.c`, `src/time_of_day.h`
- Modify: `src/game.h` — add `Clock clock;`.
- Modify: `src/game.c`

- [ ] **Step 1: Write time_of_day.h**

```c
#pragma once
#include <stdbool.h>

typedef struct Clock {
    int   minute;        // 0..1439
    int   day_index;
    float accum;         // real-seconds accumulator
} Clock;

void clock_init  (Clock *c, int start_minute);
void clock_tick  (Clock *c, float dt, bool paused);    // returns nothing; updates fields
bool clock_rolled_day(const Clock *c);                  // true on the tick that rolled
int  clock_hour  (const Clock *c);                      // 0..23
void clock_advance_to(Clock *c, int target_minute);     // sundial / sleep affordance
```

- [ ] **Step 2: Implement**

`clock_tick`: if paused, return. Else `c->accum += dt`. While `c->accum >= GAME_MIN_PER_REAL_SEC` (= 60 / (3*60) = 1/3 — i.e. one game-minute per real-second/3), advance by one minute. On minute == 0 after rollover, bump `day_index` and set `rolled_day` flag (cleared next tick).

`clock_advance_to(target)`: bumps minute (and `day_index` if it wraps).

- [ ] **Step 3: Test**

```c
void test_clock_rolls_day(void) {
    Clock c; clock_init(&c, 23 * 60 + 59);     // 23:59
    /* simulate enough dt for one game-minute */
    for (int i = 0; i < 100; i++) clock_tick(&c, 0.1f, false);
    ASSERT_EQ_INT(c.day_index, 1);
    ASSERT_EQ_INT(c.minute, 0);
}

void test_clock_pause_freezes(void) {
    Clock c; clock_init(&c, 10 * 60);
    for (int i = 0; i < 100; i++) clock_tick(&c, 0.1f, true);
    ASSERT_EQ_INT(c.minute, 10 * 60);
}

void test_advance_to_wraps(void) {
    Clock c; clock_init(&c, 23 * 60);
    clock_advance_to(&c, 1 * 60);    // wrap to next day
    ASSERT_EQ_INT(c.day_index, 1);
    ASSERT_EQ_INT(c.minute, 60);
}
```

- [ ] **Step 4: Run + commit**

```bash
git add src/time_of_day.* tests/test_time_of_day.c tests/test_main.c CMakeLists.txt
git commit -m "feat(time): clock with day rollover, pause, advance-to"
```

---

### Task 32: Sun vector, gates, on_time_changed dispatch

**Goal:** A sun vector for the shadow module; gate visibility for entities whose `gate_hour_*` excludes the current hour; per-hour `on_time_changed` callbacks fire on the tick where the hour rolls.

**Files:**
- Modify: `src/time_of_day.c`, `src/time_of_day.h`
- Modify: `src/game.c`

- [ ] **Step 1: Sun vector API**

```c
typedef struct SunVector { float dx, dy; float length; float elevation; } SunVector;

SunVector sun_at(int minute);          // 0..1439
bool entity_gate_active(int8_t a, int8_t b, int hour);   // -1 == always
```

`sun_at`: angle goes from east at dawn through south at noon to west at dusk; `length` peaks at dawn/dusk and is shortest at noon; `elevation` is 0 at night, 1 at noon. Simple sin/cos lookup is fine. Return zeros at night for `elevation` to fade shadows out.

`entity_gate_active(a, b, hour)`: if `a == -1` always true; else hours where `a <= hour <= b` (wraps if `a > b`).

- [ ] **Step 2: Dispatch on_time_changed**

In `game_tick`, after `clock_tick`, if the hour changed since last tick, iterate alive entities and
call `e->cb->on_time_changed(g, e, hour)` only when both `e->cb` and
`e->cb->on_time_changed` are non-NULL.

- [ ] **Step 3: Tests**

```c
void test_gate_wraps_midnight(void) {
    ASSERT(entity_gate_active(22, 4, 23));
    ASSERT(entity_gate_active(22, 4, 3));
    ASSERT(!entity_gate_active(22, 4, 12));
}

void test_sun_noon_short_shadow(void) {
    SunVector noon = sun_at(12 * 60);
    SunVector dawn = sun_at(6 * 60);
    ASSERT(noon.length < dawn.length);
    ASSERT(noon.elevation > dawn.elevation);
}

void test_on_time_changed_fires_once_per_hour_boundary(void) {
    /* Build a Game with a probe entity whose on_time_changed counts calls.
       Tick across the 12:59 -> 13:00 boundary. Verify count == 1. */
}
```

- [ ] **Step 4: Run + commit**

```bash
git add src/time_of_day.* src/game.c tests/test_time_of_day.c CMakeLists.txt
git commit -m "feat(time): sun vector + gates + on_time_changed dispatch"
```

---

## Phase J — Audio

### Task 33: Mixer table + crossfade

**Goal:** Layered mixer (base, texture, time-of-day, one-shots) with per-frame volume lerp toward a target.

**Files:**
- Create: `src/audio.c`, `src/audio.h`

- [ ] **Step 1: Write audio.h**

```c
#pragma once
#include <stdbool.h>

typedef enum { AUDIO_BASE, AUDIO_TEXTURE, AUDIO_TOD, AUDIO_LAYER_COUNT } AudioLayer;

void audio_init(void);
void audio_shutdown(void);

void audio_set_layer(AudioLayer layer, const char *clip_key, float target_volume);
void audio_clear_layer(AudioLayer layer);
void audio_oneshot(const char *clip_key, float volume);
void audio_update(float dt);

void audio_master_volume (float v);   // 0..1
void audio_ambience_volume(float v);
void audio_sfx_volume    (float v);
```

- [ ] **Step 2: Implement mixer table**

Per layer holds `Music handle, char clip[64], float volume, float target_volume`. Each `audio_update` lerps `volume` toward `target_volume` at a configurable rate (default ~1.5 s for the full range). raylib's `UpdateMusicStream` is called per layer. When `target_volume` is 0 and `volume <= 0.01`, the layer's clip is unloaded.

Shared-position rule: when swapping the clip for a layer, set `SeekMusicStream` to keep the global loop phase consistent across layers. (Track `g_loop_phase_seconds` from `audio_init` time; align all layers to it.)

- [ ] **Step 3: No automated test (raylib + audio device required)**

Manual smoke: in title screen, call `audio_set_layer(AUDIO_BASE, "birds_loop.ogg", 1.0f);` and verify a sound plays.

- [ ] **Step 4: Wire to CMake amazeing target**

- [ ] **Step 5: Commit**

```bash
git add src/audio.* CMakeLists.txt
git commit -m "feat(audio): layered mixer with crossfade and master/sub volumes"
```

---

### Task 34: Audio zones + time-of-day layer swap

**Goal:** Detect when the player enters/exits a `audio_zones:` rect; crossfade the named clip into the `AUDIO_TEXTURE` slot. Day → dusk → night transitions swap the `AUDIO_TOD` slot.

**Files:**
- Modify: `src/audio.c`, `src/audio.h`
- Modify: `src/game.c` — call `audio_update_world(g)` each tick.

- [ ] **Step 1: Per-tick zone resolution**

```c
void audio_update_world(struct Game *g) {
    // For each zone in g->world.maze.zones, check if player (tile coords) is inside.
    // Topmost listed zone wins. If found, audio_set_layer(AUDIO_TEXTURE, zone.clip, 1.0).
    // If none, audio_set_layer(AUDIO_TEXTURE, NULL, 0.0).
}

void audio_set_time_of_day(int hour) {
    const char *clip;
    if (hour >= 6 && hour < 18) clip = "tod_day.ogg";
    else if (hour < 20)         clip = "tod_dusk.ogg";
    else                        clip = "tod_night.ogg";
    audio_set_layer(AUDIO_TOD, clip, 0.7f);
}
```

`audio_set_time_of_day` is called from `game_tick` on hour changes (Task 32 already detects these).

- [ ] **Step 2: Manual smoke**

Walk into the slice's creek zone; confirm `water_creek.ogg` crossfades in over ~1.5 s.

- [ ] **Step 3: Commit**

```bash
git add src/audio.* src/game.c
git commit -m "feat(audio): zone-based texture layer + time-of-day swap"
```

---

## Phase K — Camera + Render

### Task 35: Camera follow with deadzone + smoothing

**Goal:** Camera tracks the player with a soft deadzone; outside the deadzone it eases toward the player.

**Files:**
- Create: `src/camera.c`, `src/camera.h`
- Modify: `src/game.h` — add `GameCamera camera;`.

- [ ] **Step 1: Write camera.h**

```c
#pragma once
#include "util_math.h"

typedef struct GameCamera {
    Vec2  pos;                /* world-space camera center, in tile units */
    Vec2  deadzone_half;      /* e.g. {2, 1.5} tiles */
    float smoothing;          /* per-second easing factor; 6.0 = snappy */
} GameCamera;

void camera_init   (GameCamera *c, Vec2 start);
void camera_follow (GameCamera *c, Vec2 target, float dt);
```

- [ ] **Step 2: Implement**

`camera_follow`: compute delta = target - pos. Clamp to zero if `|delta.x| <= deadzone_half.x` and `|delta.y| <= deadzone_half.y`. Otherwise ease: `pos = lerp(pos, target_outside_deadzone, 1 - exp(-smoothing * dt))`.

- [ ] **Step 3: Test**

```c
void test_camera_inside_deadzone_does_not_move(void) {
    GameCamera c; camera_init(&c, (Vec2){10, 10});
    c.deadzone_half = (Vec2){2, 2};
    camera_follow(&c, (Vec2){11, 11}, 1.0f);
    ASSERT_NEAR(c.pos.x, 10.0f, 0.001f);
}
void test_camera_outside_eases(void) {
    GameCamera c; camera_init(&c, (Vec2){0, 0});
    c.deadzone_half = (Vec2){1, 1};
    c.smoothing = 6.0f;
    camera_follow(&c, (Vec2){10, 0}, 1.0f);
    ASSERT(c.pos.x > 5.0f && c.pos.x < 9.0f);
}
```

Add suite to test_main.c.

- [ ] **Step 4: Run + commit**

```bash
git add src/camera.* tests/test_camera.c CMakeLists.txt
git commit -m "feat(camera): follow with deadzone and exp smoothing"
```

---

### Task 36: World render texture + iso projection

**Goal:** Render the world to a fixed-size 480×270 `RenderTexture2D`, then upscale nearest-neighbor to the window. Iso projection helpers map tile coords ↔ screen coords.

**Files:**
- Create: `src/render.c`, `src/render.h`
- Create: `src/render_world.c`

- [ ] **Step 1: Write render.h**

```c
#pragma once
struct Game;

#define WORLD_RT_W 480
#define WORLD_RT_H 270
#define TILE_W 64
#define TILE_H 32

void render_init    (void);
void render_shutdown(void);
void render_frame   (struct Game *g, float alpha);
```

Iso helpers in render_world.c:

```c
static inline void iso_world_to_screen(float tx, float ty, float *sx, float *sy) {
    *sx = (tx - ty) * (float)(TILE_W / 2);
    *sy = (tx + ty) * (float)(TILE_H / 2);
}
static inline void iso_screen_to_world(float sx, float sy, float *tx, float *ty) {
    *tx = (sx / (TILE_W / 2) + sy / (TILE_H / 2)) * 0.5f;
    *ty = (sy / (TILE_H / 2) - sx / (TILE_W / 2)) * 0.5f;
}
```

(Note: `TILE_W/TILE_H` are 2:1 — the 64×32 dimetric ratio in spec §3.7.)

- [ ] **Step 2: Allocate the render texture**

`render_init`: `g_world_rt = LoadRenderTexture(WORLD_RT_W, WORLD_RT_H); SetTextureFilter(g_world_rt.texture, TEXTURE_FILTER_POINT);`

- [ ] **Step 3: Implement render_frame skeleton**

```c
void render_frame(Game *g, float alpha) {
    BeginTextureMode(g_world_rt);
    ClearBackground(BLACK);
    /* sky/gradient -> parallax -> ground -> shadows -> entities -> dusk overlay
       (subsequent tasks fill these in) */
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);

    /* integer-upscale world to window */
    int win_w = GetScreenWidth(), win_h = GetScreenHeight();
    int scale_i = (int)fmaxf(1.0f, floorf(fminf(
        (float)win_w / WORLD_RT_W, (float)win_h / WORLD_RT_H)));
    float w = WORLD_RT_W * (float)scale_i, h = WORLD_RT_H * (float)scale_i;
    Rectangle src = {0, 0, WORLD_RT_W, -WORLD_RT_H};   // flip Y
    Rectangle dst = {(win_w - w) / 2, (win_h - h) / 2, w, h};
    DrawTexturePro(g_world_rt.texture, src, dst, (Vector2){0, 0}, 0.0f, WHITE);

    /* UI layer at native resolution drawn after */
    ui_render_native(g);

    EndDrawing();
}
```

- [ ] **Step 4: Manual smoke**

Run. Expected: 480×270 letterboxed (centered) world layer (currently black) with no UI yet.

- [ ] **Step 5: Commit**

```bash
git add src/render.* src/render_world.c CMakeLists.txt
git commit -m "feat(render): world RT + nearest upscale + iso projection"
```

---

### Task 37: Ground tile drawing

**Goal:** Draw the maze's tile grid as iso-projected sprites. Tile-to-sprite mapping from the theme's `tiles.txt` (extended with `sprite:` lines).

**Files:**
- Modify: `src/render_world.c`
- Modify: per-theme `tiles.txt` to include sprite paths

- [ ] **Step 1: Extend theme tile table**

```
# tiles.txt
. 1 sprite:dirt.png
G 1 sprite:grass.png
T 0 sprite:tree_oak.png      # placeholder; trees are entities not tiles in production
W 0 sprite:water.png
```

- [ ] **Step 2: Draw the visible tile range**

In `render_world.c`, compute the camera-visible tile range, iterate, project each to screen using `iso_world_to_screen`, draw via `asset_acquire_texture`.

```c
static void draw_ground(World *w, GameCamera *cam) {
    int x0, y0, x1, y1; world_visible_tile_range(w, cam, &x0, &y0, &x1, &y1);
    for (int ty = y0; ty <= y1; ty++) {
        for (int tx = x0; tx <= x1; tx++) {
            char c = w->maze.tiles[ty * w->maze.width + tx];
            const char *sprite = w->tile_sprite[(int)c];
            if (!sprite) continue;
            Texture2D *t = asset_acquire_texture(sprite);
            float sx, sy;
            iso_world_to_screen(tx - cam->pos.x, ty - cam->pos.y, &sx, &sy);
            DrawTexture(*t, (int)(sx + WORLD_RT_W / 2),
                        (int)(sy + WORLD_RT_H / 2), WHITE);
        }
    }
}
```

- [ ] **Step 3: Manual smoke with placeholder tile sprites**

Load a tiny maze. Confirm tiles draw in iso orientation.

- [ ] **Step 4: Commit**

```bash
git add src/render_world.c assets/themes/* CMakeLists.txt
git commit -m "feat(render): iso ground tile drawing"
```

---

### Task 38: Y-sort entity pass

**Goal:** Sort entities by foot point (world Y + offset from sprite manifest) and draw bottom-up. Player interleaves into the entity list.

**Files:**
- Modify: `src/render_world.c`

- [ ] **Step 1: Build a draw list per frame**

```c
typedef struct DrawItem { float foot_y; Vec2 pos; Texture2D *tex; Vec2 origin; int z; } DrawItem;
static int draw_item_cmp(const void *a, const void *b) {
    const DrawItem *A = a, *B = b;
    return (A->foot_y < B->foot_y) ? -1 : (A->foot_y > B->foot_y) ? 1 : 0;
}
```

For each alive entity, look up its `SpriteDef`, compute `foot_y = pos.y + sprite->foot_y_offset`, push a `DrawItem`. Push the player similarly.

- [ ] **Step 2: qsort and draw**

`qsort(items, n, sizeof DrawItem, draw_item_cmp);` then iterate, project, draw.

- [ ] **Step 3: Manual smoke**

Confirm sprites overlap correctly: an entity south of the player draws over the player; north entities draw behind.

- [ ] **Step 4: Commit**

```bash
git add src/render_world.c
git commit -m "feat(render): y-sorted entity draw pass"
```

---

### Task 39: Shadow, parallax, dusk overlay

**Goal:** Per-entity shadows derived from sun vector; parallax wall layers offset by camera × per-layer factor; multiplicative dusk tint over the playfield.

**Files:**
- Create: `src/shadow.c`, `src/shadow.h`
- Create: `src/parallax.c`, `src/parallax.h`
- Modify: `src/render_world.c`

- [ ] **Step 1: shadow.h + impl**

```c
typedef struct ShadowOpts { float offx, offy; float scale_y; float alpha; } ShadowOpts;
ShadowOpts shadow_for(SunVector sun, float entity_height);
```

`shadow_for`: `offx = sun.dx * sun.length * h; offy = sun.dy * sun.length * h; scale_y = sun.length; alpha = sun.elevation;`. Draw a tinted ellipse or pre-drawn blob sprite at the entity foot, offset by `(offx, offy)`, vertically scaled by `scale_y`, alpha multiplied by `alpha`.

- [ ] **Step 2: parallax.h + impl**

```c
typedef struct ParallaxLayer { Texture2D *tex; float factor; } ParallaxLayer;
void parallax_draw(ParallaxLayer *layers, int count, GameCamera *cam);
```

Two layers in the slice: far (0.3), near (0.7). Draw each as a horizontally-repeating strip whose X offset = `-cam.pos.x * factor`. Player cannot walk behind them; they're decoration past the playable area.

- [ ] **Step 3: Dusk overlay**

In `render_world.c`, after drawing entities, draw a full-screen rect with color =
`tint_for_minute(g->clock.minute)` and `BLEND_MULTIPLIED`. Use these anchor colors and linearly
interpolate between adjacent anchors:
- 06:00 dawn `(255, 220, 200, 255)`
- 12:00 noon `(255, 255, 255, 255)` (no-op)
- 18:00 dusk `(255, 180, 140, 255)`
- 22:00 night `(80, 90, 140, 255)`, held until dawn

- [ ] **Step 4: shadow unit test**

```c
void test_shadow_at_noon_is_short(void) {
    SunVector noon = sun_at(12 * 60);
    SunVector dawn = sun_at(6 * 60);
    ShadowOpts a = shadow_for(noon, 1.0f);
    ShadowOpts b = shadow_for(dawn, 1.0f);
    ASSERT(fabsf(a.offx) < fabsf(b.offx));
    ASSERT(a.scale_y < b.scale_y);
}
```

- [ ] **Step 5: Manual smoke + commit**

Confirm shadows move with sun, parallax slides when camera moves, dusk tints over time.

```bash
git add src/shadow.* src/parallax.* src/render_world.c tests/test_shadow.c CMakeLists.txt
git commit -m "feat(render): shadows, parallax walls, dusk overlay"
```

---

## Phase L — UI

### Task 40: UI dispatch + title screen

**Goal:** UI module renders at native resolution. Title screen shows the game name and two options (Start / Quit), localized.

**Files:**
- Create: `src/ui.c`, `src/ui.h`
- Create: `src/ui_title.c`
- Create: `src/render_ui.c`

- [ ] **Step 1: Write ui.h**

```c
#pragma once
struct Game;

void ui_init    (void);
void ui_shutdown(void);

void ui_tick    (struct Game *g, const struct InputFrame *in);
void ui_render_native(struct Game *g);

float ui_scale (void);            // computed from window height + Settings choice
```

- [ ] **Step 2: ui.c dispatches on game state**

`ui_tick(g, in)`: switch on `game_state(g)`; call `ui_title_tick`, `ui_profile_tick`, etc. Same for render.

- [ ] **Step 3: Implement ui_title.c**

Two menu items localized via `t("ui.title.start")` and `t("ui.title.quit")`. On `ACT_CONFIRM`, transition to `GS_PROFILE`. On `ACT_CANCEL`, set a quit flag the main loop reads.

Render: centered text via the bundled font; current selection highlighted. Use `MeasureTextEx` for layout.

- [ ] **Step 4: Bundled font load**

In `ui_init`, load `assets/ui/font/Inter-Regular.ttf` via `asset_acquire_font` with a Latin Extended codepoint set.

- [ ] **Step 5: Manual smoke + commit**

```bash
git add src/ui.* src/ui_title.c src/render_ui.c CMakeLists.txt
git commit -m "feat(ui): UI dispatch + native-resolution title screen"
```

---

### Task 41: Profile select + create

**Goal:** List existing profiles (up to 4), allow creating a new one with a name, deletion guarded behind a confirm.

**Files:**
- Create: `src/ui_profile.c`

- [ ] **Step 1: Layout**

List 4 slots. Empty slot shows "+ New Profile". Selected slot enters either "Start" (existing) or "Create" mode (typed name).

For text input, use raylib's `GetCharPressed()` queue; cap at `PROFILE_NAME_MAX-1` chars; reject empty names.

- [ ] **Step 2: Wire to profile.c**

`profile_list` populates the slot list. Selecting a slot + `ACT_CONFIRM` sets `g->active_profile_id = ps[i].id;` and transitions to `GS_MAZE_SELECT`.

- [ ] **Step 3: Delete flow**

Highlight + `ACT_CANCEL` shows a "Delete profile? Confirm/Cancel" prompt overlay. On confirm: `profile_delete(id)`.

- [ ] **Step 4: Manual smoke + commit**

```bash
git add src/ui_profile.c CMakeLists.txt
git commit -m "feat(ui): profile select + create + delete"
```

---

### Task 42: Maze-select hub

**Goal:** Visual hub map (default per §11) with parallax background; each maze shown as a labelled icon with lock/unlock state. Selecting a maze loads it.

**Files:**
- Create: `src/ui_maze_select.c`

- [ ] **Step 1: Hub data**

A small static table (`MazeHubEntry` array) lives in `ui_maze_select.c`:

```c
typedef struct { const char *maze_id; const char *name_key; float hub_x, hub_y;
                 const char *requires_item;   /* NULL if none */ } MazeHubEntry;

static const MazeHubEntry HUB[] = {
    {"nature",       "maze.nature.name",        180, 140, NULL},
    /* Other mazes placeholder for future plans, marked locked. */
};
```

- [ ] **Step 2: Lock check**

A maze is "unlocked" if `requires_item == NULL` or `save_load_profile_items` contains it. Locked icons render dim with a lock badge.

- [ ] **Step 3: Selection + load**

On `ACT_CONFIRM` on the Nature icon, `world_load_maze(&g->world, "data/mazes/nature.maze", "assets/themes/nature")`; transition to `GS_IN_MAZE`. Resume from snapshot if present via `save_load_maze_snapshot`; otherwise start at `maze.start_x/y` with `clock_min = maze.time_start_min`.

- [ ] **Step 4: Manual smoke + commit**

```bash
git add src/ui_maze_select.c CMakeLists.txt
git commit -m "feat(ui): maze-select hub with parallax background"
```

---

### Task 43: HUD + clock display

**Goal:** In-maze HUD shows the clock (HH:MM, day index when > 0), worn outfit icon, and a small "?" hint key reminder.

**Files:**
- Create: `src/ui_hud.c`

- [ ] **Step 1: Layout**

Top-left: clock + day. Top-right: outfit icon. Below outfit: small "Press [I] for Inventory" reminder (uses `input_get_binding` to display the current key glyph).

- [ ] **Step 2: Color-independent cues**

HUD never relies on color alone. Use icons + labels for outfit indicator, and shape badges where status is conveyed (§2.15).

- [ ] **Step 3: Manual smoke + commit**

```bash
git add src/ui_hud.c CMakeLists.txt
git commit -m "feat(ui): HUD with clock, outfit, control hints"
```

---

### Task 44: Dialog renderer with text speed + fade affordance

**Goal:** Dialog box reveals localized lines at a configurable text-speed (slow / normal / instant). Pauses the clock. Also provides `ui_fade_then_advance_clock` for the sundial.

**Files:**
- Create: `src/ui_dialog.c`

- [ ] **Step 1: Dialog state**

```c
typedef struct DialogState {
    const DialogDef *def;
    int   line_index;
    int   chars_revealed;
    float chars_per_second;   /* from settings: slow=20, normal=40, instant=10000 */
    bool  active;
} DialogState;
```

`ui_dialog_play(g, def, entity)` populates the state and pauses the clock (sets `g->paused_for_dialog = true`).

- [ ] **Step 2: Tick + render**

Per tick: `chars_revealed += chars_per_second * dt`. `ACT_CONFIRM`:
- If line not fully revealed, snap to fully revealed.
- Else advance to next line. If past last line, run `post_state` (if any) and clear `active`; resume clock.

Render: bottom-of-screen panel with speaker (if any) + the current line truncated to `chars_revealed`. Use the bundled font at UI scale.

- [ ] **Step 3: Fade affordance**

```c
void ui_fade_then_advance_clock(Game *g, int target_minute) {
    /* Set a fade overlay alpha animation; at peak, call clock_advance_to. */
}
```

Render `BLACK` rect with animated alpha over the whole screen (UI layer). Show "X hours pass…" with `t("ui.time.advance")` interpolated.

- [ ] **Step 4: Manual smoke + commit**

```bash
git add src/ui_dialog.c CMakeLists.txt
git commit -m "feat(ui): dialog box with text speed + sundial fade"
```

---

### Task 45: Inventory grid

**Goal:** Inventory screen lists all items the active profile + active maze hold, grouped by `kind` (outfit / tool / consumable / access). Used for both browsing and the multi-item picker case (§2.5).

**Files:**
- Create: `src/ui_inventory.c`

- [ ] **Step 1: Aggregate items**

When opened, query `save_load_profile_items(pid)` + `save_load_maze_inventory(pid, maze_id)`. For each id, look up `ItemDef` for name, icon, kind, desc. Group by kind into four columns.

- [ ] **Step 2: Outfit wear toggle**

For `KIND_OUTFIT`, `ACT_CONFIRM` toggles `outfit_worn` to that item; calls `save_set_outfit_worn` and `save_request_checkpoint(CKPT_PAUSE)`. Player sprite layer reflects the change next render.

- [ ] **Step 3: Picker mode**

`ui_inventory_open_as_picker(g, applicable_item_ids[], n)` opens the inventory restricted to those items. `ACT_CONFIRM` calls back to `interaction_dispatch(g, target, chosen_item)`.
Task 45 also extends `Game` with `InteractionTarget pending_picker_target;` and
`bool picker_active;`. `interaction_dispatch` copies the selected target into `Game` before opening
the picker; the inventory UI reads it back on confirm. Do not store the target in module-static
state because tests and future split-screen/tooling need explicit ownership.

- [ ] **Step 4: Manual smoke + commit**

```bash
git add src/ui_inventory.c CMakeLists.txt
git commit -m "feat(ui): inventory grid with kind grouping + picker mode"
```

---

### Task 46: Settings + remap

**Goal:** Settings screen: text speed (slow / normal / instant), volume sliders (master / ambience / sfx), language picker, key bindings list with swap-or-cancel prompts. "Reset bindings to defaults" button. Non-blocking notice if bindings were just recovered.

**Files:**
- Create: `src/ui_settings.c`

- [ ] **Step 1: Layout**

Tabs: Controls / Audio / Display / Language. Each tab is a list of settings.

- [ ] **Step 2: Bindings UI**

For each `Action`, show its current key glyph. Selecting an action + pressing a new key triggers `input_propose_binding`:
- `REMAP_RESERVED` → show "This key is reserved" toast.
- `REMAP_SWAP_REQUIRED` → modal: "[KEY] is currently bound to [Action]. Swap or Cancel?" Swap calls `input_apply_swap`.
- `REMAP_OK` → apply directly.

Each apply writes to `settings` via `input_save_bindings`.

On open, check `input_load_recovery_notice()` (set by `input_load_bindings` when recovery happened) and show a non-blocking banner: `t("settings.bindings.recovered")`.

Leaving Settings: if `input_missing_required()` is true, show a blocking modal: "These actions still need bindings: [list]". Cannot exit until fixed or "Reset to defaults" used.

- [ ] **Step 3: Volume + text speed + language**

Volume sliders bind to `audio_master_volume`/`audio_ambience_volume`/`audio_sfx_volume`. Text speed updates `g->dialog_chars_per_second`. Language picker calls `loc_load_language(code)`. Each setting persists via a small `settings_set(profile_id, key, value)` helper in save.c.

- [ ] **Step 4: Manual smoke + commit**

```bash
git add src/ui_settings.c src/save.c src/save.h CMakeLists.txt
git commit -m "feat(ui): settings + remap with conflict/recovery handling"
```

---

## Phase M — Slice content

### Task 47: Nature theme manifest + placeholder art

**Goal:** Per-sprite manifest and placeholder PNGs for everything the slice draws. Placeholders are flat-color rectangles with a hand-readable label baked in.

**Files:**
- Create: `assets/themes/nature/manifest.txt`
- Create: `assets/themes/nature/tiles.txt`
- Create: placeholder PNGs (use a small helper script, see Step 1).
- Create: `assets/RENDER_CONVENTIONS.md`

- [ ] **Step 1: Placeholder generator script**

```bash
# tools/gen_placeholders.sh — uses ImageMagick to bake colored rectangles
# with text labels at the right pixel sizes (matches manifest dimensions).
convert -size 64x32 xc:'#8aa66a' -gravity center -pointsize 8 \
    -annotate +0+0 'grass' assets/themes/nature/grass.png
convert -size 64x96 xc:'#5d4632' -gravity center -pointsize 10 \
    -annotate +0+0 'tree_oak' assets/themes/nature/tree_oak.png
# ... and so on for: water, dirt, withered_plant, flower, seed_pickup,
# sign, sundial, locked_gate, npc_gardener, rain_boots_pickup, fountain.
```

Commit the generator + the generated PNGs so the build doesn't depend on ImageMagick at build time.

- [ ] **Step 2: Write manifest.txt**

```
sprite:        grass.png
origin:        32, 16
foot:          0, 0
shadow_h:      0.0
layer:         decor

sprite:        dirt.png
origin:        32, 16
foot:          0, 0
shadow_h:      0.0
layer:         decor

sprite:        tree_oak.png
origin:        32, 96
foot:          0, 0
collide:       rect -6 -4 12 8
proximity:     rect -10 -8 20 16
shadow_h:      1.4
layer:         object

sprite:        seed_pickup.png
origin:        16, 24
foot:          0, 0
proximity:     rect -8 -8 16 16
shadow_h:      0.4
layer:         object
interact:      0, -16

sprite:        withered_plant.png
origin:        16, 24
foot:          0, 0
collide:       rect -4 -2 8 4
proximity:     rect -10 -10 20 16
shadow_h:      0.6
layer:         object
interact:      0, -16

sprite:        flower.png
origin:        16, 24
foot:          0, 0
proximity:     rect -10 -10 20 16
shadow_h:      0.6
layer:         object
interact:      0, -16

sprite:        rain_boots_pickup.png
origin:        16, 16
foot:          0, 0
proximity:     rect -10 -10 20 16
shadow_h:      0.3
layer:         object

sprite:        locked_gate.png
origin:        32, 64
foot:          0, 0
collide:       rect -16 -4 32 8
proximity:     rect -20 -10 40 20
shadow_h:      1.0
layer:         object

sprite:        sign.png
origin:        16, 32
foot:          0, 0
collide:       rect -4 -2 8 4
proximity:     rect -10 -10 20 20
shadow_h:      0.6
layer:         object
interact:      0, -16

sprite:        sundial.png
origin:        24, 32
foot:          0, 0
collide:       rect -8 -2 16 6
proximity:     rect -16 -10 32 20
shadow_h:      0.8
layer:         object
interact:      0, -12

sprite:        npc_gardener.png
origin:        16, 48
foot:          0, 0
collide:       rect -4 -4 8 8
proximity:     rect -14 -14 28 22
shadow_h:      1.2
layer:         character
interact:      0, -36

sprite:        water.png
origin:        32, 16
foot:          0, 0
shadow_h:      0.0
layer:         decor

sprite:        mud.png
origin:        32, 16
foot:          0, 0
shadow_h:      0.0
layer:         decor
```

- [ ] **Step 3: tiles.txt for nature**

```
. 1 sprite:dirt.png
g 1 sprite:grass.png
m 1 sprite:mud.png
w 0 sprite:water.png
```

- [ ] **Step 4: RENDER_CONVENTIONS.md**

```markdown
# Render Conventions
- Iso angle: 2:1 dimetric (tiles 64×32 px).
- Sun direction at noon: straight down, length minimum, alpha 1.
- Sun direction at dawn (6:00): east, length max, alpha 0.6.
- Outfit-layer alignment grid: 16×16 px cell at sprite origin (32, 48 for the standard character sheet).
```

- [ ] **Step 5: Commit**

```bash
git add assets/themes/nature/ assets/RENDER_CONVENTIONS.md tools/gen_placeholders.sh
git commit -m "asset(nature): placeholder art + sprite manifest + tile table"
```

---

### Task 48: Slice items, dialogs, strings

**Goal:** All slice items declared in `data/items.txt`, the gardener dialog declared in `data/dialogs/nature.txt`, all strings in `data/strings/en.lang`.

**Files:**
- Create: `data/items.txt`
- Create: `data/dialogs/nature.txt`
- Create: `data/strings/en.lang`

- [ ] **Step 1: data/items.txt**

```
item:        rain_boots
name:        item.rain_boots.name
desc:        item.rain_boots.desc
scope:       profile
kind:        outfit
icon:        ui/icons/rain_boots.png

item:        seed
name:        item.seed.name
desc:        item.seed.desc
scope:       maze
kind:        consumable
icon:        ui/icons/seed.png
use_tag:     water_plant
```

- [ ] **Step 2: data/dialogs/nature.txt**

```
dialog:      npc.nature.gardener.greet
speaker:     npc.nature.gardener.name
lines:
  npc.nature.gardener.greet.l1
  npc.nature.gardener.greet.l2
repeat:      once
post_state:  give_item:rain_boots
```

- [ ] **Step 3: data/strings/en.lang**

```
# title
ui.title.start = Start
ui.title.quit  = Quit

# settings
ui.settings.controls          = Controls
ui.settings.audio             = Audio
ui.settings.display           = Display
ui.settings.language          = Language
ui.settings.text_speed.slow   = Slow
ui.settings.text_speed.normal = Normal
ui.settings.text_speed.instant= Instant
ui.settings.bindings.recovered = Bindings were missing actions; defaults restored.
ui.settings.bindings.required  = These actions still need bindings:

# maze names
maze.nature.name = Nature

# items
item.rain_boots.name = Rain Boots
item.rain_boots.desc = Lets you cross muddy ground.
item.seed.name       = Seed
item.seed.desc       = A small seed. Plant it where something might grow.

# dialog
npc.nature.gardener.name      = Gardener
npc.nature.gardener.greet.l1  = Oh! You're not from around here.
npc.nature.gardener.greet.l2  = These boots should help you out. Take them.

# hints
hint.door.locked       = Locked. Something is needed to open it.
hint.door.wrong_item   = That doesn't fit this lock.
hint.welcome           = Welcome to the Nature maze.
hint.mud.needs_boots   = The mud is too deep without boots.
hint.withered_plant    = It looks thirsty.
hint.withered_plant.2  = Maybe a seed would help it grow?
hint.sundial           = Sundial. Wait until later?

# time
ui.time.advance = A few hours pass...
```

- [ ] **Step 4: Commit**

```bash
git add data/items.txt data/dialogs/nature.txt data/strings/en.lang
git commit -m "content(slice): nature items, gardener dialog, en strings"
```

---

### Task 49: Slice maze

**Goal:** Hand-craft the smallest possible Nature map that exercises every subsystem.

**Files:**
- Create: `data/mazes/nature.maze`

- [ ] **Step 1: data/mazes/nature.maze**

```
name:       maze.nature.name
theme:      nature
size:       24x14
start:      3,7
time_start: 08:00
ambient:    birds_loop.ogg, wind_soft.ogg

tiles:
........................
.gggggggggggggggggggggg.
.g....................g.
.g..g..mm.........g...g.
.g..g..mm....ww...gg..g.
.g..g........ww.......g.
.g........gg......g...g.
.g........gg......g...g.
.g...............mm...g.
.g.........gg.........g.
.g.........gg.........g.
.g....................g.
.gggggggggggggggggggggg.
........................

entities:
sign#nature.sign.intro        4 7   sprite:sign.png sign:hint.welcome
npc#nature.npc.gardener       6 6   sprite:npc_gardener.png dialog:npc.nature.gardener.greet
withered_plant#nature.wp.a    5 8   sprite:withered_plant.png use_tag:water_plant proximity:rect:-10,-10,20,16
pickup#nature.seed.a          12 8  sprite:seed_pickup.png item:seed
door#nature.gate.muddy        16 8  sprite:locked_gate.png needs:rain_boots target:area_b
hazard#nature.mud.patch       18 8  sprite:mud.png hazard:mud needs:rain_boots hint:hint.mud.needs_boots collide:rect:-32,-16,64,32
sundial#nature.sundial.main   18 6  sprite:sundial.png gate:12-13
pickup#nature.rb.a            14 11 sprite:rain_boots_pickup.png item:rain_boots

audio_zones:
creek 11 3 14 5 texture:water_creek.ogg
```

Behaviors used: `sign`, `npc`, `withered_plant`, `pickup`, `door`, `hazard`, `sundial`. The prefixes are
registered behavior names; visual sprite variants are carried by `sprite:` args. Register a small
`withered_plant` behavior in `entity_interact.c` if not already (it shares the `door` pattern:
accepts `use_tag:water_plant`, becomes a flower sprite, then non-blocking).

- [ ] **Step 2: Add the missing `withered_plant` behavior**

In `entity_interact.c`:

```c
static void withered_on_interact(Game *g, Entity *e, const char *item_id) {
    if (!item_id) {
        ui_dialog_show_key(g, "hint.withered_plant");
        return;
    }
    const ItemDef *d = items_find(item_id);
    if (!d || strcmp(d->use_tag, "water_plant") != 0) {
        ui_dialog_show_key(g, "hint.withered_plant.2");
        return;
    }
    /* For the slice, treat "seed" as a water_plant stand-in via its kind=consumable
       and decrement count. */
    game_dirty_inventory_add(g, item_id, -1);
    e->interact.active = true;
    e->sprite_id = sprites_find("flower.png")->id;   /* swap sprite */
    game_dirty_entity_override(g, e->stable_id, 0, 1, -1);
    game_request_checkpoint(g, CKPT_DIALOG_DONE);
}

static const EntityCallbacks WITHERED_CB = { .on_interact = withered_on_interact };

/* Add inside entity_interact_register: */
entity_registry_register("withered_plant", &WITHERED_CB);
```

(The slice's seed is the `use_tag:water_plant` placeholder declared in Task 48, so the validator
and the `withered_plant` behavior agree.)

- [ ] **Step 3: Commit**

```bash
git add data/mazes/nature.maze src/entity_interact.c data/items.txt
git commit -m "content(slice): nature maze + withered_plant behavior"
```

---

### Task 50: Audio placeholders + zone wiring

**Goal:** Real-but-placeholder audio files at the paths referenced by the slice. Quiet sine/noise loops are fine.

**Files:**
- Create: `assets/audio/birds_loop.ogg`
- Create: `assets/audio/wind_soft.ogg`
- Create: `assets/audio/water_creek.ogg`
- Create: `assets/audio/pickup.ogg`
- Create: `assets/audio/tod_day.ogg`
- Create: `assets/audio/tod_dusk.ogg`
- Create: `assets/audio/tod_night.ogg`

- [ ] **Step 1: Generate placeholder OGGs**

```bash
# tools/gen_placeholder_audio.sh — uses sox to synthesize short looping OGGs
sox -n -r 44100 -c 2 assets/audio/birds_loop.ogg synth 4 sine 880 vol 0.15
sox -n -r 44100 -c 2 assets/audio/wind_soft.ogg  synth 4 brownnoise vol 0.10
sox -n -r 44100 -c 2 assets/audio/water_creek.ogg synth 4 pinknoise vol 0.18
sox -n -r 44100 -c 1 assets/audio/pickup.ogg     synth 0.15 sine 1320 fade 0 0.15 0.1
sox -n -r 44100 -c 2 assets/audio/tod_day.ogg    synth 4 sine 220 vol 0.08
sox -n -r 44100 -c 2 assets/audio/tod_dusk.ogg   synth 4 sine 165 vol 0.08
sox -n -r 44100 -c 2 assets/audio/tod_night.ogg  synth 4 sine 110 vol 0.06
```

Commit both the generator script and the generated OGGs.

- [ ] **Step 2: Manual smoke**

Load the slice, enter the creek zone (tiles 11–14, 3–5). Confirm `water_creek.ogg` crossfades in. Walk out, confirm it fades out.

- [ ] **Step 3: Commit**

```bash
git add assets/audio/ tools/gen_placeholder_audio.sh
git commit -m "asset(audio): placeholder ambient and one-shot loops"
```

---

## Phase N — Validator + Acceptance

### Task 51: Maze validator CLI tool

**Goal:** `tools/validate.c` enforces §5.1: required headers, walkability, entity stable_id uniqueness + maze-prefix convention, behavior/item/dialog/loc-key resolution, audio zone bounds, reachability, structural soft-lock checks.

**Files:**
- Create: `tools/validate.c`
- Create: `tools/validator_registry.c`
- Create: `tests/test_validator.c`
- Modify: `CMakeLists.txt` — add `validate` executable.

- [ ] **Step 1: Add validate executable**

```cmake
add_executable(validate
    tools/validate.c
    tools/validator_registry.c
    src/maze_data.c src/world.c src/entity.c src/items.c src/dialogs.c src/localization.c
    src/sprite_manifest.c src/util_log.c src/util_math.c)
target_include_directories(validate PRIVATE src third_party/test_harness)
```

`tools/validator_registry.c` registers no-op callbacks for every behavior name the validator accepts
(`pickup`, `door`, `sign`, `sundial`, `npc`, `withered_plant`, `decor`, and hazard variants).
This lets `world_load_maze` resolve behavior names without linking gameplay modules that depend on UI.

- [ ] **Step 2: Validation checks (each returns a diagnostic)**

Implement each rule as `static bool check_X(...)`. Aggregate by
`int validate_run(const char *maze_path, Diag *out, int max)`.
For checks that need walkability or attached entity behavior, initialize the validator registry
and load through `world_load_maze` with the maze's theme directory; do not duplicate the tile-table
parser in `validate.c`.

Required rules (all errors unless noted):
- Header present and parses: `name`, `theme`, `size`, `start`.
- `tile_count == width * height`.
- `start_x, start_y` is walkable.
- Per entity: `stable_id` is unique within the maze (error).
- Per entity: `stable_id` starts with `maze_id + "."` (warning, error in CI when `--strict`).
- Per entity: `behavior` resolves in registry (error).
- Per entity: any `item:X` arg resolves in `items.txt` (error).
- Per entity: any `dialog:X` arg resolves in dialogs (error).
- Loc keys referenced (`maze.X.name`, `npc.X.name`, dialog line keys, `hint.X` keys) exist in `en.lang` (warning; CI elevates to error with `--strict`).
- Audio zones: rectangles inside the map.
- Reachability: fixed-point BFS from `start`. Start with walkable tiles + initially-open doors.
  After each BFS pass, mark any door/openable blocker whose `needs:` item is now reachable as open,
  then re-run BFS until no new blockers open. The final reachable set must visit every progression
  item (each `pickup` entity, the goal exit).
- Consumable soft-lock: for each `pickup` entity referencing a maze-scope consumable that targets a withered-plant/lock-style behavior elsewhere, verify either a second reachable instance exists, OR the target's behavior is in the `REJECTS_ON_MISUSE` whitelist (`door`, `withered_plant` are in the whitelist).

- [ ] **Step 3: Tests in tests/test_validator.c**

```c
void test_missing_header(void) {
    Diag diags[16];
    int n = validate_run("tests/fixtures/bad/missing_header.maze", diags, 16);
    ASSERT(n > 0);
    ASSERT_EQ_INT(diags[0].severity, DIAG_ERROR);
    ASSERT(strstr(diags[0].msg, "name") != NULL);
}

void test_duplicate_stable_id(void) { /* fixture: two entities share an id; expect error */ }
void test_unknown_item_reference(void) { /* fixture: entity with item:made_up; expect error */ }
void test_unreachable_pickup(void) { /* fixture: pickup surrounded by walls; expect error */ }
void test_maze_prefix_convention_warning(void) {
    Diag diags[16];
    int n = validate_run("tests/fixtures/bad/wrong_prefix.maze", diags, 16);
    /* default mode: warning */
    bool has_warn = false;
    for (int i = 0; i < n; i++) if (diags[i].severity == DIAG_WARN) has_warn = true;
    ASSERT(has_warn);
}
```

Create the fixture mazes referenced.

- [ ] **Step 4: CLI entry point**

```c
int main(int argc, char **argv) {
    bool strict = false; const char *path = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--strict")) strict = true;
        else path = argv[i];
    }
    if (!path) { fprintf(stderr, "usage: validate [--strict] <maze.maze>\n"); return 2; }
    Diag diags[256];
    int n = validate_run(path, diags, 256);
    int err = 0;
    for (int i = 0; i < n; i++) {
        bool fatal = diags[i].severity == DIAG_ERROR
                  || (strict && diags[i].severity == DIAG_WARN);
        printf("%s: %s\n", fatal ? "ERROR" : "WARN", diags[i].msg);
        if (fatal) err++;
    }
    return err ? 1 : 0;
}
```

- [ ] **Step 5: CMake target for running validator on all slice mazes**

```cmake
add_custom_target(validate_all
    COMMAND validate --strict ${CMAKE_SOURCE_DIR}/data/mazes/nature.maze
    DEPENDS validate)
```

- [ ] **Step 6: Run validate_all + commit**

```bash
cmake --build build --target validate_all
git add tools/validate.c tools/validator_registry.c tests/test_validator.c tests/fixtures/bad/ CMakeLists.txt
git commit -m "tool(validate): maze validator + CLI + slice CI target"
```

---

### Task 52: Headless smoke test

**Goal:** A `tools/smoke_test.c` binary that boots without a GL context, loads every `.maze` file in `data/mazes/`, attaches behaviors, steps the simulation `N` fixed ticks, and confirms no asserts/crashes/leaks. Catches data-driven regressions.

**Files:**
- Create: `tools/smoke_test.c`
- Create: `tools/headless_ui_stubs.c`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Identify which modules to compile-in**

Everything except `render*`, `audio*`, `ui*`, `parallax`, `shadow` (raylib-dependent paths).
World simulation + entities + collision + interaction + time + save are usable headless, but several
gameplay callbacks intentionally surface hints through UI functions. Link `tools/headless_ui_stubs.c`
with no-op implementations of `ui_dialog_show_key`, `ui_dialog_play`, and
`ui_fade_then_advance_clock` so the smoke test exercises gameplay without creating a GL context.

- [ ] **Step 2: Implement smoke_test.c**

```c
int main(void) {
    util_log_init(NULL);
    items_load("data/items.txt");
    dialogs_load("data/dialogs/nature.txt");
    loc_load_language_from("data/strings/", "en");

    entity_registry_init();
    entity_item_register();
    entity_hazard_register();
    entity_interact_register();
    entity_npc_register();

    /* Stub save: use a temp DB */
    save_open("/tmp/amz_smoke.db");
    int pid = profile_create("smoke");

    Game g = {0};
    g.active_profile_id = pid;
    if (!world_load_maze(&g.world, "data/mazes/nature.maze", "assets/themes/nature")) return 1;
    player_init(&g.player, (Vec2){g.world.maze.start_x, g.world.maze.start_y});

    InputFrame in = {0};
    for (int i = 0; i < 1000; i++) {
        game_tick(&g, &in, 1.0f / 60.0f);
    }
    /* Then run a short scripted interaction path: move near the intro sign and
       press interact; move near the gardener and press interact; move to the
       seed pickup and press interact. Keep the route data-driven and under
       ~300 fixed ticks so this remains a smoke test, not a brittle playthrough. */
    save_close();
    return 0;
}
```

- [ ] **Step 3: Add CMake target + CTest entry**

```cmake
add_executable(smoke_test
    tools/smoke_test.c
    tools/headless_ui_stubs.c
    /* all the .c files of the non-raylib modules */)
target_link_libraries(smoke_test PRIVATE sqlite3)
add_test(NAME smoke_test COMMAND smoke_test
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
```

- [ ] **Step 4: Run + commit**

```bash
ctest --test-dir build --output-on-failure -R smoke_test
git add tools/smoke_test.c tools/headless_ui_stubs.c CMakeLists.txt
git commit -m "test(smoke): headless maze boot + step test"
```

---

### Task 53: Boot integrity check + corruption recovery dialog

**Goal:** On `save_open`, if `integrity_check` fails, copy the DB (+ `-wal`/`-shm`) to a timestamped backup, surface a player-facing modal explaining what happened, and only proceed with a fresh DB after explicit confirmation.

**Files:**
- Modify: `src/save.c`
- Modify: `src/main.c`
- Create: `src/ui_recovery.c`

- [ ] **Step 1: Refine save_open flow**

```c
typedef enum { SAVE_OK, SAVE_CORRUPT, SAVE_FAIL } SaveOpenResult;
SaveOpenResult save_open_checked(const char *path);
```

- Opens the DB.
- If `sqlite3_open` fails → `SAVE_FAIL`.
- Else `integrity_check`. If not "ok" → call `save_backup("integrity_check_failed")` and return `SAVE_CORRUPT` (DB stays open; migrations NOT run; caller decides).
- Else run migrations and return `SAVE_OK`.

- [ ] **Step 2: Confirmation flow in main.c**

```c
SaveOpenResult r = save_open_checked(db);
if (r == SAVE_FAIL) { /* show fatal modal, exit */ }
if (r == SAVE_CORRUPT) {
    ui_recovery_show(/* "We made a backup at <ts>.bak. Start with a fresh save?" */);
    if (!ui_recovery_user_confirmed()) exit(0);
    save_reset_to_blank();    /* deletes the corrupt db and reopens fresh */
}
```

`ui_recovery_show` runs a minimal full-screen modal loop with `OK` / `Quit` keys; doesn't depend on the regular UI state machine (it runs before `game_create`).

- [ ] **Step 3: Manual smoke**

Manually corrupt the DB (`echo garbage > ~/Library/Application\ Support/AMazeIng/amazeing.db`), boot. Expected: modal appears, `*.bak` file exists, choosing OK creates a fresh DB.

- [ ] **Step 4: Commit**

```bash
git add src/save.* src/main.c src/ui_recovery.c CMakeLists.txt
git commit -m "feat(save): integrity check + backup + recovery dialog at boot"
```

---

### Task 54: Vertical slice acceptance playthrough

**Goal:** Confirm the slice satisfies the §9 MVP acceptance criteria insofar as they apply to a single maze.

**Files:**
- Create: `docs/superpowers/plans/vertical-slice-acceptance-checklist.md`

- [ ] **Step 1: Write the checklist (this is the artifact, not new code)**

```markdown
# Vertical Slice Acceptance Checklist

- [ ] **Completable cold.** Boot. New profile. Maze select → Nature. Reach the gardener, take the dialog (rain_boots granted). Use the seed on the withered plant. Walk through mud (now possible). Use rain_boots on the locked gate. Reach the goal tile. No external instructions consulted.
- [ ] **Resume correctness.** Mid-slice, close the app. Reopen. State resumes at last checkpoint: same position, inventory, items, clock, day.
- [ ] **Reset safety.** Trigger maze reset. Confirm: rain_boots still in profile_items; outfit_worn preserved; maze inventory cleared; entity_overrides cleared; clock back to 08:00 day 0.
- [ ] **Soft-lock resistance.** Use the seed on the locked gate by mistake. Confirm: hint shown, seed not consumed, gate still locked, second pickup not required.
- [ ] **Fully localized.** `grep -rE '"[A-Z][a-zA-Z ]{4,}"' src/ui*.c` returns no English literals — every string went through `t()`.
- [ ] **Accessibility minimum.**
  - Open Settings → Controls. Rebind `ACT_INTERACT` from E to F. Confirm prompt appeared.
  - Open Settings → Audio. Slide each of master/ambience/sfx; effect heard.
  - Open Settings → Display → Text speed = Slow. Trigger a dialog. Text reveals slowly.
  - Verify no color-only puzzle cues in the slice (the locked gate has a lock icon + outline, not just a color).
- [ ] **Save robustness.** Corrupt the DB manually. Reboot. Recovery modal appears. Timestamped backup exists. Confirm + new fresh DB starts.
- [ ] **Performance.** Run on a 2018-era laptop at 1080p windowed. Frame rate ≥ 60 fps observed for 60 seconds of normal play.
- [ ] **Quiet exit.** Quit during play. Reopen. SQLite opens cleanly (no recovery errors logged). Latest checkpoint is present.

If any item fails, file a follow-up task before considering the slice complete.
```

- [ ] **Step 2: Run the checklist manually; mark each box**

This is a playthrough, not an automated test. Allocate ~45 minutes.

- [ ] **Step 3: Commit the completed checklist**

```bash
git add docs/superpowers/plans/vertical-slice-acceptance-checklist.md
git commit -m "docs: vertical slice acceptance checklist"
```

---

## Follow-on plans (out of scope for this plan)

When the slice passes, the next plans should be:

1. **`2026-XX-XX-a-maze-ing-content-nature-final.md`** — promote the slice's Nature maze to a full ~30-60 minute experience: real art, final progression items (winter coat in this maze), full dialog tree, all five hazards exercised.
2. **`2026-XX-XX-a-maze-ing-content-circuit.md`** — Circuit Board maze (color-independent puzzle cues are critical here).
3. **`2026-XX-XX-a-maze-ing-content-village.md`** — Village maze (shopkeeper introduces multi-item picker case at scale).
4. **`2026-XX-XX-a-maze-ing-content-themepark.md`** — Theme Park.
5. **`2026-XX-XX-a-maze-ing-content-snow.md`** — Snow/Ice (requires winter coat from Nature).
6. **`2026-XX-XX-a-maze-ing-packaging.md`** — macOS notarization, Windows zip, Linux AppImage; integrates with the existing `macos-release` skill.
7. **`2026-XX-XX-a-maze-ing-ci.md`** — resolve §11 CI host question; wire validator + test_runner + smoke_test into CI on all three platforms.

The slice's engine should not need substantive changes for these — they're content plans on top of a frozen architecture. If a content plan reveals an engine gap, file a small engine plan first.

---

## Self-review notes

Run after writing this plan — these are the spec coverage and consistency checks the writing-plans skill prescribes.

**Spec coverage by task:**

| Spec section | Covered by |
|---|---|
| §2.1 Core loop | Tasks 22–27, 28–30, 47–49 (in slice form) |
| §2.2 Collect-and-use | Tasks 27, 28, 29 (`door` accept/reject), 49 (withered_plant) |
| §2.3 Soft-lock prevention | Tasks 29 (door rejects on misuse), 49 (slice exercises rule 3), 51 (validator structural check) |
| §2.3 Reset semantics table | Task 16 (`save_reset_maze`), Task 54 (manual check) |
| §2.4 Shape-based collision | Task 3 (`util_math`), Task 24 (`collision`) |
| §2.5 Interaction model | Task 27 + Task 45 (picker UI) |
| §2.6 Maze relationships | Task 42 (hub lock state) |
| §2.7 Item scope | Task 9 (items.txt), Task 15+16 (separate save tables), Task 28 (scope dispatch) |
| §2.8 4–6 mazes | Slice covers one; follow-on plans handle the rest |
| §2.9 Time of day | Tasks 31, 32, 39 (dusk overlay), 44 (sundial fade) |
| §2.10 Saves + 4 profiles | Tasks 13–17 |
| §2.11 Hinting | Tasks 27 (fail-hint path), 30 (NPC hints), 48 (hint strings) |
| §2.12 Audio | Tasks 33, 34, 50 |
| §2.13 Protagonist | Task 25 (outfit field), Task 45 (wear toggle) — face overlay deferred to content plan |
| §2.14 Localization | Tasks 6, 7, 8, 48 |
| §2.15 Accessibility | Tasks 18, 19, 43 (color-independent cues), 44 (text speed), 46 (sliders + remap), 54 (verification) |
| §3.1 Module layout | File structure section |
| §3.2 Dependency rules | Raylib quarantine in CMake; reflected in Task 52 smoke test sources |
| §3.3 Update pipeline | Task 21 (fixed-step), Task 18 (buffered actions) |
| §3.4 Memory ownership | Task 5 (refcount cache), Task 23 (entity arrays stable), Task 7 (loc owns strings) |
| §3.5 Entity model | Task 23 |
| §3.6 Maze data format + stable IDs | Tasks 12, 51 (validator enforces prefix) |
| §3.7 Rendering pipeline | Tasks 36–39 |
| §3.8 Audio mixer | Tasks 33, 34 |
| §3.9 Save system | Tasks 13–17, 53 |
| §3.10 Localization | Tasks 6–8 |
| §4.1 Asset pipeline | Task 47, 50 |
| §4.2 Item definitions | Task 9 |
| §4.3 Dialog definitions | Task 10 |
| §4.4 Sprite manifest + per-instance overrides | Tasks 11, 12 (`shape_override`), 23 (apply on load) |
| §4.5 Conventions doc | Task 47 |
| §5 Tooling (validator) | Task 51 |
| §6 Build + cross-platform | Task 1 (CMake, raylib FetchContent, AMAZEING_RAYLIB_LOCAL), Task 13 (sqlite vendored) |
| §7 Testing | Tests in every task; Tasks 51, 52 are the dedicated test tools |
| §8 Vertical slice | This entire plan |
| §9 MVP acceptance | Task 54 (slice-scoped) |
| §11 Open questions | Defaults adopted: §11 fade-to-black sundial (Task 44), §11 visual hub map (Task 42), §11 LLVM 4-space clang-format (Task 1). CI and telemetry deferred to follow-on plans. |

**Type-consistency review notes (after writing the plan, before execution):**

- `MazeSnapshot` defined in Task 15 has fields `{x, y, facing, clock_min, day_index}`; used identically in Tasks 16, 54.
- `CheckpointPayload` defined in Task 16; consumed in entity-behavior tasks (28–30) via `game_dirty_*` helpers that update those fields.
- `Shape` and `Vec2` defined in Task 3; same names throughout Tasks 11–27.
- `Action` enum defined in Task 18; values referenced by Tasks 19, 20, 27, 40, 46.
- `Entity` struct fields agreed across Task 23 (declaration) and Tasks 24–30 (usage). `e->item.item_id` is `char[48]` (not `ItemId`), matching how `items.c` and `save.c` exchange string ids. The spec's tentative `ItemId` typedef is implemented as a string for debuggability per §3.6 stable-id rationale.
- Behavior names registered in `entity_*_register` (Tasks 28–30, plus `withered_plant` in Task 49)
  match the literals used in maze fixtures (Task 12) and slice content (Task 49): `pickup`, `door`,
  `sign`, `sundial`, `npc`, `withered_plant`, `decor`, hazard variants.

**Placeholder scan:** No "TBD" / "implement later" / "fill in details" / vague error-handling notes. Where implementation outlines are described in prose (parsers, mixer crossfade, validator rules), the prose is concrete enough to produce code without further clarification.

---

## Execution Handoff

**Plan complete and saved to** `docs/superpowers/plans/2026-05-25-a-maze-ing-vertical-slice.md`. Two execution options:

1. **Subagent-Driven (recommended)** — Dispatch a fresh subagent per task, review between tasks, fast iteration. Use `superpowers:subagent-driven-development`.
2. **Inline Execution** — Execute tasks in this session using `superpowers:executing-plans`, batch execution with checkpoints for review.

Which approach?
