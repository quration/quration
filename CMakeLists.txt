cmake_minimum_required(VERSION 3.16)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(utils)
set_version(VERSION)

project(
  qret
  VERSION ${VERSION}
  DESCRIPTION "Quration: Quantum Resource Estimation Toolchain for FTQC"
  LANGUAGES CXX)
set(PROJECT_NAMESPACE qret)
message(STATUS "${PROJECT_NAME} version: ${PROJECT_VERSION}")

set(CORE_LIB_NAME "${PROJECT_NAME}-core")
set(ALGORITHM_LIB_NAME "${PROJECT_NAME}-algorithm")
set(EXE_NAME "${PROJECT_NAME}")
set(PYBIND_NAME "_${PROJECT_NAME}_impl")
set(PY_LIB_NAME "py${PROJECT_NAME}")

#
# Set general options.
#
option(BUILD_SHARED_LIBS "Build using shared libraries." ON)
option(QRET_DEV_MODE "Build QRET library in developer mode." OFF)
option(QRET_BUILD_APPLICATION "Build QRET application." ON)
option(QRET_BUILD_ALGORITHM "Build QRET algorithm library." OFF)
option(QRET_BUILD_EXAMPLE "Build QRET examples." OFF)
option(QRET_BUILD_TEST "Build QRET tests." OFF)
option(QRET_BUILD_PYTHON "Build python wrapper of QRET." OFF)
option(QRET_PYTHON_WITH_ALGORITHM
       "Build python wrapper with algorithm support." OFF)
option(QRET_MEASURE_COVERAGE "Measure coverage of QRET library." OFF)
option(QRET_PROFILE_CPU "Profile CPU usage" OFF)
message(STATUS "QRET general options.")
message(STATUS "  * BUILD_SHARED_LIBS         : ${BUILD_SHARED_LIBS}")
message(STATUS "  * QRET_DEV_MODE             : ${QRET_DEV_MODE}")
message(STATUS "  * QRET_BUILD_APPLICATION    : ${QRET_BUILD_APPLICATION}")
message(STATUS "  * QRET_BUILD_EXAMPLE        : ${QRET_BUILD_EXAMPLE}")
message(STATUS "  * QRET_BUILD_TEST           : ${QRET_BUILD_TEST}")
message(STATUS "  * QRET_BUILD_PYTHON         : ${QRET_BUILD_PYTHON}")
message(STATUS "  * QRET_BUILD_ALGORITHM      : ${QRET_BUILD_ALGORITHM}")
message(STATUS "  * QRET_PYTHON_WITH_ALGORITHM: ${QRET_PYTHON_WITH_ALGORITHM}")
message(STATUS "  * QRET_MEASURE_COVERAGE     : ${QRET_MEASURE_COVERAGE}")
message(STATUS "  * QRET_PROFILE_CPU          : ${QRET_PROFILE_CPU}")

#
# Set detailed options.
#
option(QRET_USE_PEGTL "Use PEGTL." ON)
option(QRET_USE_QULACS "Use qulacs." OFF)
message(STATUS "QRET detailed options.")
message(STATUS "  * QRET_USE_PEGTL : ${QRET_USE_PEGTL}")
message(STATUS "  * QRET_USE_QULACS: ${QRET_USE_QULACS}")

#
# Set default build type to be Release.
#
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE
        STRING
        "Choose the type of build, options are: Debug Release. (default: Release)"
        FORCE)
endif()

#
# Check opions.
#
function(set_to_off option description)
  if(${option})
    message(WARNING "Set ${option} to OFF ${description}")
    set(${option}
        OFF
        CACHE BOOL "Set ${option} to OFF ${description}" FORCE)
  endif()
endfunction(set_to_off)
function(set_to_on option description)
  if(NOT ${option})
    message(WARNING "Set ${option} to ON ${description}")
    set(${option}
        ON
        CACHE BOOL "Set ${option} to ON ${description}" FORCE)
  endif()
endfunction(set_to_on)
if(MSVC)
  # Collect all binaries in a stable path on Windows.
  set(QRET_WINDOWS_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/bin>")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${QRET_WINDOWS_OUTPUT_DIRECTORY}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${QRET_WINDOWS_OUTPUT_DIRECTORY}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${QRET_WINDOWS_OUTPUT_DIRECTORY}")

  foreach(option QRET_DEV_MODE QRET_MEASURE_COVERAGE QRET_USE_QULACS)
    set_to_off(${option} "in Windows.")
  endforeach(option)
endif()
if(QRET_BUILD_PYTHON)
  foreach(option QRET_BUILD_APPLICATION QRET_BUILD_EXAMPLE QRET_BUILD_TEST
                 QRET_MEASURE_COVERAGE QRET_USE_QULACS)
    set_to_off(${option} "when building pyqret.")
  endforeach(option)
  if(QRET_PYTHON_WITH_ALGORITHM)
    set_to_on(QRET_BUILD_ALGORITHM "when building pyqret with algorithm.")
  else()
    set_to_off(QRET_BUILD_ALGORITHM "when building pyqret without algorithm.")
  endif()
endif()

#
# Include deps.cmake
#
include(deps)

#
# Configure version header.
#
configure_file(cmake/version.h.in quration-core/src/qret/version.h)

#
# Configure options.
#
if(QRET_BUILD_TEST)
  enable_testing()
endif()

#
# Configure global options.
#

# Always generate compile_commands.json.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#
# Settings for developer.
#
if(QRET_DEV_MODE)
  find_program(CCACHE_PROGRAM ccache)

  if(CCACHE_PROGRAM)
    message(STATUS "Enable ccache.")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  endif()
endif()

#
# Add subdirectories.
#
add_subdirectory(quration-core/src)
if(QRET_BUILD_ALGORITHM)
  add_subdirectory(quration-algorithm/src)
endif()

if(QRET_BUILD_APPLICATION)
  add_subdirectory_with_runtime_output(quration-core/main
                                       "${CMAKE_BINARY_DIR}/main")
endif()

if(QRET_BUILD_EXAMPLE)
  add_subdirectory_with_runtime_output(quration-core/examples
                                       "${CMAKE_BINARY_DIR}/examples")
  if(QRET_BUILD_ALGORITHM)
    add_subdirectory_with_runtime_output(quration-algorithm/examples
                                         "${CMAKE_BINARY_DIR}/examples")
  endif()
endif()

if(QRET_BUILD_TEST)
  add_subdirectory(quration-core/tests)
  if(QRET_BUILD_ALGORITHM)
    add_subdirectory(quration-algorithm/tests)
  endif()
endif()

if(QRET_BUILD_PYTHON)
  add_subdirectory(quration-core/python)
endif()

#
# Configure install settings.
#
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
configure_package_config_file(
  cmake/${PROJECT_NAME}Config.cmake.in
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
  NO_CHECK_REQUIRED_COMPONENTS_MACRO)
install(
  FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
  COMPONENT Devel)
if(MSVC)
  foreach(_target IN ITEMS yaml-cpp::yaml-cpp Boost::program_options)
    if(TARGET ${_target})
      get_target_property(_dll_path ${_target} IMPORTED_LOCATION_RELEASE)
      if(NOT _dll_path)
        get_target_property(_dll_path ${_target} IMPORTED_LOCATION)
      endif()

      if(_dll_path)
        install(FILES ${_dll_path} DESTINATION ${CMAKE_INSTALL_BINDIR})
      else()
        message(WARNING "${_target} runtime DLL not found, manual copying might be needed.")
      endif()

      unset(_dll_path)
    else()
      message(WARNING "${_target} target not found, manual copying might be needed.")
    endif()
  endforeach()
endif()
install(
  TARGETS ${CORE_LIB_NAME}
  EXPORT ${PROJECT_NAME}Targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(QRET_BUILD_ALGORITHM)
  install(
    TARGETS ${ALGORITHM_LIB_NAME}
    EXPORT ${PROJECT_NAME}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(
  EXPORT ${PROJECT_NAME}Targets
  NAMESPACE ${PROJECT_NAMESPACE}::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
install(
  DIRECTORY quration-core/src/qret
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  COMPONENT Devel
  FILES_MATCHING
  PATTERN "*.h")
if(QRET_BUILD_ALGORITHM)
  install(
    DIRECTORY quration-algorithm/src/qret/algorithm
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qret
    COMPONENT Devel
    FILES_MATCHING
    PATTERN "*.h")
endif()
install(FILES "${PROJECT_BINARY_DIR}/quration-core/src/qret/version.h"
        DESTINATION include/qret)
