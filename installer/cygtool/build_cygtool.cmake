﻿#
if(NOT DEFINED CMAKE_GENERATOR)
  if(${CMAKE_COMMAND} MATCHES "mingw")
    # meybe mingw
    set(CMAKE_GENERATOR "Unix Makefiles")
  elseif(CMAKE_HOST_WIN32)
    set(CMAKE_GENERATOR "Visual Studio 16 2019")
  else()
    set(CMAKE_GENERATOR "Unix Makefiles")
  endif()
endif()
if((${CMAKE_GENERATOR} MATCHES "Visual Studio 16 2019") OR
    (${CMAKE_GENERATOR} MATCHES "Visual Studio 17 2022"))
  # 32bit build for inno setup
  set(GENERATE_OPTION "-A;Win32")
endif()

message("CMAKE_GENERATOR=${CMAKE_GENERATOR}")
message("CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")

file(REMOVE_RECURSE build)
file(MAKE_DIRECTORY build)

execute_process(
  COMMAND ${CMAKE_COMMAND} .. -G ${CMAKE_GENERATOR} ${GENERATE_OPTION} -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
  WORKING_DIRECTORY "build"
  )

execute_process(
  COMMAND ${CMAKE_COMMAND} --build . --config release
  WORKING_DIRECTORY "build"
  )

if(DEFINED CMAKE_INSTALL_PREFIX)
  execute_process(
    COMMAND ${CMAKE_COMMAND} --build . --config release --target install
    WORKING_DIRECTORY "build"
    )
endif()
