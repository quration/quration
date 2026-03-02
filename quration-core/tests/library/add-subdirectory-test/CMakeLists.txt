cmake_minimum_required(VERSION 3.16)

project(add-subdirectory-test CXX)

# qret
add_subdirectory(../../../.. qret)

add_executable(library-test main.cpp)

target_link_libraries(library-test qret-core::qret-core)
get_filename_component(QRET_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../.."
                       ABSOLUTE)
target_compile_definitions(
  library-test
  PRIVATE QRET_TEST_DATA_DIR="${QRET_ROOT_DIR}/quration-core/tests/data")
set_target_properties(
  library-test
  PROPERTIES CXX_STANDARD 20
             CXX_STANDARD_REQUIRED ON
             CXX_EXTENSIONS OFF)
