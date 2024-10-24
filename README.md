# gamestreets
OpenStreetMap parser for Godot Engine

# Build
## For Windows, Linux, MacOS

```sh
$ cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<install_folder>
$ cmake --build build --parallel
$ cmake --install build
```

## For web (ensure you have Emscripten installed)
```sh
$ emcmake cmake -B buildweb -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<install_folder>
$ cmake --build buildweb --parallel
$ cmake --install buildweb
```