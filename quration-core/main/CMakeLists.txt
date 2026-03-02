set(TARGET_NAME "${PROJECT_NAME}")
set(CORE_LIB_NAME "${PROJECT_NAME}-core")

add_executable(${TARGET_NAME} main.cpp)
target_link_libraries(${TARGET_NAME} PRIVATE ${CORE_LIB_NAME})
set_target_properties(
  ${TARGET_NAME}
  PROPERTIES CXX_STANDARD 20
             CXX_STANDARD_REQUIRED ON
             CXX_EXTENSIONS OFF)
target_compile_options(${TARGET_NAME} PRIVATE ${QRET_COMPILE_OPTIONS})
install(TARGETS ${TARGET_NAME})
