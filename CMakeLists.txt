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
    add_executable(idym_test_variant
        tests/variant.cpp
        include/idym/variant.hpp
        include/idym/type_traits.hpp
        include/idym/utility.hpp
    )
    target_link_libraries(idym_test_variant PRIVATE idym)
    
    enable_testing()
    add_test(NAME idym-variant COMMAND idym_test_variant)
endif()
