﻿option(COMPILER_GCC "gcc" ON)
option(COMPILER_CLANG "clang" OFF)
option(COMPILER_64BIT "64bit" OFF)

message(STATUS "CMAKE_HOST_SYSTEM_NAME=${CMAKE_HOST_SYSTEM_NAME}")
if(COMPILER_GCC)
  set(COMPILER mingw)
  if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    if(COMPILER_64BIT)
      set(COMPILER_FRIENDLY mingw_gcc_x64)
      set(ENV{PATH} "C:/msys64/mingw64/bin;C:/msys64/usr/bin")
    else(COMPILER_64BIT)
      set(COMPILER_FRIENDLY mingw_gcc)
      set(ENV{PATH} "C:/msys64/mingw32/bin;C:/msys64/usr/bin")
    endif(COMPILER_64BIT)
    set(CMAKE_C_COMPILER gcc)
    set(CMAKE_CXX_COMPILER g++)
    set(CMAKE_RC_COMPILER windres)
  else()
    list(APPEND GENERATE_OPTIONS "-DCMAKE_SYSTEM_NAME=Windows")
    if(COMPILER_64BIT)
      set(COMPILER_FRIENDLY mingw_gcc_x64)
      set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
      set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
      set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
    else(COMPILER_64BIT)
      set(COMPILER_FRIENDLY mingw_gcc)
      set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
      set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
      set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
    endif(COMPILER_64BIT)
  endif()
elseif(COMPILER_CLANG)
  set(COMPILER mingw)
  if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    if(COMPILER_64BIT)
      set(COMPILER_FRIENDLY mingw_clang_x64)
      set(ENV{PATH} "C:/msys64/mingw64/bin;C:/msys64/usr/bin")
    else(COMPILER_64BIT)
      set(COMPILER_FRIENDLY mingw_clang)
      set(ENV{PATH} "C:/msys64/mingw32/bin;C:/msys64/usr/bin")
    endif(COMPILER_64BIT)
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
    set(CMAKE_RC_COMPILER windres)
  else()
    list(APPEND GENERATE_OPTIONS "-DCMAKE_SYSTEM_NAME=Windows")
    if(COMPILER_64BIT)
      set(COMPILER_FRIENDLY mingw_clang_x64)
      set(CMAKE_C_COMPILER x86_64-w64-mingw32-clang)
      set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-clang++)
      set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
    else(COMPILER_64BIT)
      set(COMPILER_FRIENDLY mingw_clang)
      set(CMAKE_C_COMPILER i686-w64-mingw32-clang)
      set(CMAKE_CXX_COMPILER i686-w64-mingw32-clang++)
      set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
    endif()
  endif()
else()
  message(FATAL_ERROR "check compiler")
endif()

list(APPEND GENERATE_OPTIONS "-G" "Unix Makefiles")
#list(APPEND GENERATE_OPTIONS "-G" Ninja)
list(APPEND GENERATE_OPTIONS "-DCMAKE_BUILD_TYPE=Release")
set(BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/../build_${COMPILER_FRIENDLY}_appveyor")
list(APPEND BUILD_TOOL_OPTIONS "--" "-j")

include(${CMAKE_CURRENT_LIST_DIR}/build_appveyor.cmake)
