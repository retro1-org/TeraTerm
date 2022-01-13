﻿# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=Win32 -P oniguruma.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -P oniguruma.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 15 2017" -DCMAKE_CONFIGURATION_TYPE=Release -P oniguruma.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=Win32 -DCMAKE_CONFIGURATION_TYPE=Release -P oniguruma.cmake
# cmake -DCMAKE_GENERATOR="Visual Studio 16 2019" -DARCHITECTURE=x64 -DCMAKE_CONFIGURATION_TYPE=Release -P oniguruma.cmake

####
if(("${CMAKE_BUILD_TYPE}" STREQUAL "") AND ("${CMAKE_CONFIGURATION_TYPE}" STREQUAL ""))
  if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
	# multi-configuration
	execute_process(
	  COMMAND ${CMAKE_COMMAND}
	  -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
	  -DCMAKE_CONFIGURATION_TYPE=Release
	  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/VSToolchain.cmake
	  -DARCHITECTURE=${ARCHITECTURE}
	  -P oniguruma.cmake
	  )
	execute_process(
	  COMMAND ${CMAKE_COMMAND}
	  -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
	  -DCMAKE_CONFIGURATION_TYPE=Debug
	  -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/VSToolchain.cmake
	  -DARCHITECTURE=${ARCHITECTURE}
	  -P oniguruma.cmake
	  )
	return()
  elseif(("$ENV{MSYSTEM}" MATCHES "MINGW") OR ("${CMAKE_COMMAND}" MATCHES "mingw"))
	# mingw on msys2
	if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	  set(CMAKE_BUILD_TYPE Release)
	endif()
  elseif("${CMAKE_GENERATOR}" MATCHES "Unix Makefiles")
	# mingw
	# single-configuration
	if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
	  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../mingw.toolchain.cmake")
	endif()
	if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	  set(CMAKE_BUILD_TYPE Release)
	endif()
  elseif("${CMAKE_GENERATOR}" MATCHES "NMake Makefiles")
	# VS nmake
	# single-configuration
	if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
	  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/VSToolchain.cmake")
	endif()
	if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	  set(CMAKE_BUILD_TYPE Release)
	endif()
  else()
	# single-configuration
	if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	  set(CMAKE_BUILD_TYPE Release)
	endif()
  endif()
endif()

include(script_support.cmake)

set(SRC_DIR_BASE "onig-6.9.7.1")
set(SRC_ARC "${SRC_DIR_BASE}.tar.gz")
set(SRC_URL "https://github.com/kkos/oniguruma/releases/download/v6.9.7.1/${SRC_ARC}")
set(SRC_ARC_HASH_SHA256 6444204b9c34e6eb6c0b23021ce89a0370dad2b2f5c00cd44c342753e0b204d9)

set(DOWN_DIR "${CMAKE_SOURCE_DIR}/download/oniguruma")
set(EXTRACT_DIR "${CMAKE_SOURCE_DIR}/build/oniguruma/src")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/build/oniguruma/src/onig-6.9.7")
set(BUILD_DIR "${CMAKE_SOURCE_DIR}/build/oniguruma/build_${TOOLSET}")
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/oniguruma_${TOOLSET}")
if(("${CMAKE_GENERATOR}" MATCHES "Win64") OR ("${ARCHITECTURE}" MATCHES "x64") OR ("$ENV{MSYSTEM_CHOST}" STREQUAL "x86_64-w64-mingw32") OR ("${CMAKE_COMMAND}" MATCHES "mingw64"))
  set(BUILD_DIR "${BUILD_DIR}_x64")
  set(INSTALL_DIR "${INSTALL_DIR}_x64")
endif()

########################################

if(NOT EXISTS ${SRC_DIR}/README.md)

  file(DOWNLOAD
    ${SRC_URL}
    ${DOWN_DIR}/${SRC_ARC}
    EXPECTED_HASH SHA256=${SRC_ARC_HASH_SHA256}
    SHOW_PROGRESS
    )

  file(MAKE_DIRECTORY ${EXTRACT_DIR})

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar "xvf" ${DOWN_DIR}/${SRC_ARC}
    WORKING_DIRECTORY ${EXTRACT_DIR}
    )

  file(COPY
    ${SRC_DIR}/COPYING
    DESTINATION ${CMAKE_CURRENT_LIST_DIR}/doc_help)
  file(RENAME
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/COPYING
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/Oniguruma-LICENSE.txt)

  file(COPY
    ${SRC_DIR}/doc/RE
    DESTINATION ${CMAKE_CURRENT_LIST_DIR}/doc_help/en)
  file(COPY
    ${SRC_DIR}/doc/RE.ja
    DESTINATION ${CMAKE_CURRENT_LIST_DIR}/doc_help/ja)
  file(RENAME
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/ja/RE.ja
    ${CMAKE_CURRENT_LIST_DIR}/doc_help/ja/RE)
endif()

########################################

file(MAKE_DIRECTORY "${BUILD_DIR}")

if("${CMAKE_GENERATOR}" MATCHES "Visual Studio")

  ######################################## multi configuration

  if(NOT "${ARCHITECTURE}" STREQUAL "")
	set(CMAKE_A_OPTION -A ${ARCHITECTURE})
  endif()
  execute_process(
	COMMAND ${CMAKE_COMMAND} ${SRC_DIR} -G ${CMAKE_GENERATOR} ${CMAKE_A_OPTION}
	-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
	-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
	${TOOLCHAINFILE}
	-DCMAKE_DEBUG_POSTFIX=d
	-DBUILD_SHARED_LIBS=OFF
	-DMSVC_STATIC_RUNTIME=ON
	WORKING_DIRECTORY ${BUILD_DIR}
	RESULT_VARIABLE rv
	)
  if(NOT rv STREQUAL "0")
	message(FATAL_ERROR "cmake generate fail ${rv}")
  endif()

  execute_process(
	COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_CONFIGURATION_TYPE} --target install
	WORKING_DIRECTORY ${BUILD_DIR}
	RESULT_VARIABLE rv
	)
  if(NOT rv STREQUAL "0")
	message(FATAL_ERROR "cmake install fail ${rv}")
  endif()

else()
  ######################################## single configuration
  
  execute_process(
	COMMAND ${CMAKE_COMMAND} ${SRC_DIR} -G ${CMAKE_GENERATOR}
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
	-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
	-DBUILD_SHARED_LIBS=OFF
	WORKING_DIRECTORY ${BUILD_DIR}
	RESULT_VARIABLE rv
	)
  if(NOT rv STREQUAL "0")
	message(FATAL_ERROR "cmake build fail ${rv}")
  endif()
  
  execute_process(
	COMMAND ${CMAKE_COMMAND} --build . --target install
	WORKING_DIRECTORY ${BUILD_DIR}
	RESULT_VARIABLE rv
	)
  if(NOT rv STREQUAL "0")
	message(FATAL_ERROR "cmake install fail ${rv}")
  endif()

endif()
