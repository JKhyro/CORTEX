cmake_minimum_required(VERSION 3.20)

project(CORTEX
  VERSION 0.1.0
  DESCRIPTION "Native CORTEX runtime components"
  LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(cortex_imported_catalog_mapper STATIC
  src/imported_catalog_mapper.c)

add_library(cortex_character_runtime STATIC
  src/character_runtime.c)

target_include_directories(cortex_imported_catalog_mapper
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(cortex_character_runtime
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(cortex_character_runtime
  PUBLIC cortex_imported_catalog_mapper)

if(MSVC)
  target_compile_options(cortex_imported_catalog_mapper PRIVATE /W4 /permissive-)
  target_compile_options(cortex_character_runtime PRIVATE /W4 /permissive-)
else()
  target_compile_options(cortex_imported_catalog_mapper PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(cortex_character_runtime PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(cortex_imported_catalog_mapper_tests
  tests/test_imported_catalog_mapper.c)

add_executable(cortex_character_runtime_tests
  tests/test_character_runtime.c)

target_link_libraries(cortex_imported_catalog_mapper_tests
  PRIVATE cortex_imported_catalog_mapper)

target_link_libraries(cortex_character_runtime_tests
  PRIVATE cortex_character_runtime)

if(MSVC)
  target_compile_options(cortex_imported_catalog_mapper_tests PRIVATE /W4 /permissive-)
  target_compile_options(cortex_character_runtime_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(cortex_imported_catalog_mapper_tests PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(cortex_character_runtime_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME cortex_imported_catalog_mapper_tests
  COMMAND cortex_imported_catalog_mapper_tests)

add_test(NAME cortex_character_runtime_tests
  COMMAND cortex_character_runtime_tests)
