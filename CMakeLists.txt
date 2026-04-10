cmake_minimum_required(VERSION 3.20)

project(CORTEX
  VERSION 0.1.0
  DESCRIPTION "Native CORTEX runtime components"
  LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
include(GNUInstallDirs)

add_library(cortex_imported_catalog_mapper STATIC
  src/imported_catalog_mapper.c)

add_library(cortex_character_runtime STATIC
  src/character_runtime.c)

add_library(cortex_runtime_persistence STATIC
  src/runtime_persistence.c)

add_executable(cortex_host
  tools/cortex_host.c)

target_include_directories(cortex_imported_catalog_mapper
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(cortex_character_runtime
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(cortex_runtime_persistence
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(cortex_character_runtime
  PUBLIC cortex_imported_catalog_mapper)

target_link_libraries(cortex_runtime_persistence
  PUBLIC cortex_character_runtime)

target_link_libraries(cortex_host
  PRIVATE cortex_runtime_persistence)

if(MSVC)
  target_compile_options(cortex_imported_catalog_mapper PRIVATE /W4 /permissive-)
  target_compile_options(cortex_character_runtime PRIVATE /W4 /permissive-)
  target_compile_options(cortex_runtime_persistence PRIVATE /W4 /permissive-)
  target_compile_options(cortex_host PRIVATE /W4 /permissive-)
else()
  target_compile_options(cortex_imported_catalog_mapper PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(cortex_character_runtime PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(cortex_runtime_persistence PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(cortex_host PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(cortex_imported_catalog_mapper_tests
  tests/test_imported_catalog_mapper.c)

add_executable(cortex_character_runtime_tests
  tests/test_character_runtime.c)

add_executable(cortex_runtime_persistence_tests
  tests/test_runtime_persistence.c)

target_link_libraries(cortex_imported_catalog_mapper_tests
  PRIVATE cortex_imported_catalog_mapper)

target_link_libraries(cortex_character_runtime_tests
  PRIVATE cortex_character_runtime)

target_link_libraries(cortex_runtime_persistence_tests
  PRIVATE cortex_runtime_persistence)

if(MSVC)
  target_compile_options(cortex_imported_catalog_mapper_tests PRIVATE /W4 /permissive-)
  target_compile_options(cortex_character_runtime_tests PRIVATE /W4 /permissive-)
  target_compile_options(cortex_runtime_persistence_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(cortex_imported_catalog_mapper_tests PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(cortex_character_runtime_tests PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(cortex_runtime_persistence_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME cortex_imported_catalog_mapper_tests
  COMMAND cortex_imported_catalog_mapper_tests)

add_test(NAME cortex_character_runtime_tests
  COMMAND cortex_character_runtime_tests)

add_test(NAME cortex_runtime_persistence_tests
  COMMAND cortex_runtime_persistence_tests)

set(CORTEX_HOST_DEMO_STATE
  ${CMAKE_CURRENT_BINARY_DIR}/cortex_host_demo.state)
set(CORTEX_HOST_INGEST_STATE
  ${CMAKE_CURRENT_BINARY_DIR}/cortex_host_ingest.state)

add_test(NAME cortex_host_demo
  COMMAND cortex_host demo ${CORTEX_HOST_DEMO_STATE})

add_test(NAME cortex_host_inspect_demo
  COMMAND cortex_host inspect ${CORTEX_HOST_DEMO_STATE})
set_tests_properties(cortex_host_inspect_demo
  PROPERTIES DEPENDS cortex_host_demo)

add_test(NAME cortex_host_ingest_fixture
  COMMAND cortex_host ingest
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cortex_catalog_input.tsv
    ${CORTEX_HOST_INGEST_STATE})

add_test(NAME cortex_host_audits_fixture
  COMMAND cortex_host audits ${CORTEX_HOST_INGEST_STATE})
set_tests_properties(cortex_host_audits_fixture
  PROPERTIES DEPENDS cortex_host_ingest_fixture)

add_test(NAME cortex_host_missing_state_fails
  COMMAND cortex_host inspect
    ${CMAKE_CURRENT_BINARY_DIR}/cortex_host_missing.state)
set_tests_properties(cortex_host_missing_state_fails
  PROPERTIES WILL_FAIL TRUE)

add_test(NAME cortex_host_bad_schema_fails
  COMMAND cortex_host inspect
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cortex_bad_schema.state)
set_tests_properties(cortex_host_bad_schema_fails
  PROPERTIES WILL_FAIL TRUE)

add_test(NAME cortex_host_invalid_catalog_fails
  COMMAND cortex_host ingest
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cortex_invalid_catalog.tsv
    ${CMAKE_CURRENT_BINARY_DIR}/cortex_host_invalid.state)
set_tests_properties(cortex_host_invalid_catalog_fails
  PROPERTIES WILL_FAIL TRUE)

install(TARGETS cortex_imported_catalog_mapper cortex_character_runtime cortex_runtime_persistence cortex_host
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES include/cortex/imported_catalog_mapper.h
  include/cortex/character_runtime.h
  include/cortex/runtime_persistence.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cortex)
