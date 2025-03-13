cmake_minimum_required(VERSION 3.14)

project(idym)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(idym INTERFACE)
target_include_directories(idym INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(idym_test_variant
    tests/variant.cpp
    include/idym/variant.hpp
    include/idym/type_traits.hpp
    include/idym/utility.hpp
)
target_link_libraries(idym_test_variant PRIVATE idym)
