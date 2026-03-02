# Boost
set(Boost_NO_WARN_NEW_VERSIONS 1)
find_package(Boost REQUIRED)
if(QRET_BUILD_APPLICATION OR QRET_BUILD_EXAMPLE)
  find_package(Boost REQUIRED COMPONENTS program_options)
endif()

# fmt
find_package(fmt CONFIG REQUIRED)

# Eigen3
find_package(Eigen3 CONFIG REQUIRED)

# nlohmann-json
find_package(nlohmann_json CONFIG REQUIRED)

# pcg
find_path(PCG_INCLUDE_DIRS "pcg_extras.hpp")

# yaml-cpp
find_package(yaml-cpp CONFIG REQUIRED)

# PEGTL
if(QRET_USE_PEGTL)
  find_package(pegtl QUIET)

  if(${pegtl_FOUND})
    message(STATUS "Find PEGTL.")
  else()
    message(STATUS "Download PEGTL.")
    include(FetchContent)
    FetchContent_Declare(
      pegtl
      GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
      GIT_TAG 3.2.7)
    FetchContent_MakeAvailable(pegtl)
  endif()
endif()

# Qulacs
if(QRET_USE_QULACS)
  if(DEFINED ENV{QULACS_PATH})
    set(QULACS_BASE_DIR $ENV{QULACS_PATH})
  else()
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/_externals/v0.6.2.tar.gz)
      message(STATUS "Download Qulacs.")
      execute_process(COMMAND mkdir -p _externals/
                      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
      execute_process(
        COMMAND curl -o _externals/v0.6.2.tar.gz -L
                https://github.com/qulacs/qulacs/archive/refs/tags/v0.6.2.tar.gz
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()

    if(NOT EXISTS ${CMAKE_BINARY_DIR}/_externals/qulacs-0.6.2)
      execute_process(
        COMMAND tar -xzvf v0.6.2.tar.gz
        OUTPUT_QUIET
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/_externals)
    endif()

    set(QULACS_BASE_DIR ${CMAKE_BINARY_DIR}/_externals/qulacs-0.6.2)
    message(STATUS "Configure Qulacs.")
    execute_process(
      COMMAND
        cmake -S . -B build -D CMAKE_C_COMPILER=${CMAKE_C_COMPILER} -D
        CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -D
        BOOST_INCLUDEDIR=${Boost_INCLUDE_DIR} -G "Unix Makefiles" -D
        CMAKE_BUILD_TYPE=Release -D USE_OMP:STR=No -D USE_GPU:STR=No -D
        USE_PYTHON:STR=No -DUSE_TEST=No
      WORKING_DIRECTORY ${QULACS_BASE_DIR})
    message(STATUS "Build Qulacs.")
    execute_process(
      COMMAND cmake --build build -- -j
      WORKING_DIRECTORY ${QULACS_BASE_DIR}
      OUTPUT_QUIET)
  endif()
endif()

# GridSynth
if(DEFINED ENV{GRIDSYNTH_PATH})
  set(GRIDSYNTH_PATH $ENV{GRIDSYNTH_PATH})
  message(STATUS "Use gridsynth: ${GRIDSYNTH_PATH}")
else()
  message(
    WARNING
      "Environment variable GRIDSYNTH_PATH is not set, so rotation gates may not be available."
  )

  if(QRET_BUILD_TEST)
    if(MSVC)
      set(GRIDSYNTH_PATH ${PROJECT_SOURCE_DIR}/externals/bin/gridsynth.exe)
    else()
      set(GRIDSYNTH_PATH ${PROJECT_SOURCE_DIR}/externals/bin/gridsynth)
    endif()
  endif()
endif()

if(QRET_DEV_MODE)
  # sanitizer
  set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/externals/sanitizers-cmake/cmake"
                        ${CMAKE_MODULE_PATH})
  find_package(Sanitizers)
endif()

if(QRET_BUILD_TEST)
  # Google Test
  find_package(GTest CONFIG REQUIRED)
  include(GoogleTest)
endif()

if(QRET_BUILD_PYTHON)
  find_package(
    Python 3.10 REQUIRED
    COMPONENTS Interpreter Development.Module
    OPTIONAL_COMPONENTS Development.SABIModule)

  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE
        Release
        CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                                 "MinSizeRel" "RelWithDebInfo")
  endif()

  # nanobind
  add_subdirectory(${CMAKE_SOURCE_DIR}/externals/nanobind)
endif()
