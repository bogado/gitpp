project(gitpp CXX)
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
include_directories(src)

add_executable(gitpp_test test/test.cpp)

include_directories(TARGET gitpp_test vendor/bandit)
set_property(TARGET gitpp_test PROPERTY COMPILE_OPTIONS --std=c++1z)
set_property(TARGET gitpp_test PROPERTY INTERFACE_COMPILE_OPTIONS --std=c++1z)
set_property(TARGET gitpp_test PROPERTY CXX_STANDARD_REQUIRED ON)
add_definitions(-DTEST_RESOURCE_PATH=\"${PROJECT_SOURCE_DIR}/test/res\")
