set(TARGET_NAME "${PROJECT_NAME}-core")

if(MSVC)
  list(
    APPEND
    QRET_PRIVATE_COMPILE_OPTIONS
    # Private compile options for MSVC:
    "/bigobj" # Allow big object
    "/DNOMINMAX"
    "/DWIN32_LEAN_AND_MEAN=1"
    "/D_CRT_SECURE_NO_WARNINGS"
    "/D_CRT_SECURE_NO_DEPRECATE"
    "/MP" # Build with multiple processes
    "/DNDEBUG" # Always define NDEBUG.
  )
  list(APPEND QRET_PUBLIC_COMPILE_OPTIONS
       # Public compile options for MSVC:
       "/Zc:preprocessor" # Enable preprocessor conformance mode
  )
else()
  list(
    APPEND
    QRET_PRIVATE_COMPILE_OPTIONS
    "-Wall"
    "-Wextra"
    "-Wcast-align"
    "-Wcast-qual"
    "-Wdelete-non-virtual-dtor"
    "-Wold-style-cast"
    "-Woverloaded-virtual"
    "-Wpointer-arith"
    "-Wfloat-equal"
    "-Winit-self"
    "-Wredundant-decls"
    "-Wconversion"
    "-Wsign-promo"
    "-Wswitch-default"
    "-Wvariadic-macros"
    "-Wwrite-strings"
    "-Wno-unknown-pragmas")

  list(APPEND QRET_COMPILE_DEFINITIONS "_GLIBCXX_USE_CXX11_ABI=1")
endif()

if(MSVC)
  list(
    APPEND
    QRET_COMPILE_DEFINITIONS
    # Compile definitions for Windows
    "GLOG_NO_ABBREVIATED_SEVERITIES"
    "BOOST_UUID_FORCE_AUTO_LINK"
    "QRET_BUILDING_DLL")
else()

endif()

if(QRET_USE_PEGTL)
  list(APPEND QRET_COMPILE_DEFINITIONS "USE_PEGTL")
endif()

if(QRET_USE_QULACS)
  list(APPEND QRET_COMPILE_DEFINITIONS "USE_QULACS")
endif()

list(
  APPEND
  QRET_SOURCE_FILES
  # Analysis
  qret/analysis/circuit_drawer.cpp
  qret/analysis/compute_graph.cpp
  qret/analysis/counter.cpp
  qret/analysis/dot.cpp
  qret/analysis/printer.cpp
  qret/analysis/visualizer.cpp
  # Base
  qret/base/gridsynth.cpp
  qret/base/graph.cpp
  qret/base/json.cpp
  qret/base/log.cpp
  qret/base/option.cpp
  qret/base/portable_function.cpp
  qret/base/string.cpp
  qret/base/system.cpp
  qret/base/type.cpp
  # Codegen
  qret/codegen/asm_printer.cpp
  qret/codegen/asm_streamer.cpp
  qret/codegen/dummy.cpp
  qret/codegen/machine_function_pass.cpp
  qret/codegen/machine_function.cpp
  # Frontend
  qret/frontend/argument.cpp
  qret/frontend/attribute.cpp
  qret/frontend/builder.cpp
  qret/frontend/circuit.cpp
  qret/frontend/circuit_generator.cpp
  qret/frontend/control_flow.cpp
  qret/frontend/functor.cpp
  qret/frontend/intrinsic.cpp
  qret/frontend/openqasm2.cpp
  # IR
  qret/ir/basic_block.cpp
  qret/ir/function.cpp
  qret/ir/instruction.cpp
  qret/ir/instructions.cpp
  qret/ir/json.cpp
  qret/ir/module.cpp
  # Math
  qret/math/boolean_formula.cpp
  qret/math/clifford.cpp
  qret/math/float.cpp
  qret/math/fraction.cpp
  qret/math/integer.cpp
  qret/math/lcu_helper.cpp
  qret/math/pauli.cpp
  # Parser
  qret/parser/openqasm2.cpp
  qret/parser/openqasm3.cpp
  # Runtime
  qret/runtime/clifford_state.cpp
  qret/runtime/full_quantum_state.cpp
  qret/runtime/simulator.cpp
  qret/runtime/toffoli_state.cpp
  # Target
  qret/target/target_machine.cpp
  qret/target/target_registry.cpp
  # Target/SC_LS_FIXED_V0
  qret/target/sc_ls_fixed_v0/target_info/sc_ls_fixed_v0_target_info.cpp
  qret/target/sc_ls_fixed_v0/asm_printer.cpp
  qret/target/sc_ls_fixed_v0/calc_compile_info.cpp
  qret/target/sc_ls_fixed_v0/compile_info.cpp
  qret/target/sc_ls_fixed_v0/external_pass.cpp
  qret/target/sc_ls_fixed_v0/inst_queue.cpp
  qret/target/sc_ls_fixed_v0/instruction.cpp
  qret/target/sc_ls_fixed_v0/pipeline_state.cpp
  qret/target/sc_ls_fixed_v0/lowering.cpp
  qret/target/sc_ls_fixed_v0/mapping.cpp
  qret/target/sc_ls_fixed_v0/routing.cpp
  qret/target/sc_ls_fixed_v0/sc_ls_fixed_v0_compile_backend.cpp
  qret/target/sc_ls_fixed_v0/runtime_simulation_pruning.cpp
  qret/target/sc_ls_fixed_v0/search_chip_comm.cpp
  qret/target/sc_ls_fixed_v0/search_grid.cpp
  qret/target/sc_ls_fixed_v0/simulator.cpp
  qret/target/sc_ls_fixed_v0/state.cpp
  qret/target/sc_ls_fixed_v0/topology.cpp
  qret/target/sc_ls_fixed_v0/validation.cpp
  # Target/TUTORIAL_NISQ_V0
  qret/target/tutorial_nisq_v0/target_info/tutorial_nisq_v0_target_info.cpp
  qret/target/tutorial_nisq_v0/asm_printer.cpp
  qret/target/tutorial_nisq_v0/instruction.cpp
  qret/target/tutorial_nisq_v0/lowering.cpp
  qret/target/tutorial_nisq_v0/tutorial_nisq_v0_compile_backend.cpp
  # Transforms
  qret/transforms/external/external_pass.cpp
  qret/transforms/ipo/inliner.cpp
  qret/transforms/scalar/decomposition.cpp
  qret/transforms/scalar/delete_consecutive_same_pauli.cpp
  qret/transforms/scalar/delete_opt_hint.cpp
  qret/transforms/scalar/ignore_global_phase.cpp
  qret/transforms/scalar/static_condition_pruning.cpp
  # Root
  qret/pass.cpp
  #
)

if(QRET_BUILD_APPLICATION)
  list(
    APPEND
    QRET_SOURCE_FILES
    # Target compile backend
    qret/target/compile_format.cpp
    # CMD
    qret/cmd/compile.cpp
    qret/cmd/asm.cpp
    qret/cmd/diagram.cpp
    qret/cmd/main.cpp
    qret/cmd/opt.cpp
    qret/cmd/parse.cpp
    qret/cmd/print.cpp
    qret/cmd/profile.cpp
    qret/cmd/simulate.cpp)
endif()

if(QRET_BUILD_PYTHON)
  message(STATUS "Build ${TARGET_NAME} as object library")
  add_library(${TARGET_NAME} OBJECT ${QRET_SOURCE_FILES})
  list(APPEND QRET_COMPILE_DEFINITIONS "QRET_BUILD_PYTHON")
elseif(BUILD_SHARED_LIBS OR QRET_MEASURE_COVERAGE)
  message(STATUS "Build ${TARGET_NAME} as shared library")
  add_library(${TARGET_NAME} SHARED ${QRET_SOURCE_FILES})
else()
  message(STATUS "Build ${TARGET_NAME} as static library")
  message(
    WARNING
      "When building as a static library, some features may not be available.")
  add_library(${TARGET_NAME} STATIC ${QRET_SOURCE_FILES})
endif()
add_library(${TARGET_NAME}::${TARGET_NAME} ALIAS ${TARGET_NAME})

target_include_directories(
  ${TARGET_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${PCG_INCLUDE_DIRS}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
         $<INSTALL_INTERFACE:include>)
target_link_libraries(
  ${TARGET_NAME}
  PUBLIC Boost::boost fmt::fmt-header-only nlohmann_json::nlohmann_json
  PRIVATE Eigen3::Eigen)

if(QRET_BUILD_APPLICATION)
  target_link_libraries(${TARGET_NAME} PUBLIC Boost::program_options
                                              yaml-cpp::yaml-cpp)
else()
  target_link_libraries(${TARGET_NAME} PRIVATE yaml-cpp::yaml-cpp)
endif()

set_target_properties(
  ${TARGET_NAME}
  PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
    VERSION
    ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_compile_definitions(${TARGET_NAME} PRIVATE ${QRET_COMPILE_DEFINITIONS})
target_compile_options(${TARGET_NAME} PRIVATE ${QRET_PRIVATE_COMPILE_OPTIONS})
target_compile_options(${TARGET_NAME} PUBLIC ${QRET_PUBLIC_COMPILE_OPTIONS})

if(BUILD_SHARED_LIBS OR QRET_BUILD_PYTHON)
  if(APPLE)
    target_link_options(${TARGET_NAME} PRIVATE -Wl,-undefined,error
                        -Wl,-dead_strip_dylibs)
  elseif(UNIX)
    target_link_options(${TARGET_NAME} PRIVATE -Wl,--no-undefined
                        -Wl,--as-needed -Wl,-z,relro -Wl,-z,now)
  endif()
else()
  target_link_options(${TARGET_NAME} PRIVATE -Wl,--exclude-libs,ALL)
endif()

if(QRET_USE_PEGTL)
  target_include_directories(${TARGET_NAME}
                             PRIVATE ${PEGTL_INSTALL_INCLUDE_DIR})
endif()

if(QRET_USE_QULACS)
  target_include_directories(${TARGET_NAME} PRIVATE ${QULACS_BASE_DIR}/src/)
  target_link_directories(${TARGET_NAME} PRIVATE ${QULACS_BASE_DIR}/lib)
  target_link_libraries(
    ${TARGET_NAME}
    PRIVATE ${QULACS_BASE_DIR}/lib/libvqcsim_static.a
            ${QULACS_BASE_DIR}/lib/libcppsim_static.a
            ${QULACS_BASE_DIR}/lib/libcsim_static.a)
endif()

if(QRET_BUILD_PYTHON)
  set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

if(QRET_MEASURE_COVERAGE)
  target_compile_options(${TARGET_NAME} PRIVATE -g -fprofile-instr-generate
                                                -fcoverage-mapping)
  target_link_options(${TARGET_NAME} PRIVATE -g -fprofile-instr-generate
                      -fcoverage-mapping)
endif()

if(QRET_DEV_MODE)
  add_sanitizers(${TARGET_NAME})
endif()

if(QRET_PROFILE_CPU)
  target_compile_options(${TARGET_NAME} PRIVATE -pg)
endif()
