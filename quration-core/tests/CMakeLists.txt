set(CORE_LIB_NAME "${PROJECT_NAME}-core")

function(add_qret_test directory filename)
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

  set(test_environment "GRIDSYNTH_PATH=${GRIDSYNTH_PATH}")
  set(test_environment_modification
      "PATH=path_list_prepend:$<TARGET_FILE_DIR:${CORE_LIB_NAME}>"
      "PATH=path_list_prepend:$<TARGET_FILE_DIR:${target}>")

  if(WIN32)
    gtest_add_tests(
      TARGET ${target}
      SOURCES ${directory}/${filename}.cpp
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      TEST_LIST ${target}_tests)
    if(${target}_tests)
      set_tests_properties(
        ${${target}_tests}
        PROPERTIES ENVIRONMENT "${test_environment}"
                   ENVIRONMENT_MODIFICATION "${test_environment_modification}")
    endif()
  else()
    gtest_discover_tests(
      ${target}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} DISCOVERY_MODE PRE_TEST
      PROPERTIES ENVIRONMENT "${test_environment}" ENVIRONMENT_MODIFICATION
                 "${test_environment_modification}")
  endif()
endfunction()

# analysis
add_qret_test(analysis circuit_drawer)
add_qret_test(analysis compute_graph)

# base
add_qret_test(base cast)
add_qret_test(base graph)
add_qret_test(base gridsynth)
add_qret_test(base iterator)
add_qret_test(base list)
add_qret_test(base option)
add_qret_test(base portable_function)
add_qret_test(base string)
add_qret_test(base type)

# frontend
add_qret_test(frontend builder)
add_qret_test(frontend functor)
add_qret_test(frontend openqasm2)
add_qret_test(frontend openqasm3)
add_qret_test(frontend qualtran)

# ir
add_qret_test(ir circuit)
add_qret_test(ir instructions)
add_qret_test(ir json)

# math
add_qret_test(math boolean_formula)
add_qret_test(math clifford)
add_qret_test(math fraction)
add_qret_test(math pauli)

# parser
add_qret_test(parser openqasm2)
add_qret_test(parser openqasm3)

# runtime
add_qret_test(runtime call_stack)
add_qret_test(runtime clifford_state)
add_qret_test(runtime full_quantum_state)
add_qret_test(runtime simulator)
add_qret_test(runtime toffoli_state)

# target
add_qret_test(target/sc_ls_fixed_v0 asm_printer)
add_qret_test(target/sc_ls_fixed_v0 calc_compile_info)
add_qret_test(target/sc_ls_fixed_v0 calc_compile_info_qec)
add_qret_test(target/sc_ls_fixed_v0 magic_factory_cell_count)
add_qret_test(target/sc_ls_fixed_v0 geometry)
add_qret_test(target/sc_ls_fixed_v0 pipeline_state)
add_qret_test(target/sc_ls_fixed_v0 lowering)
add_qret_test(target/sc_ls_fixed_v0 mapping)
add_qret_test(target/sc_ls_fixed_v0 routing)
add_qret_test(target/sc_ls_fixed_v0 search_grid)
add_qret_test(target/sc_ls_fixed_v0 search_chip_comm)
add_qret_test(target/sc_ls_fixed_v0 simulator)
add_qret_test(target/sc_ls_fixed_v0 state)
add_qret_test(target/sc_ls_fixed_v0 topology)
add_qret_test(target/sc_ls_fixed_v0 validation)

# transforms
add_qret_test(transforms/ipo inliner)
add_qret_test(transforms/scalar decomposition)
add_qret_test(transforms/scalar delete_consecutive_same_pauli)
add_qret_test(transforms/scalar delete_opt_hint)
add_qret_test(transforms/scalar ignore_global_phase)
add_qret_test(transforms/scalar static_condition_pruning)

# Test cmd
if(QRET_BUILD_APPLICATION)
  add_qret_test(cmd main)
endif()
