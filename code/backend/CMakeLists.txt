cmake_minimum_required(VERSION 3.16)
project(AlphaForge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2")
add_compile_options(-Wall -Wextra)

find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# Core engine library. All domain logic lives here so the CLI, REST server,
# tests and benchmarks share exactly one implementation.
# ---------------------------------------------------------------------------
file(GLOB ENGINE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(alphaforge_engine STATIC ${ENGINE_SOURCES})
target_include_directories(alphaforge_engine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(alphaforge_engine PUBLIC Threads::Threads)

# Third party header only dependencies for the REST server.
add_library(alphaforge_thirdparty INTERFACE)
target_include_directories(alphaforge_thirdparty INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/httplib"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# ---------------------------------------------------------------------------
# Executables
# ---------------------------------------------------------------------------
add_executable(alphaforge_cli app/cli_main.cpp)
target_link_libraries(alphaforge_cli PRIVATE alphaforge_engine)

add_executable(alphaforge_server app/server_main.cpp)
target_include_directories(alphaforge_server PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app")
target_link_libraries(alphaforge_server PRIVATE alphaforge_engine alphaforge_thirdparty)

add_executable(alphaforge_bench benchmarks/benchmark_main.cpp)
target_link_libraries(alphaforge_bench PRIVATE alphaforge_engine)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.cpp")
add_executable(alphaforge_tests ${TEST_SOURCES} tests/tests_main.cpp)
target_include_directories(alphaforge_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_link_libraries(alphaforge_tests PRIVATE alphaforge_engine)
add_test(NAME unit_tests COMMAND alphaforge_tests)
