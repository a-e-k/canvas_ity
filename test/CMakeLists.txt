# CMake build file for canvas_ity test suite and harness v1.00 -- ISC license
# Copyright (c) 2022 Andrew Kensler
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

# ======== ABOUT ========
#
# This is the CMake build script for the canvas_ity test suite and harness.
# Note that this file is not strictly necessary to build the test program;
# instead, it can just be compiled directly to an executable with a C++
# compiler, e.g.:
#     g++ -O3 -o test test.cpp
#
# However, building with CMake enables extensive warnings when building with
# GCC, Clang, ICC, or MSVC, and also offers targets for static analysis,
# dynamic analysis, and measuring test coverage.  Beyond that, it includes
# targets for measuring the size of the library itself.
#
# These are the main custom options:
#
# - WITH_SANITIZERS: builds the test program with undefined behavior, address,
#     and/or integer sanitizers depending on the compiler
# - WITH_COVERAGE: build the test program with instrumentation for measuring
#     test coverage with either gcov or llvm-cov.
#
# These are the main targets offered.  Note that some of them may be
# unavailable if the requisite tools are not found or options disabled:
#
# - all / canvas_test: build the test program, either test or test.exe.
# - coverage: build the test program, execute it to measure coverage, and
#     then show how much of canvas_ity.hpp was tested while putting a full
#     report in canvas_ity.hpp.gcov.
# - valgrind: build the test program, and then execute it under valgrind to
#     evaluate it for memory errors.
# - analyze: check the test program and library using the Clang
#     static analyzer.
# - cppcheck: check the test program and library using the CppCheck tool.
# - size: compile the library implementation to an object file using custom
#     build flags for minimal size (ignoring the usual CMake build flags) and
#     then measure the size of the executable code with the size tool.
# - lines: count the lines of source code in the library using the cloc tool.
# - semis: count the number of semicolons in the library source code.
# - test: drive the test program via CTest (for IDE test support).

cmake_minimum_required( VERSION 3.15 )
project( test )

add_executable( canvas_test test.cpp )
set_target_properties( canvas_test PROPERTIES OUTPUT_NAME test )
target_compile_options( canvas_test PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang,Intel>: -Wall -Wextra -std=c++98 -pedantic -Wshadow -Wdisabled-optimization -Wformat=2 -Winit-self -Wmissing-include-dirs -Woverloaded-virtual -Wsign-promo -Wundef -fdiagnostics-show-option -Wconversion -Wsign-conversion>
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:       -Wdouble-promotion -Wcast-align -Wctor-dtor-privacy -Wredundant-decls -Wstrict-overflow=2 -Wold-style-cast -Wnull-dereference>
  $<$<CXX_COMPILER_ID:GNU>:                        -Wlogical-op -Wduplicated-branches -Wduplicated-cond -Wnoexcept -Wstrict-null-sentinel -Wuseless-cast>
  $<$<CXX_COMPILER_ID:MSVC>:                       /permissive- /W4>
  $<$<AND:$<CONFIG:MinSizeRel>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang,Intel>>: -ffast-math -fno-math-errno -fno-exceptions -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -fno-ident>
  $<$<AND:$<CONFIG:MinSizeRel>,$<CXX_COMPILER_ID:GNU>>:                        -flto>
  $<$<AND:$<CONFIG:MinSizeRel>,$<CXX_COMPILER_ID:Intel>>:                      -ipo> )
target_link_options( canvas_test PRIVATE
  $<$<AND:$<CONFIG:MinSizeRel>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang,Intel>>: -s -Wl,--gc-sections -Wl,--hash-style=gnu -fdata-sections -fno-asynchronous-unwind-tables>
  $<$<AND:$<CONFIG:MinSizeRel>,$<CXX_COMPILER_ID:GNU,Intel>>:                  -Wl,-z,norelro>
  $<$<AND:$<CONFIG:MinSizeRel>,$<CXX_COMPILER_ID:GNU>>:                        -flto> )

option( WITH_SANITIZERS "Build with undefined behavior, address, and/or integer sanitizer" )
if( WITH_SANITIZERS )
  target_compile_options( canvas_test PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>: -fsanitize=undefined -fsanitize=address>
    $<$<CXX_COMPILER_ID:GNU,Clang>:            -fsanitize=leak>
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:     -fsanitize=integer>
    $<$<CXX_COMPILER_ID:MSVC>:                 /fsanitize=address /MTd> )
  target_link_options( canvas_test PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>: -fsanitize=undefined -fsanitize=address>
    $<$<CXX_COMPILER_ID:GNU,Clang>:            -fsanitize=leak>
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:     -fsanitize=integer>
    $<$<CXX_COMPILER_ID:MSVC>:                 /INCREMENTAL:NO> )
endif()

option( WITH_COVERAGE "Build test with coverage profiling of the library" )
if( NOT TOOL_COVERAGE )
  string( REPLACE "clang++" "llvm-cov" TOOL_COVERAGE ${CMAKE_CXX_COMPILER} )
  string( REPLACE "g++" "gcov" TOOL_COVERAGE ${TOOL_COVERAGE} )
  if( NOT EXISTS ${TOOL_COVERAGE} OR ${TOOL_COVERAGE} STREQUAL ${CMAKE_CXX_COMPILER} )
    set( TOOL_COVERAGE "" )
  endif()
  set( TOOL_COVERAGE ${TOOL_COVERAGE} CACHE PATH "Path to coverage tool for C++ compiler (gcov or llvm-cov)" FORCE )
endif()
if( WITH_COVERAGE AND TOOL_COVERAGE )
  target_compile_options( canvas_test PRIVATE --coverage -fprofile-filter-files=canvas_ity.hpp )
  target_link_options( canvas_test PRIVATE --coverage )
  set_source_files_properties( test.cpp PROPERTIES OBJECT_OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/canvas_test.dir/test.cpp.gcno )
  add_custom_command(
    TARGET canvas_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/canvas_test.dir/test.cpp.gcda )
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/canvas_test.dir/test.cpp.gcda
    COMMAND canvas_test
    USES_TERMINAL )
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/canvas_ity.hpp.gcov
    COMMAND ${TOOL_COVERAGE} $<$<IN_LIST:${CMAKE_CXX_COMPILER_ID},Clang$<SEMICOLON>AppleClang>:gcov> ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/canvas_test.dir/test.cpp.gcno
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/canvas_test.dir/test.cpp.gcno ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/canvas_test.dir/test.cpp.gcda )
  add_custom_target( coverage
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/canvas_ity.hpp.gcov )
endif()

find_program( TOOL_VALGRIND valgrind )
if( TOOL_VALGRIND )
  add_custom_target( valgrind
    COMMAND ${TOOL_VALGRIND} --tool=memcheck --error-limit=no --leak-resolution=high --show-reachable=yes --leak-check=full --trace-children=yes --undef-value-errors=yes --track-origins=yes $<TARGET_FILE:canvas_test>
    USES_TERMINAL )
endif()

if( NOT TOOL_ANALYZE )
  if( ${CMAKE_CXX_COMPILER_ID} MATCHES "^(Clang|AppleClang)$" )
    set( TOOL_ANALYZE ${CMAKE_CXX_COMPILER} CACHE PATH "Path to clang++ for static analysis" FORCE )
  endif()
  find_program( TOOL_ANALYZE clang++ )
endif()
if( TOOL_ANALYZE )
  add_custom_target( analyze
    COMMAND ${TOOL_ANALYZE} --analyze -Xanalyzer -analyzer-output=text ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
    USES_TERMINAL )
endif()

find_program( TOOL_CPPCHECK cppcheck )
if( TOOL_CPPCHECK )
  add_custom_target( cppcheck
    COMMAND ${TOOL_CPPCHECK} --enable=all --std=c++03 --suppress=missingIncludeSystem --suppress=useInitializationList ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
    USES_TERMINAL )
endif()

find_program( TOOL_SIZE size )
if( TOOL_SIZE )
  add_custom_target( size
    COMMAND ${CMAKE_CXX_COMPILER} -Os $<$<NOT:$<IN_LIST:${CMAKE_CXX_COMPILER_ID},Clang$<SEMICOLON>AppleClang>>:-ffast-math> -fno-math-errno -fno-exceptions -fno-asynchronous-unwind-tables -fmerge-all-constants -ffunction-sections -fdata-sections -c -o ${CMAKE_CURRENT_BINARY_DIR}/canvas_ity.o -x c++ -DCANVAS_ITY_IMPLEMENTATION ${CMAKE_CURRENT_SOURCE_DIR}/../src/canvas_ity.hpp
    COMMAND ${TOOL_SIZE} ${CMAKE_CURRENT_BINARY_DIR}/canvas_ity.o
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/canvas_ity.o )
endif()

find_program( TOOL_CLOC cloc )
if( TOOL_CLOC )
  add_custom_target( lines
    COMMAND ${TOOL_CLOC} ${CMAKE_CURRENT_SOURCE_DIR}/../src/canvas_ity.hpp
    USES_TERMINAL )
endif()

find_program( TOOL_TR tr )
find_program( TOOL_WC wc )
if( TOOL_TR AND TOOL_WC )
  add_custom_target( semis
    COMMAND ${TOOL_TR} -cd \; < ${CMAKE_CURRENT_SOURCE_DIR}/../src/canvas_ity.hpp | ${TOOL_WC} -c
    USES_TERMINAL
    VERBATIM )
endif()

enable_testing()
file( STRINGS test.cpp TEST_TABLE
      REGEX "^    { 0x[0-9a-f]+, [0-9]+, [0-9]+, [a-z_]+, \"[a-z_]+\" },$" )
foreach( TEST_ENTRY ${TEST_TABLE} )
  string( REGEX MATCH "\"([a-z_]+)\"" TEST_NAME ${TEST_ENTRY} )
  add_test( NAME ${CMAKE_MATCH_1}
            COMMAND canvas_test --subset ${CMAKE_MATCH_1} )
endforeach( TEST_ENTRY )
