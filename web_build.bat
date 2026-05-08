@echo off

set BuildDir=build\web
if not exist %BuildDir% mkdir %BuildDir%

clang -fno-builtin-memset -Wno-writable-strings -DWASM_BUILD=1 -DGAME_SLOW=1 -DGAME_INTERNAL=1 --target=wasm32 -std=c++11 -nostdlib -O3 -Wl,--no-entry,--export="GameUpdateAndRender",--import-memory -o %BuildDir%\game.wasm game.cpp
