set(CORE_LIB_NAME "${PROJECT_NAME}-core")

function(add_exe filename)
  set(target ${filename})
  add_executable(${target} ${target}.cpp)
  target_link_libraries(${target} PRIVATE Boost::program_options
                                          ${CORE_LIB_NAME})
  target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  set_target_properties(
    ${target}
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF)
  target_compile_options(${target} PRIVATE ${QRET_COMPILE_OPTIONS})
  install(TARGETS ${target})
endfunction()

add_exe(create_random)
add_exe(external_decompose_pass)
add_exe(external_mapping_pass)
add_exe(grover)
add_exe(portable_function)
add_exe(trotter)
