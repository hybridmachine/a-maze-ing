cmake_minimum_required(VERSION 3.20)
project(amazeing C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT MSVC)
    add_compile_options($<$<CONFIG:Debug>:-fsanitize=address,undefined>)
    add_link_options($<$<CONFIG:Debug>:-fsanitize=address,undefined>)
endif()

function(amazeing_strict target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

include(FetchContent)
if(DEFINED ENV{AMAZEING_RAYLIB_LOCAL})
    add_subdirectory($ENV{AMAZEING_RAYLIB_LOCAL} raylib_build)
elseif(EXISTS "${CMAKE_SOURCE_DIR}/../raylib/CMakeLists.txt")
    add_subdirectory("${CMAKE_SOURCE_DIR}/../raylib" raylib_build)
else()
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.0)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/sqlite/sqlite3.c" AND
   EXISTS "${CMAKE_SOURCE_DIR}/third_party/sqlite/sqlite3.h")
    add_library(sqlite3 STATIC third_party/sqlite/sqlite3.c)
    target_compile_definitions(sqlite3 PUBLIC
        SQLITE_THREADSAFE=0
        SQLITE_OMIT_LOAD_EXTENSION=1
        SQLITE_DEFAULT_WAL_SYNCHRONOUS=1)
    target_include_directories(sqlite3 PUBLIC third_party/sqlite)
    if(NOT MSVC)
        target_compile_options(sqlite3 PRIVATE -w)
    endif()
else()
    message(WARNING
        "third_party/sqlite/sqlite3.c and sqlite3.h were not found; "
        "falling back to the platform SQLite library.")
    find_library(SQLITE3_LIBRARY sqlite3 REQUIRED)
    add_library(sqlite3 INTERFACE)
    target_link_libraries(sqlite3 INTERFACE ${SQLITE3_LIBRARY})
endif()

set(AMAZEING_CORE_SOURCES
    src/util_log.c
    src/util_paths.c
    src/util_math.c
    src/localization.c
    src/items.c
    src/dialogs.c
    src/sprite_manifest.c
    src/maze_data.c
    src/save.c
    src/profile.c
    src/input.c
    src/entity.c
    src/world.c
    src/collision.c
    src/player.c
    src/interaction.c
    src/entity_item.c
    src/entity_hazard.c
    src/entity_interact.c
    src/entity_npc.c
    src/time_of_day.c
    src/camera.c
    src/shadow.c
    src/parallax.c
    src/game.c)

set(AMAZEING_APP_SOURCES
    src/main.c
    src/asset.c
    src/audio.c
    src/render.c
    src/render_world.c
    src/render_ui.c
    src/ui.c
    src/ui_title.c
    src/ui_profile.c
    src/ui_maze_select.c
    src/ui_hud.c
    src/ui_dialog.c
    src/ui_inventory.c
    src/ui_settings.c
    src/ui_recovery.c)

add_executable(amazeing ${AMAZEING_CORE_SOURCES} ${AMAZEING_APP_SOURCES})
target_include_directories(amazeing PRIVATE src)
target_link_libraries(amazeing PRIVATE raylib sqlite3 m)
amazeing_strict(amazeing)
if(APPLE)
    set_target_properties(amazeing PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "A Maze Ing"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.btabone.amazeing"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0"
        MACOSX_BUNDLE_BUNDLE_VERSION "0.1.0")
    add_custom_command(TARGET amazeing POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_BUNDLE_CONTENT_DIR:amazeing>/Resources/assets"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_BUNDLE_CONTENT_DIR:amazeing>/Resources/data"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:amazeing>/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_BUNDLE_CONTENT_DIR:amazeing>/Resources/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/data" "$<TARGET_BUNDLE_CONTENT_DIR:amazeing>/Resources/data"
        VERBATIM)
endif()

enable_testing()
add_executable(test_runner
    third_party/test_harness/test_harness.c
    tests/test_main.c
    tests/test_util_math.c
    tests/test_util_paths.c
    tests/test_localization.c
    tests/test_items.c
    tests/test_dialogs.c
    tests/test_sprite_manifest.c
    tests/test_maze_data.c
    tests/test_save_roundtrip.c
    tests/test_save_migrations.c
    tests/test_save_reset.c
    tests/test_input_remap.c
    tests/test_collision.c
    tests/test_entity_behaviors.c
    tests/test_interaction.c
    tests/test_dialog_repeat.c
    tests/test_time_of_day.c
    tests/test_camera.c
    tests/test_shadow.c
    tests/test_validator.c
    tests/test_game_fixture.c
    tools/validate.c
    tools/headless_ui_stubs.c
    ${AMAZEING_CORE_SOURCES})
target_compile_definitions(test_runner PRIVATE AMAZEING_TESTS AMAZEING_HEADLESS)
target_include_directories(test_runner PRIVATE src third_party/test_harness tools)
target_link_libraries(test_runner PRIVATE sqlite3 m)
amazeing_strict(test_runner)
add_test(NAME test_runner COMMAND test_runner WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(validate
    tools/validate.c
    tools/validator_registry.c
    ${AMAZEING_CORE_SOURCES}
    tools/headless_ui_stubs.c)
target_compile_definitions(validate PRIVATE AMAZEING_HEADLESS)
target_include_directories(validate PRIVATE src tools)
target_link_libraries(validate PRIVATE sqlite3 m)
amazeing_strict(validate)

add_executable(smoke_test
    tools/smoke_test.c
    tools/headless_ui_stubs.c
    ${AMAZEING_CORE_SOURCES})
target_compile_definitions(smoke_test PRIVATE AMAZEING_HEADLESS)
target_include_directories(smoke_test PRIVATE src tools)
target_link_libraries(smoke_test PRIVATE sqlite3 m)
amazeing_strict(smoke_test)
add_test(NAME smoke_test COMMAND smoke_test WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(validate_all
    COMMAND validate --strict ${CMAKE_SOURCE_DIR}/data/mazes/nature.maze
    DEPENDS validate
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
