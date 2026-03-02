set(CORE_LIB_NAME "${PROJECT_NAME}-core")

function(add_test directory filename)
  string(REPLACE "/" "_" directory_name ${directory})
  set(target ${directory_name}_${filename})
  add_executable(${target} ${directory}/${filename}.cpp)
  target_link_libraries(${target} PRIVATE GTest::gtest GTest::gtest_main
                                          ${CORE_LIB_NAME})
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
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

# analysis
add_test(analysis circuit_drawer)
add_test(analysis compute_graph)

# base
add_test(base cast)
add_test(base graph)
add_test(base gridsynth)
add_test(base iterator)
add_test(base list)
add_test(base option)
add_test(base portable_function)
add_test(base string)
add_test(base type)

# frontend
add_test(frontend builder)
add_test(frontend functor)
add_test(frontend openqasm2)
add_test(frontend openqasm3)

# ir
add_test(ir circuit)
add_test(ir instructions)
add_test(ir json)

# math
add_test(math boolean_formula)
add_test(math clifford)
add_test(math fraction)
add_test(math pauli)

# parser
add_test(parser openqasm2)
add_test(parser openqasm3)

# runtime
add_test(runtime call_stack)
add_test(runtime clifford_state)
add_test(runtime full_quantum_state)
add_test(runtime simulator)
add_test(runtime toffoli_state)

# target
add_test(target/sc_ls_fixed_v0 asm_printer)
add_test(target/sc_ls_fixed_v0 calc_compile_info_qec)
add_test(target/sc_ls_fixed_v0 geometry)
add_test(target/sc_ls_fixed_v0 pipeline_state)
add_test(target/sc_ls_fixed_v0 lowering)
add_test(target/sc_ls_fixed_v0 mapping)
add_test(target/sc_ls_fixed_v0 routing)
add_test(target/sc_ls_fixed_v0 search_chip_comm)
add_test(target/sc_ls_fixed_v0 state)
add_test(target/sc_ls_fixed_v0 topology)
add_test(target/sc_ls_fixed_v0 validation)

# transforms
add_test(transforms/ipo inliner)
add_test(transforms/scalar decomposition)
add_test(transforms/scalar delete_consecutive_same_pauli)
add_test(transforms/scalar delete_opt_hint)
add_test(transforms/scalar ignore_global_phase)
add_test(transforms/scalar static_condition_pruning)

# Test cmd
if(QRET_BUILD_APPLICATION)
  add_test(cmd main)
endif()
