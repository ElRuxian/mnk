#!/bin/sh
cmake -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build --target main
