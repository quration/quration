set(TARGET_NAME "_${PROJECT_NAME}_impl")
set(PY_TARGET_NAME "py${PROJECT_NAME}")
set(CORE_LIB_NAME "${PROJECT_NAME}-core")

set(PYBIND_SOURCES
    # Main
    ../pybind/src/pyqret.cpp
    ../pybind/src/backend/backend.cpp
    ../pybind/src/backend/mf.cpp
    ../pybind/src/frontend/transforms/inliner.cpp
    ../pybind/src/frontend/gate.cpp
    ../pybind/src/frontend/frontend.cpp
    ../pybind/src/frontend/ir.cpp
    ../pybind/src/frontend/pycircuit.cpp
    ../pybind/src/runtime/runtime.cpp)
if(QRET_BUILD_ALGORITHM)
  list(APPEND PYBIND_SOURCES ../pybind/src/algorithm/algorithm.cpp)
endif()

nanobind_add_module(${TARGET_NAME} STABLE_ABI NB_STATIC NB_SUPPRESS_WARNINGS
                    ${PYBIND_SOURCES})
target_link_libraries(${TARGET_NAME} PRIVATE ${CORE_LIB_NAME})
if(QRET_BUILD_ALGORITHM)
  target_link_libraries(${TARGET_NAME} PRIVATE ${ALGORITHM_LIB_NAME})
  target_compile_definitions(${TARGET_NAME} PRIVATE "QRET_BUILD_ALGORITHM")
endif()
target_include_directories(
  ${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../pybind/src
                         ${CMAKE_BINARY_DIR}/quration-core/src)
target_compile_definitions(${TARGET_NAME} PRIVATE "QRET_BUILD_PYTHON")
set_target_properties(
  ${TARGET_NAME}
  PROPERTIES CXX_STANDARD 20
             CXX_STANDARD_REQUIRED ON
             CXX_EXTENSIONS OFF)

function(add_stub stub_name module_name output)
  nanobind_add_stub(
    ${stub_name}
    INSTALL_TIME
    MODULE
    ${module_name}
    OUTPUT
    ${output}/__init__.pyi
    PYTHON_PATH
    $<TARGET_FILE_DIR:${TARGET_NAME}>
    MARKER_FILE
    ${output}/py.typed
    DEPENDS
    ${TARGET_NAME}
    VERBOSE)
endfunction()

if(WIN32)
  get_target_property(YAML_CPP_DLL yaml-cpp::yaml-cpp IMPORTED_LOCATION_RELEASE)
  if(NOT YAML_CPP_DLL)
    message(
      FATAL_ERROR
        "yaml-cpp DLL not found. Ensure yaml-cpp is installed via vcpkg.")
  endif()

  install(FILES ${YAML_CPP_DLL} DESTINATION ${PY_TARGET_NAME})
endif()

install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${PY_TARGET_NAME})
