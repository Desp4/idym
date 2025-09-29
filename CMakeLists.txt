cmake_minimum_required(VERSION 3.14)

project(idym)
option(IDYM_BUILD_TESTS "Build tests" ON)

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
endif()

add_library(idym INTERFACE)
target_include_directories(idym INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (IDYM_BUILD_TESTS)
    enable_testing()

    add_executable(idym_test_variant tests/variant.cpp tests/idym_test.hpp)
    target_link_libraries(idym_test_variant PRIVATE idym)
    add_test(NAME idym-variant COMMAND idym_test_variant)
    
    add_executable(idym_test_expected tests/expected.cpp tests/idym_test.hpp)
    target_link_libraries(idym_test_expected PRIVATE idym)
    add_test(NAME idym-expected COMMAND idym_test_expected)
endif()
