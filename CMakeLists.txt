project(gitpp CXX)
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
include_directories(src)
enable_testing()

find_package( Boost 1.60 REQUIRED COMPONENTS iostreams)
find_package( ZLIB REQUIRED )

include_directories(${Boost_INCLUDE_DIRS})

link_libraries("stdc++fs")
if(ZLIB_FOUND OR BOOST_FOUND)
    link_libraries(${ZLIB_LIBRARIES} ${Boost_LIBRARIES})
else()
    message(SEND_ERROR "Could not find zlib or boost iostream '${ZLIB_LIBRARIES}'")
endif()

set(compiler_check_level ALL_WARNINGS CACHE STRING "Defines how pedant the compiler will be, RELAXED, ALL_WARNINGS, EXTRA_WARNINGS or PEDANT.")

if ("${compiler_check_level}" STREQUAL "RELAXED")
    add_compile_options( "-Wall")
elseif("${compiler_check_level}" STREQUAL "ALL_WARNINGS")
    add_compile_options( "-Wall" "-Werror")
elseif ("${compiler_check_level}" STREQUAL "EXTRA_WARNINGS")
    add_compile_options("-Wall" "-Wextra" "-Werror")
elseif ("${compiler_check_level}" STREQUAL "PEDANT")
    add_compile_options("-Wall" "-Weverything" "-Werror" )
endif()

add_compile_options("--std=c++1z")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options("-Wno-global-constructors" "-Wno-missing-prototypes")
    add_compile_options("-Wno-c++98-compat")
    add_compile_options("-ftemplate-backtrace-limit=0")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options("-Wno-unknown-pragmas")
endif()

add_executable(gitpp_test
    test/test.cpp
    test/shared_container_test.cpp
    test/big_unsigned_test.cpp
    test/pack_index_test.cpp
    test/pack_loader_test.cpp
    )

add_executable(pack_ls
    samples/pack_ls.cpp)


add_executable(pack_cat_obj
    samples/pack_cat_obj.cpp)


include_directories(TARGET gitpp_test)
include_directories(TARGET gitpp_test SYSTEM vendor/bandit)
set_property(TARGET gitpp_test PROPERTY CXX_STANDARD_REQUIRED ON)
add_definitions(-DTEST_RESOURCE_PATH=\"${PROJECT_SOURCE_DIR}/test/res\")

add_test(GitPPTest gitpp_test)
add_custom_target(check COMMAND ./gitpp_test DEPENDS gitpp_test)
