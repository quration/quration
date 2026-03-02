#!/bin/bash -eu

VCPKG=/vcpkg/scripts/buildsystems/vcpkg.cmake

TEST_DIR=$(dirname "$0")
TEST_BUILD_DIR="$TEST_DIR/build-test"
QRET_DIR="$TEST_DIR/../../../.."
QRET_BUILD_DIR="$TEST_DIR/build-library"
QRET_TEST_DATA_DIR=$(realpath "$TEST_DIR/../../../../quration-core/tests/data")
LIB_DIR="$TEST_DIR/libqret"

BOOST_INCLUDE_DIR="$QRET_BUILD_DIR/vcpkg_installed/x64-linux/include/"
FMT_INCLUDE_DIR="$QRET_BUILD_DIR/vcpkg_installed/x64-linux/include/"
NLOHMANN_JSON_INCLUDE_DIR="$QRET_BUILD_DIR/vcpkg_installed/x64-linux/include/"

echo "Delete cache"
rm -rf $TEST_BUILD_DIR
rm -rf $QRET_BUILD_DIR
rm -rf $LIB_DIR

echo "Build qret library"
cmake -S $QRET_DIR -B $QRET_BUILD_DIR -DCMAKE_TOOLCHAIN_FILE=$VCPKG \
    -DBUILD_SHARED_LIBS=ON -DQRET_DEV_MODE=OFF \
    -DQRET_BUILD_APPLICATION=OFF -DQRET_BUILD_EXAMPLE=OFF -DQRET_BUILD_TEST=OFF -DQRET_BUILD_PYTHON=OFF -DQRET_MEASURE_COVERAGE=OFF \
    -DQRET_USE_PEGTL=ON -DQRET_USE_QULACS=OFF
cmake --build $QRET_BUILD_DIR -- -j4
cmake --install $QRET_BUILD_DIR --prefix $LIB_DIR

echo "Build test using qret library"

CXX_FLAGS="-I $LIB_DIR/include -I $BOOST_INCLUDE_DIR -I $FMT_INCLUDE_DIR -I $NLOHMANN_JSON_INCLUDE_DIR \
           -Wl,-rpath,$LIB_DIR/lib -L $LIB_DIR/lib -l qret-core \
           -std=c++20 -Wall -Wextra \
           -Wno-deprecated-literal-operator \
           -DQRET_TEST_DATA_DIR=\"$QRET_TEST_DATA_DIR\""

echo "Use clang++"
clang++ main.cpp $CXX_FLAGS
./a.out
rm a.out

echo "Use g++"
g++ main.cpp $CXX_FLAGS
./a.out
rm a.out

echo "Use CMake"
cmake -S $TEST_DIR -B $TEST_BUILD_DIR -DCMAKE_TOOLCHAIN_FILE=$VCPKG -DCMAKE_PREFIX_PATH=$LIB_DIR/lib/cmake
cmake --build $TEST_BUILD_DIR
$TEST_BUILD_DIR/library-test
