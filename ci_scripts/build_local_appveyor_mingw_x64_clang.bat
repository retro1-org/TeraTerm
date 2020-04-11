setlocal
set COMPILER=mingw_x64
set COMPILER_FRIENDLY=mingw_x64_clang
set GENERATOR=Unix Makefiles
set CMAKE_COMMAND=cmake
set CMAKE_OPTION_LIBS=
set CMAKE_OPTION_GENERATE=-DCMAKE_BUILD_TYPE=Release
set CMAKE_OPTION_BUILD=
set MINGW_CC=clang
set MINGW_CXX=clang++
set BUILD_DIR=build_%COMPILER_FRIENDLY%_msys2

cd /d %~dp0..
call ci_scripts\build_appveyor.bat
