# Build

```sh
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/amazeing
```

If raylib is not in a sibling `../raylib` checkout, set:

```sh
AMAZEING_RAYLIB_LOCAL=/path/to/raylib-5.0 cmake -S . -B build
```

The app links the platform SQLite library. The original plan asked for the amalgamation, but network
is restricted in this workspace and no amalgamation was present locally.
