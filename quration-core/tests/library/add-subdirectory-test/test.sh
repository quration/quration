#!/bin/bash -eu

VCPKG=/vcpkg/scripts/buildsystems/vcpkg.cmake

rm -rf build
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$VCPKG
cmake --build build/
./build/library-test
