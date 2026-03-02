set(CORE_LIB_NAME "${PROJECT_NAME}-core")
set(ALGORITHM_LIB_NAME "${PROJECT_NAME}-algorithm")

function(add_test directory filename)
  string(REPLACE "/" "_" directory_name ${directory})
  set(target ${directory_name}_${filename})
  add_executable(${target} ${directory}/${filename}.cpp)
  target_link_libraries(
    ${target} PRIVATE GTest::gtest GTest::gtest_main ${CORE_LIB_NAME}
                      ${ALGORITHM_LIB_NAME})
  set_target_properties(
    ${target}
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF)

  if(QRET_MEASURE_COVERAGE)
    target_compile_options(${target} PRIVATE -g -fprofile-instr-generate
                                             -fcoverage-mapping)
    target_link_options(${target} PRIVATE -g -fprofile-instr-generate
                        -fcoverage-mapping)
  endif()

  gtest_discover_tests(
    ${target}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} DISCOVERY_MODE PRE_TEST
    PROPERTIES ENVIRONMENT GRIDSYNTH_PATH=${GRIDSYNTH_PATH})
endfunction()

# gate
add_test(gate/algorithm qft)
add_test(gate/algorithm qpe)
add_test(gate/arithmetic boolean)
add_test(gate/frontend circuit)
add_test(gate/arithmetic integer_add)
add_test(gate/arithmetic integer_compare)
add_test(gate/arithmetic integer_flip)
add_test(gate/arithmetic integer_inc)
add_test(gate/arithmetic integer_mul)
add_test(gate/arithmetic integer_scaled_add)
add_test(gate/arithmetic integer_sub)
add_test(gate/arithmetic modular)
add_test(gate/data qdict)
add_test(gate/data qrom)
add_test(gate/preparation arbitrary)
add_test(gate/preparation uniform)
add_test(gate/util bitwise)
add_test(gate/util multi_control)
add_test(gate/util rotation)
add_test(gate/util util)
