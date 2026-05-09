@echo off

set BuildDir=build
if not exist %BuildDir% mkdir %BuildDir%
pushd %BuildDir%

set CommonCompilerFlags=/Od /Oi /Z7 /Zo /MTd /fp:fast /fp:except- /nologo /FC /GR- /Gm- /EHa- /W4 /WX /wd4100 /wd4189 /wd4201 /wd4505 /DGAME_SLOW=1 /DGAME_INTERNAL=1
set CommonLinkerFlags=/opt:ref /incremental:no

echo Building DLL... > lock.tmp
del game.pdb 2> NUL
cl %CommonCompilerFlags% /Fmgame.map /LD ..\game.cpp /link %CommonLinkerFlags% /EXPORT:GameUpdateAndRender
del lock.tmp

cl %CommonCompilerFlags% ..\win32_game.cpp user32.lib gdi32.lib winmm.lib /link %CommonLinkerFlags%

set WebBuildDir=%BuildDir%\web
if not exist %WebBuildDir% mkdir %WebBuildDir%
clang -fno-builtin-memset -Wno-writable-strings -DWASM_BUILD=1 -DGAME_SLOW=1 -DGAME_INTERNAL=1 --target=wasm32 -std=c++11 -nostdlib -O3 -Wl,--no-entry,--export="GameUpdateAndRender",--import-memory -o %WebBuildDir%\game.wasm ..\game.cpp

popd
