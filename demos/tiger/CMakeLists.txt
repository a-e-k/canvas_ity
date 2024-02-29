# CMake build file for PostScript tiger demo v1.00 -- ISC license
# Copyright (c) 2024 Andrew Kensler
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
# This is the CMake build script for the PostScript tiger demo for the
# canvas library.  Note that this file is not strictly necessary to build
# the demo program; instead, it can just be compiled directly to an
# executable with a C++ compiler, e.g.:
#     g++ -Ofast -march=native -mtune=native -o tiger tiger.cpp
#
# However, building with CMake enables extensive warnings when building with
# GCC, Clang, ICC, or MSVC.
#
# For the best results when benchmarking with GCC or Clang, build with:
#     -DCMAKE_BUILD_TYPE=Release
#     -DCMAKE_RELEASE_CXX_FLAGS="-Ofast -march=native -mtune=native"

cmake_minimum_required( VERSION 3.15 )
project( tiger )

add_executable( tiger tiger.cpp )
target_compile_options( tiger PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang,Intel>: -Wall -Wextra -pedantic -Wshadow -Wdisabled-optimization -Wformat=2 -Winit-self -Wmissing-include-dirs -Woverloaded-virtual -Wsign-promo -Wundef -fdiagnostics-show-option -Wconversion -Wsign-conversion>
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:       -Wdouble-promotion -Wcast-align -Wctor-dtor-privacy -Wredundant-decls -Wstrict-overflow=2 -Wold-style-cast -Wnull-dereference>
  $<$<CXX_COMPILER_ID:GNU>:                        -Wlogical-op -Wduplicated-branches -Wduplicated-cond -Wnoexcept -Wstrict-null-sentinel -Wuseless-cast>
  $<$<CXX_COMPILER_ID:MSVC>:                       /permissive- /W4> )
