set(TARGET_NAME "${PROJECT_NAME}-algorithm")

if(MSVC)
  list(
    APPEND
    QRET_ALGO_PRIVATE_COMPILE_OPTIONS
    # Private compile options for MSVC:
    "/bigobj" # Allow big object
    "/DNOMINMAX"
    "/DWIN32_LEAN_AND_MEAN=1"
    "/D_CRT_SECURE_NO_WARNINGS"
    "/D_CRT_SECURE_NO_DEPRECATE"
    "/MP" # Build with multiple processes
    "/DNDEBUG" # Always define NDEBUG.
  )
  list(APPEND QRET_ALGO_PUBLIC_COMPILE_OPTIONS
       # Public compile options for MSVC:
       "/Zc:preprocessor" # Enable preprocessor conformance mode
  )
else()
  list(
    APPEND
    QRET_ALGO_PRIVATE_COMPILE_OPTIONS
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

  list(APPEND QRET_ALGO_COMPILE_DEFINITIONS "_GLIBCXX_USE_CXX11_ABI=1")
endif()

if(MSVC)
  list(
    APPEND
    QRET_ALGO_COMPILE_DEFINITIONS
    # Compile definitions for Windows
    "GLOG_NO_ABBREVIATED_SEVERITIES"
    "BOOST_UUID_FORCE_AUTO_LINK"
    "QRET_BUILDING_DLL")
endif()

if(QRET_USE_PEGTL)
  list(APPEND QRET_ALGO_COMPILE_DEFINITIONS "USE_PEGTL")
endif()

if(QRET_USE_QULACS)
  list(APPEND QRET_ALGO_COMPILE_DEFINITIONS "USE_QULACS")
endif()

list(
  APPEND
  QRET_ALGO_SOURCE_FILES
  # Algorithm
  qret/algorithm/transform/qft.cpp
  qret/algorithm/phase_estimation/qpe.cpp
  qret/algorithm/phase_estimation/trotter.cpp
  qret/algorithm/arithmetic/boolean.cpp
  qret/algorithm/arithmetic/integer.cpp
  qret/algorithm/arithmetic/modular.cpp
  qret/algorithm/arithmetic/shor.cpp
  qret/algorithm/data/qdict.cpp
  qret/algorithm/data/qrom.cpp
  qret/algorithm/preparation/arbitrary.cpp
  qret/algorithm/preparation/uniform.cpp
  qret/algorithm/util/array.cpp
  qret/algorithm/util/bitwise.cpp
  qret/algorithm/util/multi_control.cpp
  qret/algorithm/util/reset.cpp
  qret/algorithm/util/rotation.cpp
  qret/algorithm/util/util.cpp
  qret/algorithm/util/write_register.cpp)

if(QRET_BUILD_PYTHON)
  message(STATUS "Build ${TARGET_NAME} as object library")
  add_library(${TARGET_NAME} OBJECT ${QRET_ALGO_SOURCE_FILES})
  list(APPEND QRET_ALGO_COMPILE_DEFINITIONS "QRET_BUILD_PYTHON")
elseif(BUILD_SHARED_LIBS OR QRET_MEASURE_COVERAGE)
  message(STATUS "Build ${TARGET_NAME} as shared library")
  add_library(${TARGET_NAME} SHARED ${QRET_ALGO_SOURCE_FILES})
else()
  message(STATUS "Build ${TARGET_NAME} as static library")
  message(
    WARNING
      "When building as a static library, some features may not be available.")
  add_library(${TARGET_NAME} STATIC ${QRET_ALGO_SOURCE_FILES})
endif()
add_library(${TARGET_NAME}::${TARGET_NAME} ALIAS ${TARGET_NAME})

target_include_directories(
  ${TARGET_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
         $<INSTALL_INTERFACE:include>)

# Depend on core for shared headers and symbols
set(CORE_LIB_NAME "${PROJECT_NAME}-core")
target_link_libraries(${TARGET_NAME} PUBLIC ${CORE_LIB_NAME})

target_compile_definitions(${TARGET_NAME}
                           PRIVATE ${QRET_ALGO_COMPILE_DEFINITIONS})
target_compile_options(${TARGET_NAME}
                       PRIVATE ${QRET_ALGO_PRIVATE_COMPILE_OPTIONS})
target_compile_options(${TARGET_NAME}
                       PUBLIC ${QRET_ALGO_PUBLIC_COMPILE_OPTIONS})

set_target_properties(
  ${TARGET_NAME}
  PROPERTIES CXX_STANDARD 20
             CXX_STANDARD_REQUIRED ON
             CXX_EXTENSIONS OFF
             POSITION_INDEPENDENT_CODE ON)

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
