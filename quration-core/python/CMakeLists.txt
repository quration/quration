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
# qret-core is an OBJECT library in the python build; its PRIVATE protobuf link
# (used by the Qualtran frontend/parser) does not propagate to consumers of the
# object files, so link the runtime explicitly here.
target_link_libraries(${TARGET_NAME} PRIVATE protobuf::libprotobuf)
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

# RUNTIME_DEPENDENCY_SET lets install() discover the full transitive closure of
# ${TARGET_NAME}'s runtime DLLs (yaml-cpp, protobuf, ...) automatically,
# instead of hand-maintaining a list of "known" DLL targets.
install(
  TARGETS ${TARGET_NAME}
  RUNTIME_DEPENDENCY_SET ${PROJECT_NAME}PythonRuntimeDeps
  LIBRARY DESTINATION ${PY_TARGET_NAME})
if(WIN32)
  install(
    RUNTIME_DEPENDENCY_SET ${PROJECT_NAME}PythonRuntimeDeps
    DESTINATION ${PY_TARGET_NAME}
    # python3<minor>[_d].dll is provided by the Python installation that
    # loads this module at runtime, not by the build tree or vcpkg; it must
    # not be bundled into the wheel.
    PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "python3[0-9]*(_d)?\\.dll"
    POST_EXCLUDE_REGEXES ".*system32.*")
endif()
