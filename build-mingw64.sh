#!/bin/sh
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw64.cmake -B build-win
cmake --build build-win --target main
